/**
 * @file wmfunctions.c
 * @brief Connect our internal structs to X
 *
 */

#include <assert.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include <X11/extensions/XInput2.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>

#include <xcb/xinput.h>

#include <X11/Xlib-xcb.h>
#include <xcb/randr.h>

#include "bindings.h"
#include "ewmh.h"
#include "mywm-util.h"
#include "mywm-util-private.h"
#include "wmfunctions.h"
#include "window-properties.h"
#include "defaults.h"
#include "devices.h"

static int shuttingDown=0;

#define APPLY_RULES_OR_RETURN(TYPE,winInfo) \
        if(!applyEventRules(eventRules[TYPE],winInfo))\
            return;


int isShuttingDown(){
    return shuttingDown;
}

//window methods

int focusWindow(xcb_window_t win){
    LOG(LOG_LEVEL_WARN,"Trying to set focus to %d\n",win);

    xcb_void_cookie_t cookie=xcb_input_xi_set_focus_checked(dis, win, 0, getActiveMaster()->id);
    if(checkError(cookie,win,"could not set focus to window")){
        return 0;
    }

    onWindowFocus(win);
    return 1;
}
int raiseWindow(xcb_window_t win){

    assert(dis);
    assert(win);
    LOG(LOG_LEVEL_WARN,"Trying to raise window window %d\n",win);
    int values[]={XCB_STACK_MODE_ABOVE};
    if(checkError(xcb_configure_window_checked(dis, win,XCB_CONFIG_WINDOW_STACK_MODE,values),win,"Could not raise window"))
        return 0;
    return 1;
}

int setBorderColor(xcb_window_t win,unsigned int color){
    xcb_void_cookie_t c;
    c=xcb_change_window_attributes_checked(dis, win, XCB_CW_BORDER_PIXEL,&color);

    if(checkError(c,win,"Could not set border color for window"))
        return 0;
    return 1;
}


void registerWindow(WindowInfo*winInfo){
    if(!winInfo){
        assert(0);
        return;
    }
    if(!addWindowInfo(winInfo))
        assert(0);

    Window win=winInfo->id;
    addState(winInfo, defaultWindowMask);
    assert(win);
    xcb_ewmh_get_atoms_reply_t reply;
    if(xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, win), &reply, NULL)){
        setWindowStateFromAtomInfo(winInfo,reply.atoms,reply.atoms_len,ADD);
    }
    else
        setWindowStateFromAtomInfo(winInfo,NULL,0,ADD);


    XSelectInput(dpy, win, NON_ROOT_EVENT_MASKS);

}

void setWindowStateFromAtomInfo(WindowInfo*winInfo,xcb_atom_t* atoms,int numberOfAtoms,int action){
    if(!winInfo)
        return;

    unsigned int mask=0;
    int layer=DEFAULT_LAYER;
    LOG(LOG_LEVEL_TRACE,"updateing state of %d from atoms(%d)",winInfo->id,numberOfAtoms);
    for (unsigned int i = 0; i < numberOfAtoms; i++) {
        if(atoms[i] == ewmh->_NET_WM_STATE_STICKY)
            mask|=STICKY_MASK;
        else if(atoms[i] == ewmh->_NET_WM_STATE_HIDDEN)
            mask|=MINIMIZED_MASK;
        else if(atoms[i] == ewmh->_NET_WM_STATE_MAXIMIZED_VERT)
            mask|=Y_MAXIMIZED_MASK;
        else if(atoms[i] == ewmh->_NET_WM_STATE_MAXIMIZED_VERT)
            mask|=X_MAXIMIZED_MASK;
        else if(atoms[i] == ewmh->_NET_WM_STATE_DEMANDS_ATTENTION)
            mask|=URGENT_MASK;
        else if(atoms[i] == ewmh->_NET_WM_STATE_FULLSCREEN)
            mask|=FULLSCREEN_MASK;
        else if(atoms[i] == ewmh->_NET_WM_STATE_ABOVE)
            layer=UPPER_LAYER;
        else if(atoms[i] == ewmh->_NET_WM_STATE_BELOW)
            layer=LOWER_LAYER;
    }
    Window win=winInfo->id;
    int workspaceIndex=getSavedWorkspaceIndex(win);

    moveWindowToWorkspaceAtLayer(winInfo, workspaceIndex,layer);
    addState(winInfo, mask);

}



int activateWindow(WindowInfo* winInfo){
    LOG(LOG_LEVEL_DEBUG,"activating window\n");
    if(!winInfo)return 0;
    dumpWindowInfo(winInfo);
    switchToWorkspace(winInfo->workspaceIndex);
    return raiseWindow(winInfo->id) && focusWindow(winInfo->id);
}

int deleteWindow(xcb_window_t winToRemove){
    return removeWindow(winToRemove);
}

//Workspace commands
static void attemptToMapWindow(WindowInfo* winInfo){
    if(isWindowInVisibleWorkspace(winInfo) && isMappable(winInfo)){
        checkError(xcb_map_window_checked(dis, winInfo->id),winInfo->id,"mapping window");
        setDirty();
    }
    else{
        LOG(LOG_LEVEL_DEBUG,"Failed to map workspace mappable: %d in visible workspace %d\n",isMappable(winInfo),isWindowInVisibleWorkspace(winInfo));
        //dumpWindowInfo(winInfo);
    }
}
static void updateWindowWorkspaceState(WindowInfo*info, int workspaceIndex,int state){
    if(state){
        //info->activeWorkspaceCount++;
        //assert(info->activeWorkspaceCount);
        attemptToMapWindow(info);
    }
    else{
        //if(info->activeWorkspaceCount)info->activeWorkspaceCount--;
        //assert(info->activeWorkspaceCount==0);
        if(info->activeWorkspaceCount==0)
            xcb_unmap_window(dis, info->id);
    }
}
static void setWorkspaceState(int workspaceIndex,int state){
    Node*stack=getWindowStack(getWorkspaceByIndex(workspaceIndex));
    LOG(LOG_LEVEL_DEBUG,"Setting workspace of %d  to %d:\n",workspaceIndex,state);
    FOR_EACH(stack,
            updateWindowWorkspaceState(getValue(stack),workspaceIndex,state))
}

void switchToWorkspace(int workspaceIndex){

    int currentIndex=getActiveWorkspaceIndex();
    if(currentIndex==workspaceIndex)
        return;
    LOG(LOG_LEVEL_DEBUG,"Switchting to workspace %d:\n",workspaceIndex);
    Workspace*workspaceToSwitchTo=getWorkspaceByIndex(workspaceIndex);
    if(!workspaceToSwitchTo)
        return;
    if(!isWorkspaceVisible(workspaceIndex)){
        //we need to map new windows
        LOG(LOG_LEVEL_DEBUG,"Swaping with visible workspace %d:\n",currentIndex);
        swapMonitors(workspaceIndex,currentIndex);
        setWorkspaceState(workspaceIndex,MAPPED);
        setWorkspaceState(currentIndex,UNMAPPED);

    }
    getActiveMaster()->activeWorkspaceIndex=workspaceIndex;
    xcb_ewmh_set_current_desktop_checked(
                ewmh, DefaultScreen(dpy), workspaceIndex);
}
void activateWorkspace(int workspaceIndex){

    LOG(LOG_LEVEL_TRACE,"Activating workspace:%d\n",workspaceIndex);
    Workspace*workspaceToSwitchTo=getWorkspaceByIndex(workspaceIndex);
    if(!workspaceToSwitchTo)
        return;

    Node*head=getMasterWindowStack();
    LOG(LOG_LEVEL_DEBUG,"Finding first window in active master in workspace %d:\n",workspaceIndex);

    UNTIL_FIRST(head,((WindowInfo*)getValue(head))->workspaceIndex==workspaceIndex)
    if(!head)
        head=getWindowStack(workspaceToSwitchTo);


    if(head->value)
        activateWindow(getValue(head));
    else switchToWorkspace(workspaceIndex);

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
    moveWindowToWorkspaceAtLayer(winInfo,destIndex,DEFAULT_LAYER);
}
void moveWindowToWorkspaceAtLayer(WindowInfo *winInfo,int destIndex,int layer){
    assert(destIndex!=NO_WORKSPACE);
    removeWindowFromAllWorkspaces(winInfo);
    addWindowToWorkspaceAtLayer(winInfo, destIndex, layer);
    updateWindowWorkspaceState(winInfo, destIndex, isWorkspaceVisible(destIndex));
    xcb_ewmh_set_wm_desktop(ewmh, winInfo->id, destIndex);

    LOG(LOG_LEVEL_INFO,"window %d added to workspace %d at layer %d",winInfo->id,destIndex,layer);
}

void setWorkspaceNames(char*names[],int numberOfNames){
    for(int i=0;i<numberOfNames && i<getNumberOfWorkspaces();i++)
        getWorkspaceByIndex(i)->name=names[i];
    xcb_ewmh_set_desktop_names(ewmh, defaultScreenNumber, numberOfNames, (char*)names);
}

void avoidStruct(WindowInfo*info){
    xcb_window_t win=info->id;
    xcb_ewmh_wm_strut_partial_t strut;
    LOG(LOG_LEVEL_INFO,"Avoid structs\n");
    if(xcb_ewmh_get_wm_strut_partial_reply(ewmh,
                xcb_ewmh_get_wm_strut_partial(ewmh, win), &strut, NULL)){
        setDockArea(info,sizeof(xcb_ewmh_wm_strut_partial_t)/sizeof(int),(int*)&strut);
    }
    else if(xcb_ewmh_get_wm_strut_reply(ewmh,
                xcb_ewmh_get_wm_strut(ewmh, win),
                (xcb_ewmh_get_extents_reply_t*) &strut, NULL))
        setDockArea(info,sizeof(xcb_ewmh_get_extents_reply_t)/sizeof(int),(int*)&strut);
    else{
        LOG(LOG_LEVEL_WARN,"could not read struct data\n");
        assert(0);
    }
    addDock(info);
}

void toggleShowDesktop(){}

void detectMonitors(){
    xcb_randr_get_monitors_cookie_t cookie =xcb_randr_get_monitors(dis, root, 1);
    xcb_randr_get_monitors_reply_t *monitors=xcb_randr_get_monitors_reply(dis, cookie, NULL);
    xcb_randr_monitor_info_iterator_t iter=xcb_randr_get_monitors_monitors_iterator(monitors);
    while(iter.rem){
        xcb_randr_monitor_info_t *monitorInfo = iter.data;
        addMonitor(monitorInfo->name,monitorInfo->primary,&monitorInfo->x);
        xcb_randr_monitor_info_next(&iter);
    }
    free(monitors);
}

void processNewWindow(WindowInfo* winInfo){
    LOG(LOG_LEVEL_DEBUG,"processing %d (%x)\n",winInfo->id,(unsigned int)winInfo->id);

    loadWindowProperties(winInfo);
    //dumpWindowInfo(winInfo);
    APPLY_RULES_OR_RETURN(ProcessingWindow,winInfo);
    LOG(LOG_LEVEL_DEBUG,"Registerign window\n");
    registerWindow(winInfo);

}

void connectToXserver(){
    LOG(LOG_LEVEL_DEBUG," connecting to XServe \n");
    for(int i=0;i<30;i++){
        dpy = XOpenDisplay(NULL);
        if(dpy)
            break;
        msleep(50);
    }
    if(!dpy){
        LOG(LOG_LEVEL_ERROR," Failed to connect to xserver\n");
        exit(60);
    }
    dis = XGetXCBConnection(dpy);
    if(xcb_connection_has_error(dis)){
        LOG(LOG_LEVEL_ERROR," failed to conver xlib display to xcb version\n");
        exit(1);
    }
    XSetEventQueueOwner(dpy, XCBOwnsEventQueue);

    screen=xcb_setup_roots_iterator (xcb_get_setup (dis)).data;

    root = DefaultRootWindow(dpy);
    ewmh = malloc(sizeof(xcb_ewmh_connection_t));
    xcb_intern_atom_cookie_t *cookie;
    cookie = xcb_ewmh_init_atoms(dis, ewmh);
    xcb_ewmh_init_atoms_replies(ewmh, cookie, NULL);
    xcb_set_close_down_mode(dis, XCB_CLOSE_DOWN_DESTROY_ALL);
    APPLY_RULES_OR_RETURN(onXConnection,NULL);
}


void onXConnect(){



    xcb_ewmh_set_showing_desktop(ewmh, defaultScreenNumber, 0);
    registerForEvents();
    broadcastEWMHCompilence();
    initCurrentMasters();
    assert(getActiveMaster()!=NULL);

    unsigned int currentWorkspace;

    if(!xcb_ewmh_get_current_desktop_reply(ewmh,
            xcb_ewmh_get_current_desktop(ewmh, defaultScreenNumber),
            &currentWorkspace, NULL)){
        currentWorkspace=DEFAULT_WORKSPACE_INDEX;
    }

    xcb_ewmh_set_number_of_desktops(ewmh, defaultScreenNumber, getNumberOfWorkspaces());
    activateWorkspace(currentWorkspace);
    detectMonitors();

    scan();
    tileWindows();
    xcb_flush(dis);
    LOG(LOG_LEVEL_TRACE,"finished init x connection \n");

}

void broadcastEWMHCompilence(){
    LOG(LOG_LEVEL_TRACE,"Compliying with EWMH\n");
    SET_SUPPORTED_OPERATIONS(ewmh);

    //functionless window required by EWMH spec
    //we set its class to input only and set override redirect so we (and anyone else  ignore it)
    int noevents = 1;
    xcb_window_t checkwin = xcb_generate_id(dis);
    xcb_create_window(dis, 0, checkwin, root, 0, 0, 1, 1, 0,
                      XCB_WINDOW_CLASS_INPUT_ONLY, 0, XCB_CW_OVERRIDE_REDIRECT, &noevents);
    xcb_ewmh_set_supporting_wm_check(ewmh, root, checkwin);
    xcb_ewmh_set_wm_name(ewmh, checkwin, strlen(WM_NAME), WM_NAME);

    //TODO update on monitor detection
    //xcb_ewmh_set_desktop_viewport_checked(ewmh, screen_nbr, list_len, list);

    LOG(LOG_LEVEL_TRACE,"ewmh initilized\n");
}

void updateMapState(int id,int state){
    WindowInfo*winInfo=getWindowInfo(id);
    if(!winInfo){
        if(state)
            xcb_map_window(dis, id);
        return;
    }
    if(state){
        loadWindowHints(winInfo);
        if(winInfo->state==XCB_ICCCM_WM_STATE_WITHDRAWN){
            LOG(LOG_LEVEL_WARN,"client explicint requested map but hints say it should be withdrawn; ignoring hints");
            winInfo->state=XCB_ICCCM_WM_STATE_NORMAL;
        }
        attemptToMapWindow(winInfo);
    }
    winInfo->mapped=state;
}


void closeConnection(){
    LOG(LOG_LEVEL_INFO,"closing X connection\n");
    if(dpy)
        XCloseDisplay(dpy);
}

void quit(){
    LOG(LOG_LEVEL_INFO,"Shuttign down\n");
    shuttingDown=1;
    closeConnection();
    destroyContext();
    exit(0);
}

void applyLayout(Workspace*workspace,Layout* layout){
    LOG(LOG_LEVEL_DEBUG,"using layout %s with %d\n",workspace->activeLayout->name,getNumberOfTileableWindows(getWindowStack(workspace)));
    Monitor*m=getMonitorFromWorkspace(workspace);
    assert(m);
    if(layout && layout->layoutFunction)
        layout->layoutFunction(m,
            getWindowStack(workspace),layout->args);
}
void tileWindows(){
    if(isDirty()){
        APPLY_RULES_OR_RETURN(TilingWindows,NULL);
        for(int i=0;i<getNumberOfWorkspaces();i++)
            if(isWorkspaceVisible(i)){
                Workspace* w=getWorkspaceByIndex(i);
                LOG(LOG_LEVEL_DEBUG,"tiling %d windows\n",getSize(getWindowStack(w)));
                applyLayout(w,w->activeLayout);
            }
    }
    setClean();
}
void scan() {
    xcb_query_tree_reply_t *reply;
    reply = xcb_query_tree_reply(dis, xcb_query_tree(dis, root), 0);
    if (reply) {
        int numberOfChildren = xcb_query_tree_children_length(reply);
        LOG(LOG_LEVEL_DEBUG,"detected %d kids\n",numberOfChildren);
        xcb_window_t *children = xcb_query_tree_children(reply);
        xcb_get_window_attributes_reply_t *attr;
        xcb_get_window_attributes_cookie_t cookies[numberOfChildren];
        for (int i = 0; i < numberOfChildren; i++)
            cookies[i]=xcb_get_window_attributes(dis, children[i]);

        for (int i = 0; i < numberOfChildren; i++) {
            LOG(LOG_LEVEL_DEBUG,"processing child %d\n",children[i]);
            attr = xcb_get_window_attributes_reply(dis,cookies[i], NULL);

            if(!attr)
                assert(0);
            if(!MANAGE_OVERRIDE_REDIRECT_WINDOWS && attr->override_redirect || attr->_class ==XCB_WINDOW_CLASS_INPUT_ONLY){
                LOG(LOG_LEVEL_TRACE,"Skipping child override redirect: %d class: %d\n",attr->override_redirect,attr->_class);
            }
            else {
                WindowInfo *winInfo=createWindowInfo(children[i]);
                    winInfo->mapped=attr->map_state?1:0;
                winInfo->overrideRedirect=attr->override_redirect;
                processNewWindow(winInfo);
            }
            free(attr);
        }
        free(reply);
    }
    else {
        LOG(LOG_LEVEL_ERROR,"could not query tree\n");
        assert(0);
    }
}
#define COPY_VALUES(I)  actualWindowValues[I]=winInfo->config[I]?winInfo->config[I]:values[I];
void configureWindow(Monitor*m,WindowInfo* winInfo,short values[CONFIG_LEN]){
    assert(winInfo);
    assert(m);
    int actualWindowValues[CONFIG_LEN+1];
    int mask= XCB_CONFIG_WINDOW_X|XCB_CONFIG_WINDOW_Y|
        XCB_CONFIG_WINDOW_WIDTH|XCB_CONFIG_WINDOW_HEIGHT |
        XCB_CONFIG_WINDOW_BORDER_WIDTH|XCB_CONFIG_WINDOW_STACK_MODE;
    if(winInfo){
        COPY_VALUES(0);//x
        COPY_VALUES(1);//y

        int maxWidth=getMaxDimensionForWindow(m, winInfo,0);
        if(maxWidth)
            actualWindowValues[2]=maxWidth;
        else    COPY_VALUES(2);

        int maxHeight=getMaxDimensionForWindow(m, winInfo,1);
        if(maxHeight){
            LOG(LOG_LEVEL_DEBUG,"hiehg toverried\n");
            assert(maxHeight>=values[3]);
            actualWindowValues[3]=maxHeight;
        }
        else  COPY_VALUES(3);
        COPY_VALUES(4);//border
        int i=5;
        if(winInfo->transientFor){
            mask|=XCB_CONFIG_WINDOW_SIBLING;
            actualWindowValues[i++]=winInfo->transientFor;
        }
        actualWindowValues[i]=values[5];
    }
    assert(winInfo->id);
    xcb_configure_window_checked(dis, winInfo->id, mask, actualWindowValues);
    LOG(LOG_LEVEL_DEBUG,"Window dims %d %d %d %d %d %d\n",values[0],values[1],values[2],values[3],values[4],values[5]);
    LOG(LOG_LEVEL_DEBUG,"Window dims %d %d %d %d \n",m->width,m->height,m->viewWidth,m->viewHeight);
    //if(checkError(c, winInfo->id, "could not configure window"))assert(0);
}
void processConfigureRequest(Window win,short values[5],xcb_window_t sibling,int stackMode,int mask){

    int actualValues[7];
    int i=0;
    LOG(LOG_LEVEL_DEBUG,"Window dims %d %d %d %d %d %d %d %d\n",values[0],values[1],values[2],values[3],values[4],values[5],values[6],mask);
    WindowInfo*winInfo=getWindowInfo(win);
    if(mask & XCB_CONFIG_WINDOW_X && isExternallyMoveable(winInfo))
        actualValues[i++]=values[0];
    else mask&=~XCB_CONFIG_WINDOW_X;
    if(mask & XCB_CONFIG_WINDOW_Y && isExternallyMoveable(winInfo))
            actualValues[i++]=values[1];
    else mask&=~XCB_CONFIG_WINDOW_Y;
    if(mask & XCB_CONFIG_WINDOW_WIDTH && isExternallyResizable(winInfo))
        actualValues[i++]=values[2];
    else mask&=~XCB_CONFIG_WINDOW_WIDTH;
    if(mask & XCB_CONFIG_WINDOW_HEIGHT && isExternallyResizable(winInfo))
        actualValues[i++]=values[3];
    else mask&=~XCB_CONFIG_WINDOW_HEIGHT;
    if(mask & XCB_CONFIG_WINDOW_BORDER_WIDTH)
        actualValues[i++]=values[4];
    else mask&=~XCB_CONFIG_WINDOW_BORDER_WIDTH;

    if(mask & XCB_CONFIG_WINDOW_SIBLING)
        actualValues[i++]=sibling;
    else mask&=~XCB_CONFIG_WINDOW_SIBLING;
    if(mask & XCB_CONFIG_WINDOW_STACK_MODE)
        actualValues[i++]=stackMode;
    else mask&=~XCB_CONFIG_WINDOW_STACK_MODE;
    LOG(LOG_LEVEL_TRACE,"Window dims %d %d %d %d %d %d %d \n",i,actualValues[0],actualValues[1],actualValues[2],actualValues[3],actualValues[4],mask);
    xcb_configure_window(dis, win, mask, actualValues);
}

void killWindow(xcb_window_t win){
    if(win)
        xcb_kill_client(dis, win);
}

int dirty;
void setClean(){
    dirty=0;
}
void setDirty(){
    dirty=1;
}
int isDirty(){
    return dirty;
}
