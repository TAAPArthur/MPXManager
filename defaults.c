
#include <assert.h>


#include <X11/keysym.h>
#include <xcb/xproto.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/randr.h>
#include <xcb/xinput.h>

#include "event-loop.h"
#include "defaults.h"
#include "bindings.h"
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

    [XCB_VISIBILITY_NOTIFY] = CREATE_DEFAULT_EVENT_RULE(onVisibilityEvent),
    [XCB_CREATE_NOTIFY] = CREATE_DEFAULT_EVENT_RULE(onCreateEvent),

    [XCB_DESTROY_NOTIFY] = CREATE_DEFAULT_EVENT_RULE(onDestroyEvent),
    [XCB_UNMAP_NOTIFY] = CREATE_DEFAULT_EVENT_RULE(onUnmapEvent),
    [XCB_MAP_REQUEST] = CREATE_DEFAULT_EVENT_RULE(onMapRequestEvent),
    [XCB_CONFIGURE_REQUEST] = CREATE_DEFAULT_EVENT_RULE(onConfigureRequestEvent),


    [XCB_PROPERTY_NOTIFY] = CREATE_DEFAULT_EVENT_RULE(onPropertyEvent),
    [XCB_CLIENT_MESSAGE] = CREATE_DEFAULT_EVENT_RULE(onClientMessage),

    [XCB_GE_GENERIC] = CREATE_DEFAULT_EVENT_RULE(onGenericEvent),

    [XCB_INPUT_DEVICE_KEY_PRESS + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onDeviceEvent),
    [XCB_INPUT_DEVICE_KEY_RELEASE + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onDeviceEvent),
    [XCB_INPUT_DEVICE_BUTTON_PRESS + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onDeviceEvent),
    [XCB_INPUT_DEVICE_BUTTON_RELEASE + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onDeviceEvent),
    [XCB_INPUT_ENTER + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onEnterEvent),
    [XCB_INPUT_DEVICE_FOCUS_IN + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onFocusInEvent),
    [XCB_INPUT_DEVICE_FOCUS_OUT + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onFocusOutEvent),
    [XCB_INPUT_XI_CHANGE_HIERARCHY + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onHiearchyChangeEvent),

};

void onStartup(){
    for(unsigned int i=0;i<NUMBER_OF_EVENT_RULES;i++)
        if(DEFAULT_RULES[i].onMatch.type)
            eventRules[i]=createHead(&DEFAULT_RULES[i]);
        else eventRules[i]=createEmptyHead();
}


void onHiearchyChangeEvent(){
    xcb_input_hierarchy_event_t*event=getLastEvent();
    if((event->flags & (XCB_INPUT_HIERARCHY_MASK_MASTER_ADDED |XCB_INPUT_HIERARCHY_MASK_MASTER_REMOVED)) ==0)
        return;
    /*TODO
    for (int i = 0; i < event->num_info; i++){
        if (event->info[i].flags & XIMasterRemoved)
            removeMaster(event->info[i].deviceid);
        else if (event->info[i].flags & XIMasterAdded)
            addMaster(event->info[i].deviceid);
    }
    */
}
void onDeviceEvent(){
    xcb_input_device_key_press_event_t*event=getLastEvent();

    LOG(LOG_LEVEL_TRACE,"deviec event %d %d %d\n",event->response_type,event->device_id);
    if(event->response_type<XCB_INPUT_DEVICE_BUTTON_PRESS){//key press/release
        setActiveMasterByKeyboardId(event->device_id);

        //setActiveMasterByKeyboardSlaveId(event->deviceid);
    }
    else{//button press/release
        //setActiveMasterByKeyboardSlaveId(event->deviceid);
        setActiveMasterByMouseId(event->device_id);
        setLastWindowClicked(event->child);
    }

    checkBindings(event->detail,event->state,event->response_type,
            getWindowInfo(event->child));
}

void onConfigureRequestEvent(){

    LOG(LOG_LEVEL_TRACE,"configure request received\n");
    xcb_configure_request_event_t *event=getLastEvent();
    processConfigureRequest(event->window,&event->x,event->sibling,event->stack_mode,event->value_mask);
}
void onCreateEvent(){

    LOG(LOG_LEVEL_TRACE,"create event received\n");
    xcb_create_notify_event_t *event= getLastEvent();
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
    xcb_destroy_notify_event_t *event= getLastEvent();
    deleteWindow(event->window);
}
void onVisibilityEvent(){
    LOG(LOG_LEVEL_TRACE,"made it to visibility request event\n");
    xcb_visibility_notify_event_t *event= getLastEvent();
    setVisible(getWindowInfo(event->window),event->state);

}
void onMapRequestEvent(){
    LOG(LOG_LEVEL_TRACE,"made it to map request event\n");
    xcb_map_request_event_t*event= getLastEvent();
    updateMapState(event->window,1);
}
void onUnmapEvent(){
    xcb_unmap_notify_event_t*event= getLastEvent();
    updateMapState(event->window,0);
}




void onEnterEvent(){
    xcb_input_enter_event_t*event= getLastEvent();
    setActiveMasterByMouseId(event->deviceid);
    focusWindow(event->child);
}
void onFocusInEvent(){
    xcb_input_focus_in_event_t*event= getLastEvent();
    LOG(LOG_LEVEL_DEBUG,"slave id %d\n",event->deviceid);
    setActiveMasterByKeyboardId(event->deviceid);
    //setBorder(event->child);
}
void onFocusOutEvent(){
    xcb_input_focus_out_event_t *event= getLastEvent();
    setActiveMasterByKeyboardId(event->deviceid);
    //resetBorder(event->child);
}

void onPropertyEvent(){
    xcb_property_notify_event_t*event=getLastEvent();
    loadWindowProperties(getWindowInfo(event->window));
}

void onClientMessage(){
    xcb_client_message_event_t*event=getLastEvent();
    xcb_client_message_data_t data=event->data;
    Window win=event->window;
    Atom message=event->type;
    Node*head=getAllWindows();
    assert(getSize(head));

    if(message==ewmh->_NET_CURRENT_DESKTOP)
        activateWorkspace(data.data32[0]);
    else if(message==ewmh->_NET_ACTIVE_WINDOW){
        //data[2] reqeuster's currently active window

        setActiveMasterNodeById(getNextMasterNodeFocusedOnWindow(data.data32[2]));
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
    else if(message==ewmh->_NET_WM_DESKTOP){
        if(data.data32[0]==NO_WORKSPACE)
            addState(getWindowInfo(win),STICKY_MASK);
        else
            moveWindowToWorkspace(getWindowInfo(win), data.data32[0]);
    }
    else if (message==ewmh->_NET_WM_STATE){
        dumpAtoms((xcb_atom_t *) &data.data32[1], 2);
        setWindowStateFromAtomInfo(getWindowInfo(win),(xcb_atom_t *) &data.data32[1], 2,data.data32[0]);
    }
    /*
    //ignored (we are allowed to)
    case ewmh->_NET_NUMBER_OF_DESKTOPS:
    case ewmh->_NET_DESKTOP_GEOMETRY:
    case ewmh->_NET_DESKTOP_VIEWPORT:
    */
}

void onGenericEvent(){
    xcb_ge_generic_event_t*event=getLastEvent();


    const xcb_query_extension_reply_t*reply=xcb_get_extension_data(dis, &xcb_input_id);

    if(event->extension == reply->major_opcode){
        LOG(LOG_LEVEL_TRACE,"generic event detected %d %s\n",event->response_type,genericEventTypeToString(event->response_type));
        applyEventRules(event->response_type+GENERIC_EVENT_OFFSET, NULL);
    }
    else LOG(LOG_LEVEL_ERROR,"could not get event data\n");
}

