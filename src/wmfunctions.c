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
#include "devices.h"
#include "logger.h"
#include "xsession.h"
#include "monitors.h"
#include "globals.h"
#include "events.h"

#define APPLY_RULES_OR_RETURN(TYPE,winInfo) \
        if(!applyRules(eventRules[TYPE],winInfo))\
            return;
static Node initialMappingOrder;


int isWindowVisible(WindowInfo* winInfo){
    return winInfo && hasMask(winInfo, PARTIALLY_VISIBLE);
}

int isMappable(WindowInfo* winInfo){
    return winInfo && hasMask(winInfo, MAPABLE_MASK);
}
int isTileable(WindowInfo*winInfo){
    return isMappable(winInfo) && !hasMask(winInfo, NO_TILE_MASK);
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

int isActivatable(WindowInfo* winInfo){
    return !winInfo || winInfo->mask & MAPABLE_MASK && !(winInfo->mask & HIDDEN_MASK);
}
int hasMask(WindowInfo* winInfo,int mask){
    return (winInfo->mask & mask) == mask;
}
void resetUserMask(WindowInfo* winInfo){
    if(winInfo)
        memcpy(&winInfo->mask, &DEFAULT_WINDOW_MASKS, sizeof(char));
}
int isUserMask(int mask){
    return ((char)mask)?1:0;
}

void toggleMask(WindowInfo*winInfo,int mask){
    if(hasMask(winInfo,mask))
        removeMask(winInfo,mask);
    else
        addMask(winInfo,mask);
}
void addMask(WindowInfo*winInfo,int mask){
    winInfo->mask|=mask;
    if(isUserMask(mask))
        setXWindowStateFromMask(winInfo);
}
void removeMask(WindowInfo*winInfo,int mask){
    winInfo->mask&=~mask;
    if(isUserMask(mask))
        setXWindowStateFromMask(winInfo);
}

void connectToXserver(){

    openXDisplay();

    initCurrentMasters();
    assert(getActiveMaster()!=NULL);
    detectMonitors();
    syncState();
    registerForEvents();
    APPLY_RULES_OR_RETURN(onXConnection,NULL);
    broadcastEWMHCompilence();

}
void syncState(){
    xcb_ewmh_set_showing_desktop(ewmh, defaultScreenNumber, 0);
    unsigned int currentWorkspace=DEFAULT_WORKSPACE_INDEX;
    if(LOAD_SAVED_STATE)
        if(!xcb_ewmh_get_current_desktop_reply(ewmh,
                xcb_ewmh_get_current_desktop(ewmh, defaultScreenNumber),
                &currentWorkspace, NULL)){
            currentWorkspace=DEFAULT_WORKSPACE_INDEX;
        }
    if(currentWorkspace>=getNumberOfWorkspaces())
        currentWorkspace=getNumberOfWorkspaces()-1;
    xcb_ewmh_set_number_of_desktops(ewmh, defaultScreenNumber, getNumberOfWorkspaces());
    LOG(LOG_LEVEL_INFO,"Current workspace is %d; default %d\n\n",currentWorkspace,DEFAULT_WORKSPACE_INDEX);
    activateWorkspace(currentWorkspace);
}

void scan(xcb_window_t baseWindow) {
    LOG(LOG_LEVEL_DEBUG,"Scanning children of %d\n",baseWindow);
    xcb_query_tree_reply_t *reply;
    reply = xcb_query_tree_reply(dis, xcb_query_tree(dis, baseWindow), 0);
    assert(reply && "could not query tree");
    if (reply) {
        int numberOfChildren = xcb_query_tree_children_length(reply);
        LOG(LOG_LEVEL_TRACE,"detected %d kids\n",numberOfChildren);
        xcb_window_t *children = xcb_query_tree_children(reply);
        xcb_get_window_attributes_reply_t *attr;
        xcb_get_window_attributes_cookie_t cookies[numberOfChildren];
        for (int i = 0; i < numberOfChildren; i++)
            cookies[i]=xcb_get_window_attributes(dis, children[i]);

        for (int i = 0; i < numberOfChildren; i++) {
            LOG(LOG_LEVEL_DEBUG,"processing child %d\n",children[i]);
            attr = xcb_get_window_attributes_reply(dis,cookies[i], NULL);

            assert(attr);
            if(attr->override_redirect || attr->_class ==XCB_WINDOW_CLASS_INPUT_ONLY){
                LOG(LOG_LEVEL_TRACE,"Skipping child override redirect: %d class: %d\n",attr->override_redirect,attr->_class);
            }
            else {
                WindowInfo *winInfo=createWindowInfo(children[i]);
                //if the window is not unmapped
                if(attr->map_state)
                    addMask(winInfo, MAPABLE_MASK);

                if(processNewWindow(winInfo))
                    scan(children[i]);
            }
            free(attr);
        }
        free(reply);
    }
}
void setActiveWindowProperty(int win){
    xcb_ewmh_set_active_window(ewmh, defaultScreenNumber, win);
}
void updateEWMHWorkspaceProperties(){
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
int processNewWindow(WindowInfo* winInfo){
    LOG(LOG_LEVEL_DEBUG,"processing %d (%x)\n",winInfo->id,(unsigned int)winInfo->id);

    if(winInfo->cloneOrigin==0)
        loadWindowProperties(winInfo);
    //dumpWindowInfo(winInfo);
    if(!applyRules(eventRules[ProcessingWindow],winInfo)){
        LOG(LOG_LEVEL_DEBUG,"Window is to be ignored; freeing winInfo\n");
        deleteWindowInfo(winInfo);
        return 0;
    }
    LOG(LOG_LEVEL_DEBUG,"Registering window\n");
    registerWindow(winInfo);
    updateEWMHClientList();
    return 1;
}


void loadClassInfo(WindowInfo*info){
    int win=info->id;
    xcb_icccm_get_wm_class_reply_t prop;
    xcb_get_property_cookie_t cookie = xcb_icccm_get_wm_class(dis, win);
    if(!xcb_icccm_get_wm_class_reply(dis,cookie, &prop, NULL))
        return;
    if(info->className)
        free(info->className);
    if(info->instanceName)
        free(info->instanceName);

    info->className=calloc(1+(strlen(prop.class_name)),sizeof(char));
    info->instanceName=calloc(1+(strlen(prop.instance_name)),sizeof(char));

    strcpy(info->className, prop.class_name);
    strcpy(info->instanceName, prop.instance_name);
    xcb_icccm_get_wm_class_reply_wipe(&prop);
    LOG(LOG_LEVEL_TRACE,"class name %s instance name: %s \n",info->className,info->instanceName);

}
void loadTitleInfo(WindowInfo*winInfo){
    xcb_ewmh_get_utf8_strings_reply_t wtitle;
    xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_name(ewmh, winInfo->id);
    if(winInfo->title){
        free(winInfo->title);
        winInfo->title=NULL;
    }
    if (xcb_ewmh_get_wm_name_reply(ewmh, cookie, &wtitle, NULL)) {
        winInfo->title=calloc(1+wtitle.strings_len,sizeof(char));
        memcpy(winInfo->title, wtitle.strings, wtitle.strings_len*sizeof(char));
        xcb_ewmh_get_utf8_strings_reply_wipe(&wtitle);
    }
    else {
        xcb_icccm_get_text_property_reply_t icccmName;
        cookie =xcb_icccm_get_wm_name(dis, winInfo->id);
        if (xcb_icccm_get_wm_name_reply(dis, cookie, &icccmName, NULL)) {
            winInfo->title=calloc(1+icccmName.name_len,sizeof(char));
            memcpy(winInfo->title, icccmName.name, icccmName.name_len*sizeof(char));
            xcb_icccm_get_text_property_reply_wipe(&icccmName);
        }
    }

    if(winInfo->title)
        LOG(LOG_LEVEL_TRACE,"window title %s\n",winInfo->title);
}
void loadWindowType(WindowInfo *winInfo){
    xcb_ewmh_get_atoms_reply_t name;
    if(xcb_ewmh_get_wm_window_type_reply(ewmh,
            xcb_ewmh_get_wm_window_type(ewmh, winInfo->id), &name, NULL)){
        winInfo->type=name.atoms[0];
        xcb_ewmh_get_atoms_reply_wipe(&name);
    }
    else{
        winInfo->implicitType=1;
        winInfo->type=winInfo->transientFor?ewmh->_NET_WM_WINDOW_TYPE_DIALOG:ewmh->_NET_WM_WINDOW_TYPE_NORMAL;
    }

    xcb_get_atom_name_reply_t *reply=xcb_get_atom_name_reply(
            dis, xcb_get_atom_name(dis, winInfo->type), NULL);

    if(!reply)return;
    if(winInfo->typeName)
        free(winInfo->typeName);

    winInfo->typeName=calloc(reply->name_len+1,sizeof(char));

    memcpy(winInfo->typeName, xcb_get_atom_name_name(reply), reply->name_len*sizeof(char));

    LOG(LOG_LEVEL_TRACE,"window type %s\n",winInfo->typeName);
    free(reply);
}
void loadWindowHints(WindowInfo *winInfo){
    assert(winInfo);
    xcb_icccm_wm_hints_t hints;
    if(xcb_icccm_get_wm_hints_reply(dis, xcb_icccm_get_wm_hints(dis, winInfo->id), &hints, NULL)){
        winInfo->groupId=hints.window_group;
        if(hints.input)
            addMask(winInfo, INPUT_MASK);
        if(hints.initial_state == XCB_ICCCM_WM_STATE_NORMAL)
            addMask(winInfo, MAPABLE_MASK);
    }
}

void loadProtocols(WindowInfo *winInfo){
    xcb_icccm_get_wm_protocols_reply_t reply;
    if(xcb_icccm_get_wm_protocols_reply(dis,
                xcb_icccm_get_wm_protocols(dis, winInfo->id, ewmh->WM_PROTOCOLS),
                &reply, NULL)){
        for(int i=0;i<reply.atoms_len;i++)
            if(reply.atoms[i]==WM_DELETE_WINDOW)
                addMask(winInfo, WM_DELETE_WINDOW_MASK);
            else if(reply.atoms[i]==ewmh->_NET_WM_PING)
                addMask(winInfo, WM_PING_MASK);
            //else if(reply.atoms[i]==WM_TAKE_FOCUS)
                //addMask(winInfo, WM_TAKE_FOCUS_MASK);

        xcb_icccm_get_wm_protocols_reply_wipe(&reply);
    }
}
void loadWindowProperties(WindowInfo *winInfo){

    LOG(LOG_LEVEL_TRACE,"loading window properties %d\n",winInfo->id);
    loadClassInfo(winInfo);
    loadTitleInfo(winInfo);
    loadWindowHints(winInfo);
    loadProtocols(winInfo);

    xcb_window_t prop;
    if(xcb_icccm_get_wm_transient_for_reply(dis,
            xcb_icccm_get_wm_transient_for(dis, winInfo->id), &prop, NULL)){
        winInfo->transientFor=prop;
    }
    loadWindowType(winInfo);
    //dumpWindowInfo(winInfo);
}

void registerWindow(WindowInfo*winInfo){
    assert(winInfo);
    assert(!isInList(getAllWindows(), winInfo->id) && "Window registered exists" );
    addWindowInfo(winInfo);

    winInfo->workspaceIndex=LOAD_SAVED_STATE?getSavedWorkspaceIndex(winInfo->id):getActiveWorkspaceIndex();
    addMask(winInfo, DEFAULT_WINDOW_MASKS);
    Window win=winInfo->id;
    assert(win);
    if(LOAD_SAVED_STATE){
        xcb_ewmh_get_atoms_reply_t reply;
        if(xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, win), &reply, NULL)){
            setWindowStateFromAtomInfo(winInfo,reply.atoms,reply.atoms_len,XCB_EWMH_WM_STATE_ADD);
            xcb_ewmh_get_atoms_reply_wipe(&reply);
        }
    }
    //todo move to own method
    registerForWindowEvents(win, NON_ROOT_EVENT_MASKS);
    passiveGrab(win, NON_ROOT_DEVICE_EVENT_MASKS);

    moveWindowToWorkspaceAtLayer(winInfo, winInfo->workspaceIndex,NORMAL_LAYER);
    APPLY_RULES_OR_RETURN(RegisteringWindow,winInfo);
    char state=XCB_ICCCM_WM_STATE_WITHDRAWN;
    xcb_change_property(dis,XCB_PROP_MODE_REPLACE,win,ewmh->_NET_WM_STATE,ewmh->_NET_WM_STATE,32,0,&state);
}

typedef struct {
    int mask;
    int layer;
}WindowState;


int getLastLayerForWindow(WindowInfo*winInfo){
    Workspace*workspace=getWorkspaceByIndex(winInfo->workspaceIndex);
    for(int i=0;i<NUMBER_OF_LAYERS;i++)
        if(isInList(getWindowStackAtLayer(workspace, i),winInfo->id))
            return i;
    return -1;

}

void setXWindowStateFromMask(WindowInfo*winInfo){

    xcb_atom_t supportedStates[]={SUPPORTED_STATES};

    xcb_ewmh_get_atoms_reply_t reply;
    int count=0;
    int hasState=xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, winInfo->id), &reply, NULL);
    xcb_atom_t windowState[LEN(supportedStates)+(hasState?reply.atoms_len:0)];
    if(hasState){
        for(int i=0;i<reply.atoms_len;i++){
            char isSupportedState=0;
            for(int n=0;n<LEN(supportedStates);n++)
                if(supportedStates[n]==reply.atoms[i]){
                    isSupportedState=1;
                    break;
                }
            if(!isSupportedState)
                windowState[count++]=reply.atoms[i];
        }
        xcb_ewmh_get_atoms_reply_wipe(&reply);
    }
    if(hasMask(winInfo, STICKY_MASK))
        windowState[count++]=ewmh->_NET_WM_STATE_STICKY;
    if(hasMask(winInfo, HIDDEN_MASK))
        windowState[count++]=ewmh->_NET_WM_STATE_HIDDEN;
    if(hasMask(winInfo, Y_MAXIMIZED_MASK))
        windowState[count++]=ewmh->_NET_WM_STATE_MAXIMIZED_VERT;
    if(hasMask(winInfo, X_MAXIMIZED_MASK))
        windowState[count++]=ewmh->_NET_WM_STATE_MAXIMIZED_HORZ;
    if(hasMask(winInfo, URGENT_MASK))
        windowState[count++]=ewmh->_NET_WM_STATE_DEMANDS_ATTENTION;
    if(hasMask(winInfo, FULLSCREEN_MASK))
        windowState[count++]=ewmh->_NET_WM_STATE_FULLSCREEN;
    int layer=getLastLayerForWindow(winInfo);
    if(layer==UPPER_LAYER)
        windowState[count++]=ewmh->_NET_WM_STATE_ABOVE;
    if(layer==LOWER_LAYER)
        windowState[count++]=ewmh->_NET_WM_STATE_BELOW;
    xcb_ewmh_set_wm_state(ewmh, winInfo->id, count, windowState);
}

void setWindowStateFromAtomInfo(WindowInfo*winInfo, const xcb_atom_t* atoms,int numberOfAtoms,int action){
    if(!winInfo)
        return;

    LOG(LOG_LEVEL_TRACE,"Updating state of %d from %d atoms",winInfo->id,numberOfAtoms);

    int mask=0;
    int layer=NORMAL_LAYER;
    for (unsigned int i = 0; i < numberOfAtoms; i++) {
        if(atoms[i] == ewmh->_NET_WM_STATE_STICKY)
            mask|=STICKY_MASK;
        else if(atoms[i] == ewmh->_NET_WM_STATE_HIDDEN)
            mask|=HIDDEN_MASK;
        else if(atoms[i] == ewmh->_NET_WM_STATE_MAXIMIZED_VERT)
            mask|=Y_MAXIMIZED_MASK;
        else if(atoms[i] == ewmh->_NET_WM_STATE_MAXIMIZED_HORZ)
            mask|=X_MAXIMIZED_MASK;
        else if(atoms[i] == ewmh->_NET_WM_STATE_DEMANDS_ATTENTION)
            mask|=URGENT_MASK;
        else if(atoms[i] == ewmh->_NET_WM_STATE_FULLSCREEN)
            mask|=FULLSCREEN_MASK;
        else if(atoms[i] == ewmh->_NET_WM_STATE_ABOVE)
            layer=UPPER_LAYER;
        else if(atoms[i] == ewmh->_NET_WM_STATE_BELOW)
            layer=LOWER_LAYER;
        else if(atoms[i] == WM_TAKE_FOCUS)
            mask|=WM_TAKE_FOCUS_MASK;
        else if(atoms[i] == WM_DELETE_WINDOW)
            mask|=WM_DELETE_WINDOW_MASK;
    }
    WindowState state={mask,layer};
    if(action==XCB_EWMH_WM_STATE_TOGGLE){
        if(hasMask(winInfo, state.mask) &&getLastLayerForWindow(winInfo)==state.layer)
            action=XCB_EWMH_WM_STATE_REMOVE;
        else
            action=XCB_EWMH_WM_STATE_ADD;

    }
    if(action==XCB_EWMH_WM_STATE_REMOVE){
        if(state.layer!=NORMAL_LAYER)
            moveWindowToLayerForAllWorkspaces(winInfo, NORMAL_LAYER);
        removeMask(winInfo, state.mask);
    }
    else{
        if(state.layer!=NORMAL_LAYER)
            moveWindowToLayerForAllWorkspaces(winInfo, state.layer);
        addMask(winInfo, state.mask);
    }
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
    int wait=1;
    while(wait && !isShuttingDown()){
        xcb_ewmh_send_wm_ping(ewmh, id, 0);
        flush();
        msleep(KILL_TIMEOUT);
        if(isShuttingDown())
            continue;
        lock();
        WindowInfo*winInfo=getWindowInfo(id);
        if(!winInfo){
            LOG(LOG_LEVEL_DEBUG,"Window %d no longer exists\n", id);
            wait=0;
        }
        else if(getTime()-winInfo->pingTimeStamp>=KILL_TIMEOUT){
            LOG(LOG_LEVEL_DEBUG,"Window %d is not responsive; force killing\n", id);
            killWindow(id);
            wait=0;
        }
        unlock();
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
        if(hasMask(winInfo, WM_PING_MASK))
            runInNewThread((void *(*)(void *))waitForWindowToDie,(void*) winInfo->id,1);
    }
    else{
        killWindow(winInfo->id);
    }
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
    LOG(LOG_LEVEL_DEBUG,"Trying to set focus to %d\n",win);
    assert(win);
    xcb_void_cookie_t cookie=xcb_input_xi_set_focus_checked(dis, win, XCB_CURRENT_TIME, getActiveMaster()->id);
    if(catchError(cookie)){
        return 0;
    }

    //onWindowFocus(win);
    return 1;
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
    return raiseWindow(winInfo->id) && focusWindowInfo(winInfo)?winInfo->id:0;
}

int deleteWindow(xcb_window_t winToRemove){
    int result = removeWindow(winToRemove)||removeDock(winToRemove);
    updateEWMHClientList();
    return result;
}

int attemptToMapWindow(int id){
    WindowInfo* winInfo=getValue(isInList(getAllWindows(), id));
    LOG(LOG_LEVEL_DEBUG,"Mappable status %d %d\n\n\n",isWindowInVisibleWorkspace(winInfo),isMappable(winInfo));
    if(!winInfo || isWindowInVisibleWorkspace(winInfo) && isMappable(winInfo))
        return catchError(xcb_map_window_checked(dis, id));
    return 0;
}

static void updateWindowWorkspaceState(WindowInfo*winInfo, int workspaceIndex,int map){
    if(map){
        winInfo->activeWorkspaceCount++;
        assert(winInfo->activeWorkspaceCount);

        if(winInfo->activeWorkspaceCount==1){
            attemptToMapWindow(winInfo->id);
            winInfo->workspaceIndex=workspaceIndex;
        }
    }
    else{
        if(winInfo->activeWorkspaceCount)winInfo->activeWorkspaceCount--;
        if(winInfo->activeWorkspaceCount==0)
            catchError(xcb_unmap_window_checked(dis, winInfo->id));
    }
}
static void setLayerState(int workspaceIndex,int map,int layer){
    Node*stack=getWindowStackAtLayer(getWorkspaceByIndex(workspaceIndex), layer);
    FOR_EACH(stack,
            updateWindowWorkspaceState(getValue(stack),workspaceIndex,map))
}
static void setWorkspaceState(int workspaceIndex,int map){
    LOG(LOG_LEVEL_DEBUG,"Setting workspace %d  to %d:\n",workspaceIndex,map);
    for(int i=0;i<NUMBER_OF_LAYERS;i++)
        setLayerState(workspaceIndex,map,i);

}
void switchToWorkspace(int workspaceIndex){
    if(isWorkspaceVisible(workspaceIndex)){
        LOG(LOG_LEVEL_DEBUG,"Workspace is already visible %d:\n",workspaceIndex);
        return;
    }
    int currentIndex=getActiveWorkspaceIndex();
    LOG(LOG_LEVEL_DEBUG,"Switchting to workspace %d:\n",workspaceIndex);
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
void activateWorkspace(int workspaceIndex){

    LOG(LOG_LEVEL_DEBUG,"Activating workspace:%d\n",workspaceIndex);
    Workspace*workspaceToSwitchTo=getWorkspaceByIndex(workspaceIndex);

    Node*head=getMasterWindowStack();
    LOG(LOG_LEVEL_TRACE,"Finding first window of active master in workspace %d:\n",workspaceIndex);

    UNTIL_FIRST(head,((WindowInfo*)getValue(head))->workspaceIndex==workspaceIndex)
    if(!head){

        head=getWindowStack(workspaceToSwitchTo);
        LOG(LOG_LEVEL_TRACE,"Intersection was empty; using head of stack:%d\n",getIntValue(head));
    }


    if(head->value)
        activateWindow(getValue(head));
    else switchToWorkspace(workspaceIndex);

}

void floatWindow(WindowInfo* winInfo){
    addMask(winInfo, FLOATING_MASK);
    moveWindowToLayerForAllWorkspaces(winInfo, UPPER_LAYER);

}
void sinkWindow(WindowInfo* winInfo){
    removeMask(winInfo, FLOATING_MASK);
    moveWindowToLayerForAllWorkspaces(winInfo, NORMAL_LAYER);
}

void moveWindowToLayerForAllWorkspaces(WindowInfo* winInfo,int layer){
    for(int i=0;i<getNumberOfWorkspaces();i++)
        moveWindowToLayer(winInfo, i, layer);
}
void moveWindowToLayer(WindowInfo*winInfo,int workspaceIndex,int layer){
    if(removeWindowFromWorkspace(winInfo, workspaceIndex))
        addWindowToWorkspaceAtLayer(winInfo, workspaceIndex, layer);
}
void moveWindowToWorkspace(WindowInfo* winInfo,int destIndex){
    if(destIndex==-1){
        addMask(winInfo,STICKY_MASK);
        floatWindow(winInfo);
        xcb_ewmh_set_wm_desktop(ewmh, winInfo->id, destIndex);
    }
    else
        moveWindowToWorkspaceAtLayer(winInfo,destIndex,NORMAL_LAYER);
}
void moveWindowToWorkspaceAtLayer(WindowInfo *winInfo,int destIndex,int layer){
    assert(destIndex>=0 && destIndex<getNumberOfWorkspaces());
    assert(winInfo);

    removeWindowFromAllWorkspaces(winInfo);
    addWindowToWorkspaceAtLayer(winInfo, destIndex, layer);
    updateWindowWorkspaceState(winInfo, destIndex, isWorkspaceVisible(destIndex));

    xcb_ewmh_set_wm_desktop(ewmh, winInfo->id, destIndex);

    LOG(LOG_LEVEL_INFO,"window %d added to workspace %d at layer %d",winInfo->id,destIndex,layer);
}


//window methods

void setBorderColor(xcb_window_t win,unsigned int color){
    xcb_void_cookie_t c;
    c=xcb_change_window_attributes_checked(dis, win, XCB_CW_BORDER_PIXEL,&color);
    catchError(c);
}

static void tileNonNormalLayers(WindowInfo*winInfo,int above){
    if(winInfo->transientFor){
        //check to see if the transient for window refers to a window or a group
        Node*n=getWindowStack(getActiveWorkspace());
        UNTIL_FIRST(n,((WindowInfo*)getValue(n))->groupId==winInfo->transientFor)
        int values[]={n?((WindowInfo*)getValue(n))->id:winInfo->transientFor,XCB_STACK_MODE_ABOVE};
        xcb_configure_window(dis, winInfo->id,XCB_CONFIG_WINDOW_SIBLING|XCB_CONFIG_WINDOW_STACK_MODE,values);
    }
    else
        xcb_configure_window(dis, winInfo->id,XCB_CONFIG_WINDOW_STACK_MODE,
                (int[]){above?XCB_STACK_MODE_ABOVE:XCB_STACK_MODE_BELOW});
}

void tileWorkspace(int index){
    //TDOD support APPLY_RULES_OR_RETURN(TilingWindows,&index);

    Workspace*workspace=getWorkspaceByIndex(index);
    Layout*layout=getActiveLayoutOfWorkspace(workspace->id);
    if(!layout){
        LOG(LOG_LEVEL_TRACE,"workspace %d does not have a layout; skipping \n",workspace->id);
        return;
    }
    if(!isWorkspaceVisible(workspace->id)){
        LOG(LOG_LEVEL_TRACE,"workspace %d is not visible; skipping \n",workspace->id);
        return;
    }
    LOG(LOG_LEVEL_DEBUG,"using layout %s \n",layout->name);
    Monitor*m=getMonitorFromWorkspace(workspace);
    assert(m);

    if(!isNotEmpty(getWindowStack(workspace)))
        LOG(LOG_LEVEL_DEBUG,"WARNING there are not windows to tile\n");
    if(!layout->layoutFunction)
        LOG(LOG_LEVEL_WARN,"WARNING there is not a set layout function\n");
    if(layout->layoutFunction)
        if(!layout->conditionFunction || layout->conditionFunction(layout->conditionArg))
            layout->layoutFunction(m,
                getWindowStack(workspace),layout->args);
        else
            LOG(LOG_LEVEL_TRACE,"condition not met to use layout %s \n",layout->name);
    for(int i=NORMAL_LAYER-1;i>=DESKTOP_LAYER;i--){
        Node*n=workspace->windows[i];
        FOR_EACH(n,tileNonNormalLayers(getValue(n),0));
    }
    for(int i=NORMAL_LAYER+1;i<NUMBER_OF_LAYERS;i++){
        Node*n=workspace->windows[i];
        FOR_EACH(n,tileNonNormalLayers(getValue(n),1));
    }
}

void processConfigureRequest(int win,short values[5],xcb_window_t sibling,int stackMode,int mask){
    LOG(LOG_LEVEL_TRACE,"processing configure request window %d\n",win);
    int actualValues[7];
    int i=0;
    int n=0;

    WindowInfo*winInfo=getWindowInfo(win);
    if(mask & XCB_CONFIG_WINDOW_X && ++n && isExternallyMoveable(winInfo))
        actualValues[i++]=values[n-1];
    else mask&=~XCB_CONFIG_WINDOW_X;
    if(mask & XCB_CONFIG_WINDOW_Y && ++n && isExternallyMoveable(winInfo))
            actualValues[i++]=values[n-1];
    else mask&=~XCB_CONFIG_WINDOW_Y;
    if(mask & XCB_CONFIG_WINDOW_WIDTH && ++n && isExternallyResizable(winInfo))
        actualValues[i++]=values[n-1];
    else mask&=~XCB_CONFIG_WINDOW_WIDTH;
    if(mask & XCB_CONFIG_WINDOW_HEIGHT && ++n && isExternallyResizable(winInfo))
        actualValues[i++]=values[n-1];
    else mask&=~XCB_CONFIG_WINDOW_HEIGHT;
    if(mask & XCB_CONFIG_WINDOW_BORDER_WIDTH && ++n && isExternallyBorderConfigurable(winInfo))
        actualValues[i++]=values[n-1];
    else mask&=~XCB_CONFIG_WINDOW_BORDER_WIDTH;

    if(mask & XCB_CONFIG_WINDOW_SIBLING && ++n && isExternallyRaisable(winInfo))
        actualValues[i++]=sibling;
    else mask&=~XCB_CONFIG_WINDOW_SIBLING;
    if(mask & XCB_CONFIG_WINDOW_STACK_MODE && ++n && isExternallyRaisable(winInfo))
        actualValues[i++]=stackMode;
    else mask&=~XCB_CONFIG_WINDOW_STACK_MODE;

    if(mask){
        xcb_configure_window(dis, win, mask, actualValues);
        LOG(LOG_LEVEL_TRACE,"re-configure window %d mask: %d\n",win,mask);
    }
    else
        LOG(LOG_LEVEL_DEBUG,"configure request denied for window %d; masks %d\n",win,winInfo?winInfo->mask:-1);
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
    for(int i=DESKTOP_LAYER+1;i<NUMBER_OF_LAYERS;i++)
        setLayerState(getActiveWorkspaceIndex(),!value,i);
    xcb_ewmh_set_showing_desktop(ewmh, defaultScreenNumber, value);
}
void toggleShowDesktop(){
    int hideDesktop=isShowingDesktop(getActiveWorkspaceIndex());
    setShowingDesktop(getActiveWorkspaceIndex(),!hideDesktop);
}

short*getConfig(WindowInfo*winInfo){
    return winInfo->config;
}
void setConfig(WindowInfo*winInfo,short*config){
    memcpy(winInfo->config, config, LEN(winInfo->config)*sizeof(short));
}
short*getGeometry(WindowInfo*winInfo){
    return winInfo->geometry;
}
void setGeometry(WindowInfo*winInfo,short*geometry){
    if(winInfo)
        memcpy(winInfo->geometry, geometry, LEN(winInfo->geometry)*sizeof(short));
}
int getMaxDimensionForWindow(Monitor*m,WindowInfo*winInfo,int height){
    if(hasMask(winInfo,ROOT_FULLSCREEN_MASK))
        return (&screen->width_in_pixels)[height];
    else if(hasMask(winInfo,FULLSCREEN_MASK))
        return (&m->width)[height];
    else if(hasMask(winInfo,height?Y_MAXIMIZED_MASK:X_MAXIMIZED_MASK))
        return (&m->viewWidth)[height];
    else return 0;
}
int allowRequestFromSource(WindowInfo* winInfo,int source){
    return !winInfo || hasMask(winInfo, 1<<(source+SRC_INDICATION_OFFSET));
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
