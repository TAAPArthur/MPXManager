/**
 * @file wmfunctions.c
 * @brief Connect our internal structs to X
 *
 */
/// \cond
#include <assert.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include <X11/extensions/XInput2.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xinput.h>
/// \endcond

#include "bindings.h"
#include "ewmh.h"
#include "mywm-util.h"
#include "wmfunctions.h"
#include "windows.h"
#include "masters.h"
#include "workspaces.h"
#include "devices.h"
#include "logger.h"
#include "xsession.h"
#include "monitors.h"
#include "globals.h"
#include "events.h"
#include "layouts.h"


int isWindowVisible(WindowInfo* winInfo){
    return winInfo && hasMask(winInfo, PARTIALLY_VISIBLE);
}

int isMappable(WindowInfo* winInfo){
    return winInfo && hasMask(winInfo, MAPABLE_MASK);
}
int isInteractable(WindowInfo* winInfo){
    return hasMask(winInfo,MAPPED_MASK) && !hasMask(winInfo,HIDDEN_MASK);
}
int isTileable(WindowInfo*winInfo){
    return isInteractable(winInfo) && !hasPartOfMask(winInfo, ALL_NO_TILE_MASKS);
}
int isActivatable(WindowInfo* winInfo){
    return !winInfo || hasMask(winInfo,MAPABLE_MASK | INPUT_MASK )&& !(winInfo->mask & HIDDEN_MASK);
}

int isExternallyResizable(WindowInfo* winInfo){
    return !winInfo || winInfo->mask & EXTERNAL_RESIZE_MASK;
}

int isExternallyMoveable(WindowInfo* winInfo){
    return !winInfo || winInfo->mask & EXTERNAL_MOVE_MASK;
}

int isExternallyBorderConfigurable(WindowInfo* winInfo){
    return !winInfo || winInfo->mask & EXTERNAL_BORDER_MASK;
}
int isExternallyRaisable(WindowInfo* winInfo){
    return !winInfo || winInfo->mask & EXTERNAL_RAISE_MASK;
}
int allowRequestFromSource(WindowInfo* winInfo,int source){
    return !winInfo || hasMask(winInfo, 1<<(source+SRC_INDICATION_OFFSET));
}


void connectToXserver(){

    openXDisplay();

    initCurrentMasters();
    assert(getActiveMaster()!=NULL);
    detectMonitors();
    //update workspace names
    setWorkspaceNames(NULL,0);
    syncState();
    registerForEvents();
    if(applyRules(getEventRules(onXConnection),NULL)){
        broadcastEWMHCompilence();
    }
}
void syncState(){
    xcb_ewmh_set_showing_desktop(ewmh, defaultScreenNumber, 0);
    unsigned int currentWorkspace=DEFAULT_WORKSPACE_INDEX;
    if(!LOAD_SAVED_STATE || !xcb_ewmh_get_current_desktop_reply(ewmh,
                xcb_ewmh_get_current_desktop(ewmh, defaultScreenNumber),
                &currentWorkspace, NULL)){
            currentWorkspace=DEFAULT_WORKSPACE_INDEX;
        }
    if(currentWorkspace>=getNumberOfWorkspaces())
        currentWorkspace=getNumberOfWorkspaces()-1;
    xcb_ewmh_set_number_of_desktops(ewmh, defaultScreenNumber, getNumberOfWorkspaces());
    LOG(LOG_LEVEL_INFO,"Current workspace is %d; default %d\n\n",currentWorkspace,DEFAULT_WORKSPACE_INDEX);
    switchToWorkspace(currentWorkspace);
}

void setActiveWindowProperty(int win){
    xcb_ewmh_set_active_window(ewmh, defaultScreenNumber, win);
}

static void updateEWMHWorkspaceProperties(){
    xcb_ewmh_coordinates_t viewPorts[getNumberOfWorkspaces()];
    xcb_ewmh_geometry_t workAreas[getNumberOfWorkspaces()];
    for(int i=0;i<getNumberOfWorkspaces();i++){
        if(isWorkspaceVisible(i)){
            for(int n=0;n<4;n++)
                ((int*)&workAreas[i])[n]=(&getMonitorFromWorkspace(getWorkspaceByIndex(i))->view.x)[n];
        }
    }
    xcb_ewmh_set_desktop_viewport(ewmh, defaultScreenNumber, getNumberOfWorkspaces(), viewPorts);
    xcb_ewmh_set_workarea(ewmh, defaultScreenNumber, getNumberOfWorkspaces(), workAreas);
    xcb_ewmh_set_desktop_geometry(ewmh, defaultScreenNumber, getRootWidth(), getRootHeight());

}

void updateEWMHClientList(){
    int num=getSize(getAllWindows());
    xcb_window_t stackingOrder[num];
    xcb_window_t mappingOrder[num];
    int i=0;
    FOR_EACH(WindowInfo*winInfo,getAllWindows(),stackingOrder[num-++i]=winInfo->id;)
    i=0;
    FOR_EACH(WindowInfo*winInfo,getAllWindows(),mappingOrder[num-++i]=winInfo->id);

    xcb_ewmh_set_client_list(ewmh, defaultScreenNumber, num, mappingOrder);
    xcb_ewmh_set_client_list_stacking(ewmh, defaultScreenNumber, num, stackingOrder);
}




int getSavedWorkspaceIndex(xcb_window_t win){
    unsigned int workspaceIndex=0;

    if ((xcb_ewmh_get_wm_desktop_reply(ewmh,
              xcb_ewmh_get_wm_desktop(ewmh, win), &workspaceIndex, NULL))) {
        if(workspaceIndex!=-1&&workspaceIndex>=getNumberOfWorkspaces()){
            workspaceIndex=getNumberOfWorkspaces()-1;
        }
    }
    else workspaceIndex = getActiveWorkspaceIndex();
    return workspaceIndex;
}

static void* waitForWindowToDie(int id){
    lock();
    WindowInfo*winInfo=getWindowInfo(id);
    int hasPingMask=0;
    if(winInfo){
        winInfo->pingTimeStamp=getTime();
        hasPingMask=hasMask(winInfo, WM_PING_MASK);
    }
    unlock();
    while(!isShuttingDown()){
        if(hasPingMask){
            xcb_ewmh_send_wm_ping(ewmh, id, 0);
            flush();
        }
        lock();
        winInfo=getWindowInfo(id);
        int timeout=winInfo?getTime()-winInfo->pingTimeStamp>=KILL_TIMEOUT:0;
        unlock();
        if(!winInfo){
            LOG(LOG_LEVEL_DEBUG,"Window %d no longer exists\n", id);
            break;
        }
        else if(timeout){
            LOG(LOG_LEVEL_DEBUG,"Window %d is not responsive; force killing\n", id);
            lock();killWindow(id);unlock();
            break;
        }
        msleep(KILL_TIMEOUT);
    }
    LOG(LOG_LEVEL_DEBUG,"Finshed waiting for window %d\n", id);
    return NULL;
}

void killWindowInfo(WindowInfo* winInfo){
    if(hasMask(winInfo, WM_DELETE_WINDOW_MASK)){
        xcb_client_message_event_t ev;
        ev.response_type = XCB_CLIENT_MESSAGE;
        ev.sequence = 0;
        ev.format = 32;
        ev.window = winInfo->id;
        ev.type = ewmh->WM_PROTOCOLS;
        ev.data.data32[0] = WM_DELETE_WINDOW;
        ev.data.data32[1] = getTime();
        xcb_send_event(dis, 0, winInfo->id, XCB_EVENT_MASK_NO_EVENT, (char *)&ev);
        LOG(LOG_LEVEL_INFO,"Sending request to delete window\n");
        runInNewThread((void *(*)(void *))waitForWindowToDie,(void*) winInfo->id,1);
    }
    else{
        killWindow(winInfo->id);
    }
}
void floatWindow(WindowInfo* winInfo){
    addMask(winInfo, FLOATING_MASK);
}
void sinkWindow(WindowInfo* winInfo){
    removeMask(winInfo, ALL_NO_TILE_MASKS);
}
void killWindow(xcb_window_t win){
    assert(win);
    LOG(LOG_LEVEL_DEBUG,"Killing window %d\n",win);
    catchError(xcb_kill_client_checked(dis, win));
    flush();
}

int focusWindowInfo(WindowInfo*winInfo){
    if(!hasMask(winInfo,INPUT_MASK))return 0;
    /*
    if(hasMask(winInfo, WM_TAKE_FOCUS_MASK)){
        uint32_t data[]={WM_TAKE_FOCUS,getTime()};
        LOG(LOG_LEVEL_TRACE,"sending request to take focus to ")
        xcb_ewmh_send_client_message(dis, winInfo->id, winInfo->id, ewmh->WM_PROTOCOLS, 2, data);
        return 1;
    }
    else*/

    return focusWindow(winInfo->id);
}
int focusWindow(int win){
    LOG(LOG_LEVEL_DEBUG,"Trying to set focus to %d for master %d\n",win,getActiveMaster()->id);
    assert(win);
    xcb_void_cookie_t cookie=xcb_input_xi_set_focus_checked(dis, win, XCB_CURRENT_TIME, getActiveMaster()->id);
    if(catchError(cookie)){
        return 0;
    }

    //onWindowFocus(win);
    return 1;
}
void raiseLowerWindowInfo(WindowInfo*winInfo,int above){
    assert(winInfo);
    int* values=(int[]){winInfo->transientFor,above?XCB_STACK_MODE_ABOVE:XCB_STACK_MODE_BELOW};
    int mask=XCB_CONFIG_WINDOW_STACK_MODE;
    if(winInfo->transientFor)
        mask|=XCB_CONFIG_WINDOW_SIBLING;
    else
        values++;
    xcb_configure_window(dis, winInfo->id,mask,values);
}
int raiseWindowInfo(WindowInfo* winInfo){
    int result= winInfo?raiseWindow(winInfo->id):0;
    if(result){
        FOR_EACH(WindowInfo*winInfo,getAllWindows(),
            if(isInteractable(winInfo)&&winInfo->transientFor==winInfo->id||hasMask(winInfo,ALWAYS_ON_TOP))
                raiseLowerWindowInfo(winInfo,1);
        )
    }
    return result;
}
int raiseWindow(xcb_window_t win){
    assert(dis);
    assert(win);
    LOG(LOG_LEVEL_TRACE,"Trying to raise window %d\n",win);
    int values[]={XCB_STACK_MODE_ABOVE};
    if(catchError(xcb_configure_window_checked(dis, win,XCB_CONFIG_WINDOW_STACK_MODE,values)))
        return 0;
    return 1;
}


int activateWindow(WindowInfo* winInfo){

    if(!winInfo || !isActivatable(winInfo)){
        LOG(LOG_LEVEL_TRACE,"could not activate window %d\n",winInfo?winInfo->mask:-1);
        return 0;
    }
    LOG(LOG_LEVEL_DEBUG,"activating window %d in workspace %d\n",winInfo->id,winInfo->workspaceIndex);
    switchToWorkspace(winInfo->workspaceIndex);
    return raiseWindowInfo(winInfo) && focusWindowInfo(winInfo)?winInfo->id:0;
}

static void focusNextVisibleWindow(ArrayList*masters,WindowInfo*defaultWinInfo){
    Master*active=getActiveMaster();
    FOR_EACH(Master*master,masters,
        ArrayList* masterWindowStack=getWindowStackByMaster(master);
        setActiveMaster(master);
        WindowInfo*winToFocus=NULL;
        UNTIL_FIRST(winToFocus,masterWindowStack,isWorkspaceVisible(getWorkspaceOfWindow(winToFocus)->id) && isActivatable(winToFocus)&&focusWindowInfo(winToFocus));
        if (!winToFocus && defaultWinInfo)
            focusWindowInfo(defaultWinInfo);
    );
    setActiveMaster(active);
}

int deleteWindow(xcb_window_t winToRemove){
    ArrayList mastersFocusedOnWindow={0};
    FOR_EACH(Master*master,getAllMasters(),
        if(getFocusedWindowByMaster(master) && getFocusedWindowByMaster(master)->id==winToRemove)
            addToList(&mastersFocusedOnWindow,master););
    int result = removeWindow(winToRemove);
    focusNextVisibleWindow(&mastersFocusedOnWindow,isNotEmpty(getActiveWindowStack())?getHead(getActiveWindowStack()):NULL);
    clearList(&mastersFocusedOnWindow);
            
    if(result)
        updateEWMHClientList();
    return result;
}

int attemptToMapWindow(int id){
    WindowInfo* winInfo=getWindowInfo(id);
    if(!winInfo || isWindowInVisibleWorkspace(winInfo) && isMappable(winInfo))
        return catchError(xcb_map_window_checked(dis, id));
    return 0;
}

static void updateWindowWorkspaceState(WindowInfo*winInfo, int workspaceIndex,int swappedWith){
    if(isWorkspaceVisible(workspaceIndex)){
        attemptToMapWindow(winInfo->id);
    }
    else{
        if(hasMask(winInfo,STICKY_MASK)){
            LOG(LOG_LEVEL_DEBUG,"Moving sticky window %d to workspace %d\n",winInfo->id,swappedWith);
            if(swappedWith!=-1)
                moveWindowToWorkspace(winInfo,swappedWith);
            return;
        }
        catchError(xcb_unmap_window_checked(dis, winInfo->id));
        ArrayList masters={0};
        FOR_EACH(Master*master,getAllMasters(),
            if(getFocusedWindowByMaster(master)==winInfo)
                addToList(&masters,master);
        );
        focusNextVisibleWindow(&masters,NULL);
        clearList(&masters);
    }
}

void swapWorkspaces(int index1,int index2){
    swapMonitors(index1,index2);
    if(isWorkspaceVisible(index1)!=isWorkspaceVisible(index2)){
        ArrayList*stack=getWindowStack(getWorkspaceByIndex(index1));
        FOR_EACH_REVERSED(WindowInfo*winInfo,stack,updateWindowWorkspaceState(winInfo,index1,index2));
        stack=getWindowStack(getWorkspaceByIndex(index2));
        FOR_EACH_REVERSED(WindowInfo*winInfo,stack,updateWindowWorkspaceState(winInfo,index2,index1));
    }
}
void swapWithWorkspace(int workspaceIndex){
    swapWorkspaces(getActiveWorkspaceIndex(),workspaceIndex);
}
void activateWorkspace(int workspaceIndex){
    switchToWorkspace(workspaceIndex);
    ArrayList* masterWindowStack=getWindowStackByMaster(getActiveMaster());
    WindowInfo*winToFocus=NULL;
    UNTIL_FIRST(winToFocus,masterWindowStack,workspaceIndex==getWorkspaceOfWindow(winToFocus)->id && isActivatable(winToFocus));
    LOG(LOG_LEVEL_DEBUG,"Activating window\n");
    if(winToFocus)
        activateWindow(winToFocus);
    else if (isNotEmpty(getActiveWindowStack()))
        activateWindow(getHead(getActiveWindowStack()));
    LOG(LOG_LEVEL_DEBUG,"Done\n");
}
void switchToWorkspace(int workspaceIndex){
    if(!isWorkspaceVisible(workspaceIndex)){
        int currentIndex=getActiveWorkspaceIndex();
        LOG(LOG_LEVEL_DEBUG,"Switching to workspace %d:\n",workspaceIndex);
        /*
         * Each master can have independent active workspaces. While an active workspace is generally visible when it is set,
         * it can become invisible due to another master switching workspaces. So when the master in the invisible workspaces
         * wants its workspace to become visbile it must first find a visible workspace to swap with
         */
        if(!isWorkspaceVisible(currentIndex)){
            currentIndex=getNextWorkspace(1, VISIBLE)->id;
        }
        LOG(LOG_LEVEL_DEBUG,"Swaping visible workspace %d with %d\n",currentIndex,workspaceIndex);
        //we need to map new windows
        swapWorkspaces(workspaceIndex,currentIndex);
        updateEWMHWorkspaceProperties();
        xcb_ewmh_set_current_desktop_checked(
                    ewmh, DefaultScreen(dpy), workspaceIndex);
    }
    setActiveWorkspaceIndex(workspaceIndex);
}


void moveWindowToWorkspace(WindowInfo* winInfo,int destIndex){
    
    assert(destIndex==-1 || destIndex>=0 && destIndex<getNumberOfWorkspaces());
    assert(winInfo);
    if(destIndex==-1){
        addMask(winInfo,STICKY_MASK);
        destIndex=getActiveWorkspaceIndex();
    }
    removeWindowFromWorkspace(winInfo);
    addWindowToWorkspace(winInfo, destIndex);
    updateWindowWorkspaceState(winInfo, destIndex, -1);

    xcb_ewmh_set_wm_desktop(ewmh, winInfo->id, destIndex);

    LOG(LOG_LEVEL_TRACE,"window %d added to workspace %d visible %d\n",winInfo->id,destIndex,isWorkspaceVisible(destIndex));
}


//window methods

int setBorderColor(xcb_window_t win,unsigned int color){
    xcb_void_cookie_t c;
    if(color>0xFFFFFF){
        LOG(LOG_LEVEL_WARN,"Color %d is out f range\n",color);
        return 0;
    }
    c=xcb_change_window_attributes_checked(dis, win, XCB_CW_BORDER_PIXEL,&color);
    LOG(LOG_LEVEL_TRACE,"setting border for window %d to %d\n",win,color);
    return !catchError(c);
}
int setBorder(WindowInfo* winInfo){
    return setBorderColor(winInfo->id,getActiveMaster()->focusColor);
}
int resetBorder(WindowInfo*winInfo){
    Master*master=getLastMasterToFocusWindow(winInfo->id);
    if(master)
        return setBorderColor(winInfo->id,master->focusColor);
    return setBorderColor(winInfo->id,DEFAULT_UNFOCUS_BORDER_COLOR);
}


static int filterConfigValues(int*filteredArr,WindowInfo*winInfo,short values[5],xcb_window_t sibling,int stackMode,int configMask){
    int i=0;
    int n=0;
    if(configMask & XCB_CONFIG_WINDOW_X && ++n && isExternallyMoveable(winInfo))
        filteredArr[i++]=values[n-1];
    else configMask&=~XCB_CONFIG_WINDOW_X;
    if(configMask & XCB_CONFIG_WINDOW_Y && ++n && isExternallyMoveable(winInfo))
            filteredArr[i++]=values[n-1];
    else configMask&=~XCB_CONFIG_WINDOW_Y;
    if(configMask & XCB_CONFIG_WINDOW_WIDTH && ++n && isExternallyResizable(winInfo))
        filteredArr[i++]=values[n-1];
    else configMask&=~XCB_CONFIG_WINDOW_WIDTH;
    if(configMask & XCB_CONFIG_WINDOW_HEIGHT && ++n && isExternallyResizable(winInfo))
        filteredArr[i++]=values[n-1];
    else configMask&=~XCB_CONFIG_WINDOW_HEIGHT;
    if(configMask & XCB_CONFIG_WINDOW_BORDER_WIDTH && ++n && isExternallyBorderConfigurable(winInfo))
        filteredArr[i++]=values[n-1];
    else configMask&=~XCB_CONFIG_WINDOW_BORDER_WIDTH;

    if(configMask & XCB_CONFIG_WINDOW_SIBLING && ++n && isExternallyRaisable(winInfo))
        filteredArr[i++]=sibling;
    else configMask&=~XCB_CONFIG_WINDOW_SIBLING;
    if(configMask & XCB_CONFIG_WINDOW_STACK_MODE && ++n && isExternallyRaisable(winInfo))
        filteredArr[i++]=stackMode;
    else configMask&=~XCB_CONFIG_WINDOW_STACK_MODE;
    return configMask;
}
void processConfigureRequest(int win,short values[5],xcb_window_t sibling,int stackMode,int configMask){
    LOG(LOG_LEVEL_TRACE,"processing configure request window %d\n",win);
    int actualValues[7];
    WindowInfo*winInfo=getWindowInfo(win);
    configMask=filterConfigValues(actualValues,winInfo,values,sibling,stackMode,configMask);

    if(configMask){
        xcb_configure_window(dis, win, configMask, actualValues);
        LOG(LOG_LEVEL_TRACE,"re-configure window %d configMask: %d\n",win,configMask);
    }
    else
        LOG(LOG_LEVEL_DEBUG,"configure request denied for window %d; configMasks %d\n",win,winInfo?winInfo->mask:-1);
}
void broadcastEWMHCompilence(){
    LOG(LOG_LEVEL_TRACE,"Compliying with EWMH\n");
    SET_SUPPORTED_OPERATIONS(ewmh);

    //functionless window required by EWMH spec
    //we set its class to input only and set override redirect so we (and anyone else  ignore it)
    int overrideRedirect = 1;
    xcb_window_t checkwin = xcb_generate_id(dis);
    xcb_create_window(dis, 0, checkwin, root, 0, 0, 1, 1, 0,
                      XCB_WINDOW_CLASS_INPUT_ONLY, 0, XCB_CW_OVERRIDE_REDIRECT, &overrideRedirect);
    xcb_ewmh_set_supporting_wm_check(ewmh, root, checkwin);
    xcb_ewmh_set_wm_name(ewmh, checkwin, strlen(WM_NAME), WM_NAME);

    
    xcb_get_selection_owner_reply_t* ownerReply=xcb_get_selection_owner_reply(dis,xcb_get_selection_owner(dis, WM_SELECTION_ATOM),NULL);
    if(ownerReply->owner){
        LOG(LOG_LEVEL_ERROR,"Selection %d is already owned by window %d\n",WM_SELECTION_ATOM,ownerReply->owner);
        exit(1);
    }
    
    if(catchError(xcb_set_selection_owner_checked(dis, checkwin, WM_SELECTION_ATOM, XCB_CURRENT_TIME))==0){
        xcb_client_message_event_t ev={0};
        ev.response_type = XCB_CLIENT_MESSAGE;
        ev.format = 32;
        ev.window = checkwin;
        ev.type = ewmh->WM_PROTOCOLS;
        ev.data.data32[0] = getTime();
        ev.data.data32[1] = WM_SELECTION_ATOM;
        ev.data.data32[2] = checkwin;
        xcb_send_event(dis, 0, root, XCB_EVENT_MASK_STRUCTURE_NOTIFY, (char *)&ev);
    }
    free(ownerReply);
    LOG(LOG_LEVEL_TRACE,"ewmh initilized\n");
}

void updateMapState(int id,int map){
    WindowInfo*winInfo=getWindowInfo(id);
    if(winInfo)
        if(map){
            loadWindowHints(winInfo);
            addMask(winInfo, MAPPED_MASK);
            if(winInfo->mappedBefore)return;
            else winInfo->mappedBefore=1;
            long delta=getTime()-winInfo->creationTime;
            if(delta<AUTO_FOCUS_NEW_WINDOW_TIMEOUT)
                if(focusWindowInfo(winInfo)){
                    LOG(LOG_LEVEL_DEBUG,"auto focusing window %d (Detected %ldms ago)\n",winInfo->id,delta);
                    updateFocusState(winInfo);
                    raiseWindowInfo(winInfo);
                }
        }
        else
            removeMask(winInfo, MAPPED_MASK);
}

void updateFocusState(WindowInfo*winInfo){
    if(!winInfo)return;
    onWindowFocus(winInfo->id);
}

char*getWorkspaceName(int index){
    return getWorkspaceByIndex(index)->name;
}

int getIndexFromName(char*name){
    for(int i=0;i<getNumberOfWorkspaces();i++){
        if(getWorkspaceByIndex(i)->name && strcmp(name, getWorkspaceByIndex(i)->name)==0)
            return i;
    }
    return getNumberOfWorkspaces();
}
void setWorkspaceNames(char*names[],int numberOfNames){
    for(int i=0;i<numberOfNames && i<getNumberOfWorkspaces();i++)
        getWorkspaceByIndex(i)->name=names[i];
    if(ewmh)
        xcb_ewmh_set_desktop_names(ewmh, defaultScreenNumber, numberOfNames, (char*)names);
}

void applyGravity(int win,short pos[5],int gravity){

    if(gravity==XCB_GRAVITY_STATIC)
        return;
    if(!gravity){
        xcb_size_hints_t hints;
        int result=xcb_icccm_get_wm_normal_hints_reply(dis, xcb_icccm_get_wm_normal_hints(dis, win), &hints, NULL);
        if(result)
            gravity=hints.win_gravity;
        else return;
    }
    switch(gravity){
        case XCB_GRAVITY_NORTH:
        case XCB_GRAVITY_NORTH_EAST:
        case XCB_GRAVITY_NORTH_WEST:
            pos[1]-=pos[4];
            break;
        case XCB_GRAVITY_SOUTH:
        case XCB_GRAVITY_SOUTH_EAST:
        case XCB_GRAVITY_SOUTH_WEST:
            pos[1]+=pos[4]+pos[3];
            break;
        default:
            pos[1]+=pos[3]/2;
    }
    switch(gravity){
        case XCB_GRAVITY_SOUTH_WEST:
        case XCB_GRAVITY_NORTH_WEST:
        case XCB_GRAVITY_WEST:
            pos[0]-=pos[4];
            break;
        case XCB_GRAVITY_SOUTH_EAST:
        case XCB_GRAVITY_NORTH_EAST:
        case XCB_GRAVITY_EAST:
            pos[0]+=pos[2]+pos[4];
            break;
        default:
            pos[0]+=pos[2]/2;
    }
}
void swapWindows(WindowInfo*winInfo1,WindowInfo*winInfo2){
    int index1=indexOf(getWindowStackOfWindow(winInfo1),winInfo1,sizeof(int));
    int index2=indexOf(getWindowStackOfWindow(winInfo2),winInfo2,sizeof(int));
    setElement(getWindowStackOfWindow(winInfo1),index1,winInfo2);
    setElement(getWindowStackOfWindow(winInfo2),index2,winInfo1);
    int temp=winInfo1->workspaceIndex;
    winInfo1->workspaceIndex=winInfo2->workspaceIndex;
    winInfo2->workspaceIndex=temp;
}
