
///\cond
#include <assert.h>


#include <X11/keysym.h>
#include <xcb/xproto.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/randr.h>
#include <xcb/xinput.h>
///\endcond

#include "events.h"
#include "bindings.h"
#include "wmfunctions.h"
#include "devices.h"
#include "globals.h"
#include "logger.h"
#include "mywm-util.h"
#include "state.h"
#include "monitors.h"
#include "default-rules.h"

#define MOVE_WINDOW_WITH_MOUSE(mod,btn)\
        AUTO_CHAIN_GRAB(XCB_INPUT_XI_EVENT_MASK_MOTION,\
        {-1,-1,BIND(floatWindow).noGrab=1},        \
        {mod,btn,BIND(moveWindowWithMouse)},\
        END_CHAIN(anyModifer,0)\
        );




void addRule(int i,Rule*rule){
    insertHead(eventRules[i], rule);
}
void clearAllRules(void){
    for(unsigned int i=0;i<NUMBER_OF_EVENT_RULES;i++)
        clearList(eventRules[i]);
}
void tileChangeWorkspaces(void){
    updateState(tileWorkspace);
}
void addAutoTileRules(void){
    static Rule autoTileRule=CREATE_DEFAULT_EVENT_RULE(markState);
    static Rule tileWorkspace=CREATE_DEFAULT_EVENT_RULE(tileChangeWorkspaces);
    int events[]={XCB_MAP_NOTIFY,XCB_UNMAP_NOTIFY,
            XCB_INPUT_KEY_PRESS+GENERIC_EVENT_OFFSET,XCB_INPUT_KEY_RELEASE+GENERIC_EVENT_OFFSET,
            XCB_INPUT_BUTTON_PRESS+GENERIC_EVENT_OFFSET,XCB_INPUT_BUTTON_RELEASE+GENERIC_EVENT_OFFSET
    };
    for(int i=0;i<LEN(events);i++)
        addRule(events[i],&autoTileRule);
    addRule(Idle,&tileWorkspace);
}

void addDefaultDeviceRules(void){
    static Rule deviceEventRule = CREATE_DEFAULT_EVENT_RULE(onDeviceEvent);
    for(int i=XCB_INPUT_KEY_PRESS;i<=XCB_INPUT_BUTTON_RELEASE;i++){
        addRule(GENERIC_EVENT_OFFSET+i,&deviceEventRule);
    }
}



void addAvoidDocksRule(void){
    static Rule avoidDocksRule = CREATE_RULE("_NET_WM_WINDOW_TYPE_DOCK",TYPE|LITERAL,BIND(addDock));
    addRule(ProcessingWindow, &avoidDocksRule);
}


void onXConnect(void){
    scan();
    for(int i=0;i<getNumberOfWorkspaces();i++)
        tileWorkspace(i);
}

void onHiearchyChangeEvent(void){
    xcb_input_hierarchy_event_t*event=getLastEvent();

    if(event->flags & XCB_INPUT_HIERARCHY_MASK_MASTER_ADDED|XCB_INPUT_HIERARCHY_MASK_MASTER_REMOVED){

        xcb_input_hierarchy_info_iterator_t iter = xcb_input_hierarchy_infos_iterator(event);
        while(iter.rem){

            if(iter.data->type==XCB_INPUT_DEVICE_TYPE_MASTER_KEYBOARD && iter.data->flags & XCB_INPUT_HIERARCHY_MASK_MASTER_ADDED){
                LOG(LOG_LEVEL_DEBUG,"detected new master %d %d\n",iter.data->deviceid, iter.data->attachment);
                addMaster(iter.data->deviceid, iter.data->attachment);
            }
            else if(iter.data->flags & XCB_INPUT_HIERARCHY_MASK_MASTER_REMOVED){
                LOG(LOG_LEVEL_DEBUG,"Master %d %d has been removed\n",iter.data->deviceid, iter.data->attachment);
                removeMaster(iter.data->deviceid);
            }

            xcb_input_hierarchy_info_next(&iter);
        }
    }
}

void onError(void){
    LOG(LOG_LEVEL_ERROR,"error received in event loop\n");
    logError(getLastEvent());
}
void onConfigureNotifyEvent(void){
    xcb_configure_notify_event_t *event=getLastEvent();
    setGeometry(getWindowInfo(event->window), &event->x);
}
void onConfigureRequestEvent(void){
    xcb_configure_request_event_t *event=getLastEvent();
    applyGravity(event->window,&event->x, 0);
    processConfigureRequest(event->window,&event->x,event->sibling,event->stack_mode,event->value_mask);
}
void onCreateEvent(void){
    LOG(LOG_LEVEL_TRACE,"create event received\n");
    xcb_create_notify_event_t *event= getLastEvent();
    if(event->override_redirect)return;
    if(getWindowInfo(event->window))
        return;
    WindowInfo*winInfo=createWindowInfo(event->window);
    LOG(LOG_LEVEL_TRACE,"window: %d parent: %d\n",event->window,event->parent);
    processNewWindow(winInfo);
}
void onDestroyEvent(void){
    LOG(LOG_LEVEL_TRACE,"destroy event received\n");
    xcb_destroy_notify_event_t *event= getLastEvent();
    LOG(LOG_LEVEL_TRACE,"destroy event received %d %d\n",event->event,event->window);
    deleteWindow(event->window);
}
void onVisibilityEvent(void){
    LOG(LOG_LEVEL_TRACE,"made it to visibility request event\n");
    xcb_visibility_notify_event_t *event= getLastEvent();
    WindowInfo*winInfo=getWindowInfo(event->window);
    if(winInfo)
        if(event->state==XCB_VISIBILITY_FULLY_OBSCURED)
            removeMask(winInfo, FULLY_VISIBLE);
        else if(event->state==XCB_VISIBILITY_UNOBSCURED)
            addMask(winInfo, FULLY_VISIBLE);
        else{
            removeMask(winInfo, FULLY_VISIBLE);
            addMask(winInfo, PARTIALLY_VISIBLE);
        }
}
void onMapEvent(void){
    xcb_map_notify_event_t*event= getLastEvent();

    updateMapState(event->window,1);
}
void onMapRequestEvent(void){
    xcb_map_request_event_t*event= getLastEvent();
    WindowInfo*winInfo=getWindowInfo(event->window);
    if(winInfo)
        addMask(winInfo, MAPABLE_MASK);
    attemptToMapWindow(event->window);
}
void onUnmapEvent(void){
    xcb_unmap_notify_event_t*event= getLastEvent();
    updateMapState(event->window,0);
}


void onFocusInEvent(void){
    xcb_input_focus_in_event_t*event= getLastEvent();
    LOG(LOG_LEVEL_DEBUG,"id %d window %d %d\n",event->deviceid,event->event,event->event);
    setActiveMasterByDeviceId(event->deviceid);
    onWindowFocus(event->event);
    setActiveWindowProperty(event->event);
    //setBorder(event->child);
}
void onFocusOutEvent(void){
    //xcb_input_focus_out_event_t *event= getLastEvent();
    //TODO setActiveMasterByMouseId(event->deviceid);
    //resetBorder(event->child);
}

void onPropertyEvent(void){
    xcb_property_notify_event_t*event=getLastEvent();
    if(getWindowInfo(event->window))
        loadWindowProperties(getWindowInfo(event->window));
}

void onClientMessage(void){
    xcb_client_message_event_t*event=getLastEvent();
    LOG(LOG_LEVEL_DEBUG,"Received client message/request \n\n");
    xcb_client_message_data_t data=event->data;
    xcb_window_t win=event->window;
    Atom message=event->type;

    if(message==ewmh->_NET_CURRENT_DESKTOP)
        activateWorkspace(data.data32[0]);
    else if(message==ewmh->_NET_ACTIVE_WINDOW){
        //data: source,timestamp,current active window
        WindowInfo*winInfo=getWindowInfo(win);
        if(allowRequestFromSource(winInfo, data.data32[0])){
            if(data.data32[2])//use the current master if not set
                setActiveMasterByDeviceId(getClientKeyboard(data.data32[2]));
            LOG(LOG_LEVEL_DEBUG,"Activating window %d for master %d due to client request\n",win,getActiveMaster()->id);
            activateWindow(winInfo);
        }
    }
    else if(message== ewmh->_NET_SHOWING_DESKTOP){
        LOG(LOG_LEVEL_DEBUG,"Chainign showing desktop to %d\n\n",data.data32[0]);
        setShowingDesktop(getActiveWorkspaceIndex(), data.data32[0]);
    }
    else if(message==ewmh->_NET_CLOSE_WINDOW){
        LOG(LOG_LEVEL_DEBUG,"Killing window %d\n\n",win);
        WindowInfo*winInfo=getWindowInfo(win);
        if(allowRequestFromSource(winInfo, data.data32[0])){
            killWindow(win);
        }
    }
    else if(message== ewmh->_NET_RESTACK_WINDOW){
        WindowInfo*winInfo=getWindowInfo(win);
        if(allowRequestFromSource(winInfo, data.data32[0]))
            processConfigureRequest(win, NULL, data.data32[1], data.data32[2], XCB_CONFIG_WINDOW_STACK_MODE|XCB_CONFIG_WINDOW_SIBLING);
    }
    else if(message== ewmh->_NET_REQUEST_FRAME_EXTENTS){
        xcb_ewmh_set_frame_extents(ewmh, win, DEFAULT_BORDER_WIDTH, DEFAULT_BORDER_WIDTH, DEFAULT_BORDER_WIDTH, DEFAULT_BORDER_WIDTH);
    }
    else if(message==ewmh->_NET_MOVERESIZE_WINDOW){
        int gravity=data.data8[0];
        int mask=data.data8[1] & 15;
        int source = (data.data8[1] >>4) & 15;
        short values[4];
        for(int i=1;i<=4;i++)
            values[i-1]=data.data32[i];

        WindowInfo*winInfo=getWindowInfo(win);
        if(allowRequestFromSource(winInfo, source)){
            applyGravity(win,values, gravity);
            processConfigureRequest(win, values, 0, 0, mask);
        }

    }/*
    else if(message==ewmh->_NET_WM_MOVERESIZE){
        int intData[5];
        for(int i=0;i<5;i++)
            intData[i]=data[1+i];
        moveResize(win,intData);
    }*/
    //change window's workspace
    else if(message==ewmh->_NET_WM_DESKTOP){
        LOG(LOG_LEVEL_DEBUG,"Changing window workspace %d\n\n",data.data32[0]);
        WindowInfo*winInfo=getWindowInfo(win);
        if(allowRequestFromSource(winInfo, data.data32[1]))
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

void setLastKnowMasterPosition(int x,int y){
    getActiveMaster()->mousePos[0]=x;
    getActiveMaster()->mousePos[1]=y;
}
void onDeviceEvent(void){
    xcb_input_key_press_event_t*event=getLastEvent();
    LOG(LOG_LEVEL_DEBUG,"device event %d %d %d %d\n\n",event->event_type,event->deviceid,event->sourceid,event->flags);

    if((event->flags&XCB_INPUT_KEY_EVENT_FLAGS_KEY_REPEAT) && IGNORE_KEY_REPEAT)
        return;



    setActiveMasterByDeviceId(event->deviceid);
    setLastKnowMasterPosition(event->root_x,event->root_y);
    checkBindings(event->detail,event->mods.effective,
            1<<event->event_type,
            getWindowInfo(event->child));
}

void onGenericEvent(void){
    applyRules(eventRules[loadGenericEvent(getLastEvent())], NULL);
}



void onStartup(void){
    init();
    addDefaultRules();
    createContext(NUMBER_OF_WORKSPACES);
    if(startUpMethod)
        startUpMethod();
    connectToXserver();
}


void addBasicRules(void){
    static Rule NORMAL_RULES[NUMBER_OF_EVENT_RULES] = {
        [0]= CREATE_DEFAULT_EVENT_RULE(onError),
        [XCB_VISIBILITY_NOTIFY] = CREATE_DEFAULT_EVENT_RULE(onVisibilityEvent),
        [XCB_CREATE_NOTIFY] = CREATE_DEFAULT_EVENT_RULE(onCreateEvent),

        [XCB_DESTROY_NOTIFY] = CREATE_DEFAULT_EVENT_RULE(onDestroyEvent),
        [XCB_UNMAP_NOTIFY] = CREATE_DEFAULT_EVENT_RULE(onUnmapEvent),
        [XCB_MAP_NOTIFY] = CREATE_DEFAULT_EVENT_RULE(onMapEvent),
        [XCB_MAP_REQUEST] = CREATE_DEFAULT_EVENT_RULE(onMapRequestEvent),
        [XCB_CONFIGURE_REQUEST] = CREATE_DEFAULT_EVENT_RULE(onConfigureRequestEvent),
        [XCB_CONFIGURE_NOTIFY] = CREATE_DEFAULT_EVENT_RULE(onConfigureNotifyEvent),


        [XCB_PROPERTY_NOTIFY] = CREATE_DEFAULT_EVENT_RULE(onPropertyEvent),
        [XCB_CLIENT_MESSAGE] = CREATE_DEFAULT_EVENT_RULE(onClientMessage),

        [XCB_GE_GENERIC] = CREATE_DEFAULT_EVENT_RULE(onGenericEvent),

        [XCB_INPUT_FOCUS_IN + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onFocusInEvent),
        [XCB_INPUT_FOCUS_OUT + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onFocusOutEvent),
        [XCB_INPUT_HIERARCHY + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onHiearchyChangeEvent),

        [onXConnection] = CREATE_DEFAULT_EVENT_RULE(onXConnect)
    };
    for(unsigned int i=0;i<NUMBER_OF_EVENT_RULES;i++)
        if(NORMAL_RULES[i].onMatch.type)
            insertHead(eventRules[i],&NORMAL_RULES[i]);
}

void addDefaultRules(void){

    addBasicRules();
    addDefaultDeviceRules();
    addAvoidDocksRule();
    addAutoTileRules();
}