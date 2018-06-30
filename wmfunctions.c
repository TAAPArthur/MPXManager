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

static int shuttingDown=0;

#define APPLY_RULES_OR_RETURN(TYPE,winInfo) \
        if(!applyEventRules(TYPE,winInfo))\
            return;
void msleep(int mil){
    int sec=mil/1000;
    mil=mil-sec;
    nanosleep((const struct timespec[]){{sec, mil*1e6}}, NULL);
}

Display *dpy;
xcb_connection_t *dis;
xcb_ewmh_connection_t *ewmh;
int root;
xcb_screen_t* screen;
int defaultScreenNumber;

int defaultWindowMask=0;

int isShuttingDown(){
    return shuttingDown;
}

//Master device methods
int getAssociatedMasterDevice(int deviceId){
    int ndevices;
    XIDeviceInfo *masterDevices;
    int id;
    masterDevices = XIQueryDevice(dpy, deviceId, &ndevices);
    id=masterDevices[0].attachment;
    XIFreeDeviceInfo(masterDevices);
    return id;
}
int getClientKeyboard(){
    int masterPointer;
    XIGetClientPointer(dpy, 0, &masterPointer);
    return getAssociatedMasterDevice(masterPointer);
}

int ungrabDevice(int id){
    return XIUngrabDevice(dpy, id, 0);
}
int grabDevice(int deviceID,int maskValue){
    XIEventMask eventmask = {deviceID,2,(unsigned char*)&maskValue};
    return XIGrabDevice(dpy, deviceID,  root, CurrentTime, None, GrabModeAsync,
                                       GrabModeAsync, False, &eventmask);
}
int grabButton(int deviceID,int button,int mod,int maskValue){
    XIEventMask eventmask = {deviceID,2,(unsigned char*)&maskValue};
    XIGrabModifiers modifiers[2]={mod,mod|IGNORE_MASK};
    return XIGrabButton(dpy, deviceID, button, root, 0,
        XIGrabModeAsync, XIGrabModeAsync, 1, &eventmask, 2, modifiers);
}
int ungrabButton(int deviceID,int button,int mod){
    XIGrabModifiers modifiers[2]={mod,mod|IGNORE_MASK};
    return XIUngrabButton(dpy, deviceID, button, root, 2, modifiers);
}
int grabKey(int deviceID,int keyCode,int mod,int maskValue){
    XIEventMask eventmask = {deviceID,2,(unsigned char*)&maskValue};
    XIGrabModifiers modifiers[2]={{mod},{mod|IGNORE_MASK}};
    return XIGrabKeycode(dpy, deviceID, keyCode, root, XIGrabModeAsync, XIGrabModeAsync,
            1, &eventmask, 2, modifiers);
}
int ungrabKey(int deviceID,int keyCode,int mod){
    XIGrabModifiers modifiers[2]={{mod},{mod|IGNORE_MASK}};
    return XIUngrabKeycode(dpy, deviceID, keyCode, root, 1, modifiers);
}

//master methods
int setActiveMasterByMouseId(int mouseId){
    int masterMouseOrMasterKeyboaredId=getAssociatedMasterDevice(mouseId);

    //master pointer is functionly the same as keyboard slave
    return !setActiveMasterByKeyboardId(masterMouseOrMasterKeyboaredId);
}
int setActiveMasterByKeyboardId(int keyboardId){
    Node*masterNode=getMasterNodeById(keyboardId);
    if(masterNode){
        //if true then keyboardId was a master keyboard
        setActiveMasterNodeById(masterNode);
        return 1;
    }
    masterNode=getMasterNodeById(getAssociatedMasterDevice(keyboardId));
    if(masterNode){
        setActiveMasterNodeById(masterNode);
        return 0;
    }
    LOG(LOG_LEVEL_ERROR,"device id was not a master keyboard or slave keyboard/slave pointer");
    assert(0);
    return -1;
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
/*
int setBorder(xcb_window_t wid){
    return setBorderColor(wid,getActiveMaster()->focusColor);
}
int resetBorder(xcb_window_t win){
    Master*master=getValue(getNextMasterNodeFocusedOnWindow(win));
    if(master)
        return setBorderColor(win,master->focusColor);
    return setBorderColor(win,getActiveMaster()->unfocusColor);
}

*/

int isExternallyResizable(WindowInfo* winInfo){
    return winInfo->mask & EXTERNAL_RESIZE_MASK;
}
int isExternallyMoveable(WindowInfo* winInfo){
    return winInfo->mask & EXTERNAL_RESIZE_MASK;
}

static int hasMask(WindowInfo* win,int mask){
    return win->mask & mask;
}

void updateState(WindowInfo*winInfo,int mask,MaskAction action){
    if(action==TOGGLE)
        action=hasMask(winInfo,mask)?REMOVE:ADD;
}
void addState(WindowInfo*winInfo,int mask){
    int layer=NO_LAYER;
    if(mask &STICKY_MASK){
        layer=Above;
    }
    if(mask &MINIMIZED_MASK){
        winInfo->minimizedTimeStamp=getTime();
        xcb_unmap_window(dis,winInfo->id);
    }
    if(mask &X_MAXIMIZED_MASK ){}
    if(mask &BANISHED_MASK){}
    if(mask &FULLSCREEN_MASK){}
    if(mask &EXTERNAL_RESIZE_MASK){}
    if(layer!=NO_LAYER)
        moveWindowToLayerForAllWorksppaces(winInfo, layer);

    winInfo->mask=mask;
}
void removeState(WindowInfo*winInfo,int mask){
//TODO implement
}

int getMaxDimensionForWindow(Monitor*m,WindowInfo*winInfo,int height){
    int maxDimMask=height?Y_MAXIMIZED_MASK:X_MAXIMIZED_MASK;
    if(winInfo->mask & (maxDimMask|PRIVILGED_MASK)){
        if(winInfo->mask&PRIVILGED_MASK)
            return winInfo->mask&ROOT_FULLSCREEN_MASK?(&screen->width_in_pixels)[height]:(&m->width)[height];
        else return (&m->viewWidth)[height];
    }
    else return 0;
}

int getNumberOfTileableWindows(Node*windowStack){

    int size=0;
    FOR_EACH(windowStack,
            if(isTileable(getValue(windowStack)))size++);
    return size;
}
int isTileable(WindowInfo*winInfo){
    return isMapped(winInfo) && !(winInfo->mask & (NO_TILE_MASK|PRIVILGED_MASK));
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
static int getSavedWorkspaceIndex(xcb_window_t win){
    unsigned int workspaceIndex=NO_WORKSPACE;

    if ((xcb_ewmh_get_wm_desktop_reply(ewmh,
              xcb_ewmh_get_wm_desktop(ewmh, win), &workspaceIndex, NULL))) {
        if(workspaceIndex>=getNumberOfWorkspaces())
            workspaceIndex=getNumberOfWorkspaces()-1;
    }
    else workspaceIndex = getActiveWorkspaceIndex();
    return workspaceIndex;
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

void applyLayout(Workspace*workspace,Layout* layout){
    LOG(LOG_LEVEL_DEBUG,"using layour %s\n",workspace->activeLayout->name);
    Monitor*m=getMonitorFromWorkspace(workspace);
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

int activateWindow(WindowInfo* winInfo){
    switchToWorkspace(winInfo->workspaceIndex);
    return activateWindowById(winInfo->id);
}
int activateWindowById(int id){
    LOG(LOG_LEVEL_TRACE,"activating window %d:\n",id);
    assert(id);

    return raiseWindow(id) && focusWindow(id);
}

int deleteWindow(xcb_window_t winToRemove){
    return removeWindow(winToRemove);
}
//Workspace commands

static void updateWindowWorkspaceState(WindowInfo*info, int workspaceIndex,int state){
    if(state==UNMAPPED){
        if(info->activeWorkspaceCount)
            info->activeWorkspaceCount--;
        assert(info->activeWorkspaceCount==0);
        if(info->activeWorkspaceCount==0)
            xcb_unmap_window(dis, info->id);
    }
    else{
        info->activeWorkspaceCount++;
        assert(info->activeWorkspaceCount);
        info->workspaceIndex=workspaceIndex;
        xcb_map_window(dis, info->id);
    }
}
static void setWorkspaceState(int workspaceIndex,int state){
    Node*stack=getWindowStack(getWorkspaceByIndex(workspaceIndex));
    FOR_EACH(stack,
            updateWindowWorkspaceState(getValue(stack),workspaceIndex,state))
}
void swapMonitors(int index1,int index2){
    Monitor*temp=getWorkspaceByIndex(index1)->monitor;
    getWorkspaceByIndex(index1)->monitor=getWorkspaceByIndex(index2)->monitor;
    getWorkspaceByIndex(index2)->monitor=temp;
    setDirty();
}
void switchToWorkspace(int workspaceIndex){

    int currentIndex=getActiveWorkspaceIndex();
    if(currentIndex==workspaceIndex)
        return;
    LOG(LOG_LEVEL_TRACE,"Switchting to workspace %d:\n",workspaceIndex);
    Workspace*workspaceToSwitchTo=getWorkspaceByIndex(workspaceIndex);
    if(!workspaceToSwitchTo)
        return;
    if(!isWorkspaceVisible(workspaceIndex)){
        //we need to map new windows
        swapMonitors(workspaceIndex,currentIndex);
        setWorkspaceState(currentIndex,UNMAPPED);
        setWorkspaceState(workspaceIndex,MAPPED);
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

void moveWindowToWorkspace(WindowInfo* winInfo,int destIndex){
    assert(destIndex!=NO_WORKSPACE);
    moveWindowToWorkspaceAtLayer(winInfo,destIndex,DEFAULT_LAYER);
}
void moveWindowToWorkspaceAtLayer(WindowInfo *winInfo,int destIndex,int layer){
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
        memcpy(info->properties, &strut, sizeof(xcb_ewmh_wm_strut_partial_t));
    }
    else if(xcb_ewmh_get_wm_strut_reply(ewmh,
                xcb_ewmh_get_wm_strut_partial(ewmh, win),
                (xcb_ewmh_get_extents_reply_t*) &strut, NULL))
            memcpy(info->properties, &strut, sizeof(xcb_ewmh_get_extents_reply_t));
    else
        LOG(LOG_LEVEL_WARN,"could not read struct data\n");

    addDock(info);
}

void toggleShowDesktop(){}

void detectMonitors(){
    xcb_randr_get_monitors_cookie_t cookie =xcb_randr_get_monitors(dis, root, 1);
    xcb_randr_get_monitors_reply_t *monitors=xcb_randr_get_monitors_reply(dis, cookie, NULL);
    xcb_randr_monitor_info_iterator_t iter=xcb_randr_get_monitors_monitors_iterator(monitors);
    while(iter.rem){
        xcb_randr_monitor_info_t *monitorInfo = iter.data;
        addMonitor(monitorInfo->name,monitorInfo->primary,monitorInfo->x,monitorInfo->y,monitorInfo->width,monitorInfo->height);
        xcb_randr_monitor_info_next(&iter);
    }
    free(monitors);
}

void processNewWindow(WindowInfo* winInfo){
    LOG(LOG_LEVEL_DEBUG,"processing %d (%x)\n",winInfo->id,(unsigned int)winInfo->id);

    loadWindowProperties(winInfo);
    dumpWindowInfo(winInfo);
    APPLY_RULES_OR_RETURN(ProcessingWindow,winInfo);
    LOG(LOG_LEVEL_DEBUG,"Registerign window\n");
    registerWindow(winInfo);

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
    LOG(LOG_LEVEL_DEBUG,"class name %s instance name: %s \n",info->className,info->instanceName);

}
void loadTitleInfo(WindowInfo*winInfo){
    xcb_ewmh_get_utf8_strings_reply_t wtitle;
    xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_name(ewmh, winInfo->id);
    if(winInfo->title)
        free(winInfo->title);
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
        LOG(LOG_LEVEL_DEBUG,"window title %s\n",winInfo->title);
}

void loadWindowType(WindowInfo *winInfo){
    LOG(LOG_LEVEL_TRACE,"loading type\n");
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

void loadWindowProperties(WindowInfo *winInfo){
    if(!winInfo)
        return;


    loadClassInfo(winInfo);
    loadTitleInfo(winInfo);
    xcb_icccm_wm_hints_t hints;

    if(xcb_icccm_get_wm_hints_reply(dis, xcb_icccm_get_wm_hints(dis, winInfo->id), &hints, NULL)){
        winInfo->groupId=hints.window_group;
        winInfo->input=hints.input;
    }
    loadWindowType(winInfo);



    xcb_window_t prop;
    if(xcb_icccm_get_wm_transient_for_reply(dis,
            xcb_icccm_get_wm_transient_for(dis, winInfo->id), &prop, NULL)){
        winInfo->transientFor=prop;
    }
    //dumpWindowInfo(winInfo);
}

#define COPY_VALUES(I)  actualWindowValues[I]=winInfo->config[I]?winInfo->config[I]:values[I];
void configureWindow(Monitor*m,WindowInfo* winInfo,short values[CONFIG_LEN]){
    int actualWindowValues[CONFIG_LEN+1];
    int mask= XCB_CONFIG_WINDOW_X|XCB_CONFIG_WINDOW_Y|
        XCB_CONFIG_WINDOW_WIDTH|XCB_CONFIG_WINDOW_HEIGHT |
        XCB_CONFIG_WINDOW_BORDER_WIDTH|XCB_CONFIG_WINDOW_STACK_MODE;
    if(winInfo){
        COPY_VALUES(0);//x
        COPY_VALUES(1);//y

        int maxWidth=getMaxDimensionForWindow(m, winInfo,0);
        if(maxWidth){
            LOG(LOG_LEVEL_DEBUG,"width toverried\n");
            actualWindowValues[2]=maxWidth;
        }
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
    xcb_void_cookie_t c=xcb_configure_window_checked(dis, winInfo->id, mask, actualWindowValues);
    //LOG(LOG_LEVEL_ERROR,"Window dims %d %d %d %d %d %d\n",values[0],values[1],values[2],values[3],values[4],values[5]);
    //LOG(LOG_LEVEL_ERROR,"Window dims %d %d %d %d \n",m->width,m->height,m->viewWidth,m->viewHeight);
    if(checkError(c, winInfo->id, "could not configure window"))
        assert(0);

}

int checkError(xcb_void_cookie_t cookie,int cause,char*msg){
    xcb_generic_error_t* e=xcb_request_check(dis,cookie);
    if(e){
        printf("error occured with window %d %s\n\n",cause,msg);
        char buff[256];
        XGetErrorText(dpy,e->error_code,buff,LEN(buff));
        LOG(LOG_LEVEL_ERROR,"Error code %d %s \n",
                   e->error_code,buff) ;
        free(e);
        assert(0);
    }
    return e?1:0;
}

/*TODO uncomment
void moveResize(Window win,int*values){
    int mask=0;
    WindowInfo*winInfo=getWindowInfo(win);
    if(DEFAULT_BORDER_WIDTH>=0)
        mask|=XCB_CONFIG_WINDOW_BORDER_WIDTH;
    if(winInfo){
        if(isExternallyResizable(winInfo))

            mask |= XCB_CONFIG_WINDOW_WIDTH|XCB_CONFIG_WINDOW_HEIGHT;
        if(isExternallyMoveable(winInfo))
            mask |= XCB_CONFIG_WINDOW_X|XCB_CONFIG_WINDOW_Y;
        if(isTileable(winInfo))
            mask |=XCB_CONFIG_WINDOW_SIBLING|XCB_CONFIG_WINDOW_STACK_MODE;
    }
    else mask=127; //all masks
    xcb_configure_window(dis, win, mask, values);
}*/

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
                if(attr)
                    setMapped(winInfo, attr->map_state);
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
int isWindowVisible(Window win){
    return isVisible(getWindowInfo(win));
}

void registerForEvents(){
    XSelectInput(dpy, root, ROOT_EVENT_MASKS);

    LOG(LOG_LEVEL_TRACE,"loading key %d bindings %d\n",bindingLengths[0],bindingMasks[0]);

    for(int i=0;i<4;i++)
        if(bindings[i])
            if(i<2)
                grabKeys(bindings[i],bindingLengths[i],bindingMasks[i]);
            else
                grabButtons(bindings[i],bindingLengths[i],bindingMasks[i]);
        else
            LOG(LOG_LEVEL_DEBUG,"no bindings %d",i);

    //TODO listen on select devices
    LOG(LOG_LEVEL_TRACE,"listening for device event;  masks: %d\n",DEVICE_EVENT_MASKS);
    XIEventMask eventmask = {XIAllDevices,4,(unsigned char*)&DEVICE_EVENT_MASKS};
    XISelectEvents(dpy, root, &eventmask, 1);
}

void initCurrentMasters(){
    int ndevices;
    //xcb_input_xi_query_device(c, deviceid);
    XIDeviceInfo *devices, *device;
    devices = XIQueryDevice(dpy, XIAllMasterDevices, &ndevices);

    for (int i = 0; i < ndevices; i++) {
        device = &devices[i];
        if(device->use == XIMasterKeyboard ){
            addMaster(device->deviceid);
        }
    }
    XIFreeDeviceInfo(devices);
}

void connectToXserver(){
    LOG(LOG_LEVEL_DEBUG," connecting to XServe \n");
    dpy = XOpenDisplay(NULL);
    if (!dpy)
        exit(1);
    dis = XGetXCBConnection(dpy);
    if(xcb_connection_has_error(dis)){
        LOG(LOG_LEVEL_ERROR," failed to conver xlib display to xcb version\n");
        exit(1);
    }
    xcb_screen_iterator_t iter;
    iter = xcb_setup_roots_iterator (xcb_get_setup (dis));
    screen=iter.data;

    root = DefaultRootWindow(dpy);
    ewmh = malloc(sizeof(xcb_ewmh_connection_t));
    xcb_intern_atom_cookie_t *cookie;
    cookie = xcb_ewmh_init_atoms(dis, ewmh);
    xcb_ewmh_init_atoms_replies(ewmh, cookie, NULL);
    XSetCloseDownMode(dpy,DestroyAll);

    onXConnect();

}
void onXConnect(){

    APPLY_RULES_OR_RETURN(onXConnection,NULL);

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

void setDefaultWindowMasks(int mask){
    defaultWindowMask=mask;
}
int isVisible(WindowInfo* winInfo){
    return winInfo && winInfo->visibile!=VisibilityFullyObscured;
}
void setVisible(WindowInfo* winInfo,int v){
    winInfo->visibile=v;
}
int isActivatable(WindowInfo* winInfo){
    return winInfo->input;
}
int isMapped(WindowInfo* winInfo){
    return winInfo && winInfo->mapState!=XCB_MAP_STATE_UNMAPPED;
}
void setMapped(WindowInfo* winInfo,xcb_map_state_t state){
    if(!winInfo)
        return;
    if(winInfo->mapState == state)
        return;

    if(isWindowInVisibleWorkspace(winInfo)){
        if(isTileable(winInfo))
            setDirty();
        if(state != XCB_MAP_STATE_UNMAPPED)
            xcb_map_window(dis, winInfo->id);
    }
    else if(state != XCB_MAP_STATE_UNMAPPED)
        addState(winInfo, IN_INVISIBLE_WORKSPACE);

    winInfo->mapState=state;
}
int getWindowMask(WindowInfo*info){
    return info->mask;
}


void closeConnection(){
    LOG(LOG_LEVEL_INFO,"closing X connection\n");
    XCloseDisplay(dpy);
}

void quit(){
    LOG(LOG_LEVEL_ERROR,"Shuttign down\n");
    shuttingDown=1;
    closeConnection();
    destroyContext();
}
