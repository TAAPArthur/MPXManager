
#include <assert.h>


#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/randr.h>


#include "defaults.h"
#include "bindings.h"
#include "event-loop.h"
#include "wmfunctions.h"

/**Mask of all events we listen for on the root window*/
int ROOT_EVENT_MASKS=SubstructureRedirectMask|SubstructureNotifyMask|StructureNotifyMask;
/**Mask of all events we listen for on non-root windows*/
int NON_ROOT_EVENT_MASKS=VisibilityChangeMask;

/**Mask of all events we listen for on all Master devices when grabing specific buttons/keys
 * (ie not grabing the entire device).
 * */
int DEVICE_EVENT_MASKS=XI_FocusInMask|XI_HierarchyChangedMask|XI_ButtonPressMask;
/**
 * The masks to use when grabing the keyboard for chain bindings
 */
int CHAIN_BINDING_GRAB_MASKS = XI_KeyPressMask|XI_KeyReleaseMask;
/**Lists of modifer masks to ignore. Sane default is ModMask2 (NumLock)*/
int IGNORE_MASK=Mod2Mask ;
/**Number of workspaces default is 10*/
int NUMBER_OF_WORKSPACES=1;
/**Border width when borders are active*/
int DEFAULT_BORDER_WIDTH=1;

/**Whether or not to manage override redirect windows. Note doing so breaks EWMH spec and
 * and you still won't receive events on these windows.
 * */
char MANAGE_OVERRIDE_REDIRECT_WINDOWS=0;

char* SHELL="/bin/sh";




Node* eventRules[NUMBER_OF_EVENT_RULES];

Rules DEFAULT_RULES[NUMBER_OF_EVENT_RULES] = {

    [VisibilityNotify] = CREATE_DEFAULT_EVENT_RULE(onVisibilityEvent),
    [CreateNotify] = CREATE_DEFAULT_EVENT_RULE(onCreateEvent),

    [DestroyNotify] = CREATE_DEFAULT_EVENT_RULE(onDestroyEvent),
    [UnmapNotify] = CREATE_DEFAULT_EVENT_RULE(onUnmapEvent),
    [MapRequest] = CREATE_DEFAULT_EVENT_RULE(onMapRequestEvent),
    [ConfigureRequest] = CREATE_DEFAULT_EVENT_RULE(onConfigureRequestEvent),


    [PropertyNotify] = CREATE_DEFAULT_EVENT_RULE(onPropertyEvent),
    [ClientMessage] = CREATE_DEFAULT_EVENT_RULE(onClientMessage),

    [GenericEvent] = CREATE_DEFAULT_EVENT_RULE(onGenericEvent),

    [XI_KeyPress + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onDeviceEvent),
    [XI_KeyRelease + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onDeviceEvent),
    [XI_ButtonPress + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onDeviceEvent),
    [XI_ButtonRelease + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onDeviceEvent),
    [XI_Enter + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onEnterEvent),
    [XI_FocusIn + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onFocusInEvent),
    [XI_FocusOut + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onFocusOutEvent),
    [XI_HierarchyChanged + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onHiearchyChangeEvent),

};


void onStartup(){
    for(unsigned int i=0;i<NUMBER_OF_EVENT_RULES;i++)
        if(DEFAULT_RULES[i].onMatch.type)
            eventRules[i]=createHead(&DEFAULT_RULES[i]);
        else eventRules[i]=createEmptyHead();
}


void onHiearchyChangeEvent(){
    XIHierarchyEvent *event=getLastEvent();
    if((event->flags & (XIMasterAdded |XIMasterRemoved)) ==0)
        return;
    for (int i = 0; i < event->num_info; i++){
        if (event->info[i].flags & XIMasterRemoved)
            removeMaster(event->info[i].deviceid);
        else if (event->info[i].flags & XIMasterAdded)
            addMaster(event->info[i].deviceid);
    }
}
void onDeviceEvent(){
    XIDeviceEvent *event=getLastEvent();

    LOG(LOG_LEVEL_TRACE,"deviec event %d %d %d\n",event->type,event->deviceid,event->sourceid);
    if(event->evtype<XI_ButtonPress){//key press/release
        setActiveMasterByKeyboardId(event->deviceid);

        //setActiveMasterByKeyboardSlaveId(event->deviceid);
    }
    else{//button press/release
        //setActiveMasterByKeyboardSlaveId(event->deviceid);
        setActiveMasterByMouseId(event->deviceid);
        setLastWindowClicked(event->child);
    }

    checkBindings(event->detail,event->mods.effective,event->evtype,
            getWindowInfo(event->child));
}

void onConfigureRequestEvent(){
    /*
    LOG(LOG_LEVEL_TRACE,"configure request received\n");
    XConfigureRequestEvent *event = getLastEvent();
    int win=event->window;
    int *values=&event->x;
    moveResize(win,values);*/
}
void onCreateEvent(){

    LOG(LOG_LEVEL_TRACE,"create event received\n");
    XCreateWindowEvent *event= getLastEvent();
    if(event->override_redirect && !MANAGE_OVERRIDE_REDIRECT_WINDOWS)return;

    WindowInfo*winInfo=createWindowInfo(event->window);
    winInfo->overrideRedirect=event->override_redirect;
    LOG(LOG_LEVEL_TRACE,"window: %x parent: %x\n",(unsigned int)event->window,(unsigned int)event->parent);
    processNewWindow(winInfo);

    //TODO set client list
    //xcb_ewmh_set_client_list_checked(ewmh, ->screen, list_len, list);
}
void onDestroyEvent(){
    LOG(LOG_LEVEL_TRACE,"destroy event received\n");
    XDestroyWindowEvent *event= getLastEvent();
    deleteWindow(event->window);
}
void onVisibilityEvent(){
    LOG(LOG_LEVEL_TRACE,"made it to visibility request event\n");
    XVisibilityEvent*event= getLastEvent();
    setVisible(getWindowInfo(event->window),event->state);

}
void onMapRequestEvent(){
    LOG(LOG_LEVEL_TRACE,"made it to map request event\n");
    XMapRequestEvent *event= getLastEvent();
    WindowInfo*info=getWindowInfo(event->window);
    if(info)
        setMapped(info,XCB_MAP_STATE_VIEWABLE);
    else
        xcb_map_window(dis, event->window);
}
void onUnmapEvent(){
    XUnmapEvent *event= getLastEvent();
    setMapped(getWindowInfo(event->window),XCB_MAP_STATE_UNMAPPED);
}
void onEnterEvent(){
    XIEnterEvent *event= getLastEvent();
    setActiveMasterByMouseId(event->deviceid);
    focusWindow(event->child);
}
void onFocusInEvent(){
    XIFocusInEvent *event= getLastEvent();
    LOG(LOG_LEVEL_DEBUG,"slave id %d\n",event->deviceid);
    setActiveMasterByKeyboardId(event->deviceid);
    //setBorder(event->child);
}
void onFocusOutEvent(){
    XIFocusOutEvent *event= getLastEvent();
    setActiveMasterByKeyboardId(event->deviceid);

    //resetBorder(event->child);
}

void onPropertyEvent(){
    XPropertyEvent *event=getLastEvent();
    loadWindowProperties(getWindowInfo(event->window));
}

void onClientMessage(){
    XClientMessageEvent *event=getLastEvent();
    long*data=event->data.l;
    Window win=event->window;
    Atom message=event->message_type;
    Node*head=getAllWindows();
    assert(getSize(head));

    WindowInfo*info=createWindowInfo(win);
    loadWindowProperties(info);
    dumpWindowInfo(info);
    if(message==ewmh->_NET_CURRENT_DESKTOP)
        activateWorkspace(data[0]);
    else if(message==ewmh->_NET_ACTIVE_WINDOW){
        //data[2] reqeuster's currently active window

        setActiveMasterNodeById(getNextMasterNodeFocusedOnWindow(data[2]));
        activateWindow(getWindowInfo(win));
    }
    else if(message== ewmh->_NET_SHOWING_DESKTOP)
        toggleShowDesktop();
    else if(message==ewmh->_NET_CLOSE_WINDOW)
        killWindow(win);
        //TODO handle
    //case ewmh->_NET_MOVERESIZE_WINDOW:
    /*
    else if(message==ewmh->_NET_WM_MOVERESIZE){
        int intData[5];
        for(int i=0;i<5;i++)
            intData[i]=data[1+i];
        moveResize(win,intData);
    }*/
    //change window's workspace
    else if(message==ewmh->_NET_WM_DESKTOP)
        moveWindowToWorkspace(getWindowInfo(win), data[0]);
    else if (message==ewmh->_NET_WM_STATE){
        dumpAtoms((xcb_atom_t *) &data[1], 2);
        setWindowStateFromAtomInfo(getWindowInfo(win),(xcb_atom_t *) &data[1], 2,data[0]);
    }
    /*
    //ignored (we are allowed to)
    case ewmh->_NET_NUMBER_OF_DESKTOPS:
    case ewmh->_NET_DESKTOP_GEOMETRY:
    case ewmh->_NET_DESKTOP_VIEWPORT:
    */
}

void onGenericEvent(){
    XEvent*event=getLastEvent();
    XGenericEventCookie *cookie = &event->xcookie;
    if(XGetEventData(dpy, cookie)){
        setLastEvent(cookie->data);
        LOG(LOG_LEVEL_TRACE,"generic event detected %d %s\n",cookie->evtype,genericEventTypeToString(cookie->evtype));
        applyGenericEventRules(cookie->evtype, NULL);
    }
    else LOG(LOG_LEVEL_ERROR,"could not get event data\n");
    XFreeEventData(dpy, cookie);
}

