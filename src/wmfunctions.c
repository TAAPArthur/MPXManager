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
#include "devices.h"
#include "logger.h"
#include "xsession.h"
#include "monitors.h"
#include "globals.h"
#include "events.h"
#include "layouts.h"

static Node initialMappingOrder;

int isWindowVisible(WindowInfo* winInfo){
    return winInfo && hasMask(winInfo, PARTIALLY_VISIBLE);
}

int isMappable(WindowInfo* winInfo){
    return winInfo && hasMask(winInfo, MAPABLE_MASK);
}
int isMapped(WindowInfo* winInfo){
    return winInfo && hasMask(winInfo, MAPPED_MASK);
}
int isInteractable(WindowInfo* winInfo){
    return isMapped(winInfo) && !(winInfo->mask & HIDDEN_MASK);
}
int isTileable(WindowInfo*winInfo){
    return isInteractable(winInfo) && !hasPartOfMask(winInfo, ALL_NO_TILE_MASKS);
}
int isActivatable(WindowInfo* winInfo){
    return !winInfo || winInfo->mask & MAPABLE_MASK && !(winInfo->mask & HIDDEN_MASK);
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
    syncState();
    registerForEvents();
    if(applyRules(getEventRules(onXConnection),NULL)){
        broadcastEWMHCompilence();
        dumpAllWindowInfo();
        printSummary();
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
                ((int*)&workAreas[i])[n]=(&getMonitorFromWorkspace(getWorkspaceByIndex(i))->viewX)[n];
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
    Node*n=getAllWindows();
    int i=0;
    FOR_EACH(n,
            stackingOrder[num-++i]=getIntValue(n);
    )
    i=0;
    n=&initialMappingOrder;
    FOR_EACH(n,
            mappingOrder[num-++i]=getIntValue(n);
    )

    xcb_ewmh_set_client_list(ewmh, defaultScreenNumber, num, mappingOrder);
    xcb_ewmh_set_client_list_stacking(ewmh, defaultScreenNumber, num, stackingOrder);
}




int getSavedWorkspaceIndex(xcb_window_t win){
    unsigned int workspaceIndex=0;

    if ((xcb_ewmh_get_wm_desktop_reply(ewmh,
              xcb_ewmh_get_wm_desktop(ewmh, win), &workspaceIndex, NULL))) {
        if(workspaceIndex>=getNumberOfWorkspaces())
            workspaceIndex=getNumberOfWorkspaces()-1;
    }
    else workspaceIndex = getActiveWorkspaceIndex();
    return workspaceIndex;
}

void* waitForWindowToDie(int id){
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
        Node*n=getAllWindows();
        FOR_EACH(n,
            if(isInteractable(getValue(n))&&((WindowInfo*)getValue(n))->transientFor==winInfo->id)
                raiseLowerWindowInfo(getValue(n),1);
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

int deleteWindow(xcb_window_t winToRemove){
    int result = removeWindow(winToRemove);
    if(result)
        updateEWMHClientList();
    return result;
}

int attemptToMapWindow(int id){
    WindowInfo* winInfo=getValue(isInList(getAllWindows(), id));
    LOG(LOG_LEVEL_TRACE,"Mappable status %d %d %d\n",id,isWindowInVisibleWorkspace(winInfo),isMappable(winInfo));
    if(!winInfo || isWindowInVisibleWorkspace(winInfo) && isMappable(winInfo))
        return catchError(xcb_map_window_checked(dis, id));
    return 0;
}

static void updateWindowWorkspaceState(WindowInfo*winInfo, int workspaceIndex,int map){
    if(map){
        attemptToMapWindow(winInfo->id);
        winInfo->workspaceIndex=workspaceIndex;
    }
    else{
        catchError(xcb_unmap_window_checked(dis, winInfo->id));
        Master*active=getActiveMaster();
        Node*master=getLast(getAllMasters());
        FOR_EACH_REVERSED(master,
            Node* masterWindowStack=getWindowStackByMaster(getValue(master));
            if(getValue(masterWindowStack)==winInfo){
                setActiveMaster(getValue(master));
                UNTIL_FIRST(masterWindowStack,isWorkspaceVisible(getWorkspaceOfWindow(getValue(masterWindowStack))->id));
                if(masterWindowStack)
                    focusWindowInfo(getValue(masterWindowStack));
            }
        );
        setActiveMaster(active);
    }
}
static void setWorkspaceState(int workspaceIndex,int map){
    LOG(LOG_LEVEL_DEBUG,"Setting workspace %d to %d:\n",workspaceIndex,map);
    Node*stack=getWindowStack(getWorkspaceByIndex(workspaceIndex));
    FOR_EACH(stack,updateWindowWorkspaceState(getValue(stack),workspaceIndex,map))
}
void switchToWorkspace(int workspaceIndex){
    if(isWorkspaceVisible(workspaceIndex)){
        LOG(LOG_LEVEL_TRACE,"Workspace is already visible %d:\n",workspaceIndex);
        return;
    }
    int currentIndex=getActiveWorkspaceIndex();
    LOG(LOG_LEVEL_DEBUG,"Switching to workspace %d:\n",workspaceIndex);
    /*
     * Each master can have independent active workspaces. While an active workspace is generally visible when it is set,
     * it can become invisible due to another master switching workspaces. So when the master in the invisible workspaces
     * wants its workspace to become visbile it must first find a visible workspace to swap with
     */
    if(currentIndex==workspaceIndex){
        currentIndex=getNextWorkspace(1, VISIBLE)->id;
    }
    assert(isWorkspaceVisible(currentIndex));
    //we need to map new windows
    LOG(LOG_LEVEL_DEBUG,"Swaping visible workspace %d with %d\n",currentIndex,workspaceIndex);
    swapMonitors(workspaceIndex,currentIndex);
    setWorkspaceState(workspaceIndex,1);
    setWorkspaceState(currentIndex,0);

    setActiveWorkspaceIndex(workspaceIndex);
    updateEWMHWorkspaceProperties();
    xcb_ewmh_set_current_desktop_checked(
                ewmh, DefaultScreen(dpy), workspaceIndex);
}


void moveWindowToWorkspace(WindowInfo* winInfo,int destIndex){
    assert(destIndex>=0 && destIndex<getNumberOfWorkspaces());
    assert(winInfo);

    removeWindowFromAllWorkspaces(winInfo);
    addWindowToWorkspace(winInfo, destIndex);
    updateWindowWorkspaceState(winInfo, destIndex, isWorkspaceVisible(destIndex));

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


int filterConfigValues(int*filteredArr,WindowInfo*winInfo,short values[5],xcb_window_t sibling,int stackMode,int configMask){
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

    /*
    char selectionName[]={'W','M','_','S','0'+defaultScreenNumber,'\0'};


    xcb_intern_atom_reply_t* reply=xcb_intern_atom_reply(dis,
            xcb_intern_atom(dis, 0, strlen(selectionName), selectionName),NULL);
    assert(reply);
    xcb_set_selection_owner(dis, checkwin, reply->atom, XCB_CURRENT_TIME);
    free(reply);
    */
    LOG(LOG_LEVEL_TRACE,"ewmh initilized\n");
}

void updateMapState(int id,int map){
    WindowInfo*winInfo=getWindowInfo(id);
    if(winInfo)
        if(map){
            loadWindowHints(winInfo);
            addMask(winInfo, MAPPED_MASK);
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
    xcb_ewmh_set_desktop_names(ewmh, defaultScreenNumber, numberOfNames, (char*)names);
}
int isShowingDesktop(int index){
    return getWorkspaceByIndex(index)->showingDesktop;
}
void setShowingDesktop(int index,int value){
    getWorkspaceByIndex(index)->showingDesktop=value;
    setWorkspaceState(getActiveWorkspaceIndex(),!value);
    xcb_ewmh_set_showing_desktop(ewmh, defaultScreenNumber, value);
}
void toggleShowDesktop(){
    int hideDesktop=isShowingDesktop(getActiveWorkspaceIndex());
    setShowingDesktop(getActiveWorkspaceIndex(),!hideDesktop);
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
