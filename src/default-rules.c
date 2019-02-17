
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
#include "windows.h"
#include "devices.h"
#include "globals.h"
#include "logger.h"
#include "masters.h"
#include "workspaces.h"
#include "xsession.h"
#include "state.h"
#include "monitors.h"
#include "default-rules.h"
#include "layouts.h"

static void tileChangeWorkspaces(void){
    updateState(tileWorkspace);
}
void autoAddToWorkspace(WindowInfo*winInfo){
    if(winInfo->dock)return;    
    int index=LOAD_SAVED_STATE?getSavedWorkspaceIndex(winInfo->id):getActiveWorkspaceIndex();
    moveWindowToWorkspace(winInfo, index);
    //char state=XCB_ICCCM_WM_STATE_WITHDRAWN;
    //xcb_change_property(dis,XCB_PROP_MODE_REPLACE,win,ewmh->_NET_WM_STATE,ewmh->_NET_WM_STATE,32,0,&state);
}
void addAutoTileRules(void){
    static Rule autoTileRule=CREATE_DEFAULT_EVENT_RULE(markState);
    static Rule tileWorkspace=CREATE_DEFAULT_EVENT_RULE(tileChangeWorkspaces);
    int events[]={XCB_MAP_NOTIFY,XCB_UNMAP_NOTIFY,
            XCB_INPUT_KEY_PRESS+GENERIC_EVENT_OFFSET,XCB_INPUT_KEY_RELEASE+GENERIC_EVENT_OFFSET,
            XCB_INPUT_BUTTON_PRESS+GENERIC_EVENT_OFFSET,XCB_INPUT_BUTTON_RELEASE+GENERIC_EVENT_OFFSET,
            XCB_RANDR_NOTIFY_OUTPUT_CHANGE + MONITOR_EVENT_OFFSET,
    };
    for(int i=0;i<LEN(events);i++)
        appendRule(events[i],&autoTileRule);
    appendRule(Idle,&tileWorkspace);
}

void addDefaultDeviceRules(void){
    static Rule deviceEventRule = CREATE_DEFAULT_EVENT_RULE(onDeviceEvent);
    for(int i=XCB_INPUT_KEY_PRESS;i<=XCB_INPUT_MOTION;i++){
        appendRule(GENERIC_EVENT_OFFSET+i,&deviceEventRule);
    }
}
int isImplicitType(WindowInfo*winInfo){
   return winInfo->implicitType;
}

static void printStatus(void){
    if(printStatusMethod && STATUS_FD)
        printStatusMethod();
}
void addPrintRule(void){
    static Rule printRule=CREATE_DEFAULT_EVENT_RULE(printStatus);
    appendRule(Idle,&printRule);
}
void addIgnoreRule(void){
    
    static Rule ignoreRule= CREATE_WILDCARD(BIND(isImplicitType),.passThrough=PASSTHROUGH_IF_FALSE);
    appendRule(ProcessingWindow,&ignoreRule);
}
void addFloatRules(void){
    static Rule dialogRule=CREATE_LITERAL_RULE("_NET_WM_WINDOW_TYPE_DIALOG",TYPE,BIND(floatWindow),);
    appendRule(RegisteringWindow, &dialogRule);
    static Rule notificationRule=CREATE_LITERAL_RULE("_NET_WM_WINDOW_TYPE_NOTIFICATION",TYPE,BIND(floatWindow));
    appendRule(RegisteringWindow, &notificationRule);
}

static Rule avoidDocksRule = {"_NET_WM_WINDOW_TYPE_DOCK",TYPE|LITERAL,AND(BIND(loadDockProperties),BIND(markAsDock)),.passThrough=PASSTHROUGH_IF_TRUE};
void addAvoidDocksRule(void){
    appendRule(ProcessingWindow, &avoidDocksRule);
}
void addFocusFollowsMouseRule(void){

    static Rule focusFollowsMouseRule=CREATE_DEFAULT_EVENT_RULE(focusFollowMouse);
    NON_ROOT_DEVICE_EVENT_MASKS|=XCB_INPUT_XI_EVENT_MASK_ENTER;
    appendRule(GENERIC_EVENT_OFFSET+XCB_INPUT_ENTER,&focusFollowsMouseRule);
}

void onXConnect(void){
    scan(root);
    //for each master try to make set the focused window to the currently focused window (or closet one by id)
    FOR_EACH(Master*master,getAllMasters(),
        setActiveMaster(master);
        onWindowFocus(getActiveFocus(master->id));
    );
}

void onHiearchyChangeEvent(void){
    xcb_input_hierarchy_event_t*event=getLastEvent();

    if(event->flags & XCB_INPUT_HIERARCHY_MASK_MASTER_ADDED){
        LOG(LOG_LEVEL_DEBUG,"detected new master\n");
        initCurrentMasters();
    }
    if(event->flags & XCB_INPUT_HIERARCHY_MASK_MASTER_REMOVED){

        xcb_input_hierarchy_info_iterator_t iter = xcb_input_hierarchy_infos_iterator(event);
        while(iter.rem){
            if(iter.data->flags & XCB_INPUT_HIERARCHY_MASK_MASTER_REMOVED){
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
    if(event->override_redirect||getWindowInfo(event->window))return;
    if(IGNORE_SUBWINDOWS && event->parent!=root)return;
    WindowInfo*winInfo=createWindowInfo(event->window);
    winInfo->creationTime=getTime();
    winInfo->parent=event->parent;
    LOG(LOG_LEVEL_DEBUG,"window: %d parent: %d\n",event->window,event->parent);
    processNewWindow(winInfo);
}
void onDestroyEvent(void){
    LOG(LOG_LEVEL_TRACE,"destroy event received\n");
    xcb_destroy_notify_event_t *event= getLastEvent();
    LOG(LOG_LEVEL_TRACE,"destroy event received %d %d\n",event->event,event->window);
    deleteWindow(event->window);
}
void onVisibilityEvent(void){

    xcb_visibility_notify_event_t *event= getLastEvent();
    LOG(LOG_LEVEL_TRACE,"made it to visibility request event %d %d\n",event->window,event->state);
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
    WindowInfo*winInfo=getWindowInfo(event->window);
    if(winInfo){
        removeMask(winInfo, FULLY_VISIBLE);
        if(isSyntheticEvent())
            removeMask(winInfo, MAPABLE_MASK);
    }
}

void focusFollowMouse(void){
    xcb_input_enter_event_t*event= getLastEvent();
    setActiveMasterByDeviceId(event->deviceid);
    int win=event->event;
    WindowInfo*winInfo=getWindowInfo(win);
    LOG(LOG_LEVEL_DEBUG,"focus following mouse %d win %d\n",getActiveMaster()->id,win);
    if(winInfo)
        focusWindowInfo(winInfo);
    else focusWindow(win);
}
void onFocusInEvent(void){
    xcb_input_focus_in_event_t*event= getLastEvent();
    LOG(LOG_LEVEL_TRACE,"id %d window %d %d\n",event->deviceid,event->event,event->child);
    setActiveMasterByDeviceId(event->deviceid);
    WindowInfo*winInfo=getWindowInfo(event->event);
    WindowInfo*oldFocus=getFocusedWindow();
    if(winInfo){
        updateFocusState(winInfo);
        setBorder(winInfo);
    }
    if(oldFocus && oldFocus!=winInfo)
        resetBorder(oldFocus);
    setActiveWindowProperty(event->event);
    //setBorder(event->child);
}

void onPropertyEvent(void){
    xcb_property_notify_event_t*event=getLastEvent();
    if(getWindowInfo(event->window))
        loadWindowProperties(getWindowInfo(event->window));
    else processNewWindow(createWindowInfo(event->window));
}

void onClientMessage(void){
    xcb_client_message_event_t*event=getLastEvent();

    xcb_client_message_data_t data=event->data;
    xcb_window_t win=event->window;
    Atom message=event->type;

    LOG(LOG_LEVEL_DEBUG,"Received client message/request for window: %d\n",win);

    if(message==ewmh->_NET_CURRENT_DESKTOP)
        switchToWorkspace(data.data32[0]);
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
    else if(message== ewmh->_NET_SHOWING_DESKTOP){}
    else if(message==ewmh->_NET_CLOSE_WINDOW){
        LOG(LOG_LEVEL_DEBUG,"Killing window %d\n\n",win);
        WindowInfo*winInfo=getWindowInfo(win);
        if(!winInfo)
            killWindow(win);
        else if(allowRequestFromSource(winInfo, data.data32[0])){
            killWindowInfo(winInfo);
        }
    }
    else if(message== ewmh->_NET_RESTACK_WINDOW){
        WindowInfo*winInfo=getWindowInfo(win);
        LOG(LOG_LEVEL_TRACE,"Restacking Window %d sibling %d detail %d\n\n",win,data.data32[1], data.data32[2]);
        if(allowRequestFromSource(winInfo, data.data32[0]))
            processConfigureRequest(win, NULL, data.data32[1], data.data32[2], XCB_CONFIG_WINDOW_STACK_MODE|XCB_CONFIG_WINDOW_SIBLING);
    }
    else if(message== ewmh->_NET_REQUEST_FRAME_EXTENTS){
        xcb_ewmh_set_frame_extents(ewmh, win, DEFAULT_BORDER_WIDTH, DEFAULT_BORDER_WIDTH, DEFAULT_BORDER_WIDTH, DEFAULT_BORDER_WIDTH);
    }
    else if(message==ewmh->_NET_MOVERESIZE_WINDOW){
        LOG(LOG_LEVEL_TRACE,"Move/Resize window request %d\n\n",win);
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
        int destIndex=data.data32[0];
        if(allowRequestFromSource(winInfo, data.data32[1]))
            if(destIndex==-1){
                addMask(winInfo,STICKY_MASK);
                floatWindow(winInfo);
                xcb_ewmh_set_wm_desktop(ewmh, winInfo->id, destIndex);
            }
            else
                moveWindowToWorkspace(getWindowInfo(win), destIndex);
    }
    else if (message==ewmh->_NET_WM_STATE){
        LOG(LOG_LEVEL_DEBUG,"Settings client wm state\n");
        dumpAtoms((xcb_atom_t *) &data.data32[1], 2);
        WindowInfo*winInfo=getWindowInfo(win);
        if(winInfo){
            setWindowStateFromAtomInfo(winInfo,(xcb_atom_t *) &data.data32[1], 2,data.data32[0]);
        }
        else{
            WindowInfo fakeWinInfo={.mask=0,.id=win};
            loadSavedAtomState(&fakeWinInfo);
            setWindowStateFromAtomInfo(&fakeWinInfo,(xcb_atom_t *) &data.data32[1], 2,data.data32[0]);
        }
    }
    else if(message==ewmh->WM_PROTOCOLS){
        if(data.data32[0]==ewmh->_NET_WM_PING){
            if(win==root){
                LOG(LOG_LEVEL_DEBUG,"Pong received\n");
                WindowInfo*winInfo=getWindowInfo(data.data32[2]);
                if(winInfo){
                    LOG(LOG_LEVEL_DEBUG,"Updated ping timestamp for window %d\n\n",winInfo->id);
                    winInfo->pingTimeStamp=getTime();
                }
            }
        }
    }
    /*
    //ignored (we are allowed to)
    case ewmh->_NET_NUMBER_OF_DESKTOPS:
    case ewmh->_NET_DESKTOP_GEOMETRY:
    case ewmh->_NET_DESKTOP_VIEWPORT:
    */
}

WindowInfo* getTargetWindow(int root,int event,int child){
    Master*master=getActiveMaster();
    int i;
    int list[]={0,root,event,child,master->targetWindow};
    for(i=LEN(list)-1;i>=1&&!list[i];i--);
    return getWindowInfo(list[i]);
}
void onDeviceEvent(void){
    xcb_input_key_press_event_t*event=getLastEvent();
    LOG(LOG_LEVEL_DEBUG,"device event seq: %d type: %d id %d (%d) flags %d windows: %d %d %d\n",
        event->sequence,event->event_type,event->deviceid,event->sourceid,event->flags,
        event->root,event->event,event->child);

    if((event->flags&XCB_INPUT_KEY_EVENT_FLAGS_KEY_REPEAT) && IGNORE_KEY_REPEAT)
        return;

    setActiveMasterByDeviceId(event->deviceid);
    setLastKnowMasterPosition(event->root_x>>16,event->root_y>>16);
    checkBindings(event->detail,event->mods.effective,
            1<<event->event_type,
            getTargetWindow(event->root,event->event,event->child));
}

void onGenericEvent(void){
    applyRules(getEventRules(loadGenericEvent(getLastEvent())), NULL);
}



void onStartup(void){
    if(preStartUpMethod)
        preStartUpMethod();
    addDefaultRules();
    //TODO find way to customize number of workspaces in config
    resetContext();
    if(startUpMethod)
        startUpMethod();

    for(int i=0;i<getNumberOfWorkspaces();i++)
        if(!getActiveLayoutOfWorkspace(i))
            addLayoutsToWorkspace(i,DEFAULT_LAYOUTS,NUMBER_OF_DEFAULT_LAYOUTS);

    for(int i=0;i<NUMBER_OF_EVENT_RULES;i++){
        FOR_EACH(Rule*rule,getEventRules(i),initRule(rule));
    }
    connectToXserver();
}

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
    [XCB_INPUT_HIERARCHY + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onHiearchyChangeEvent),
    [XCB_RANDR_NOTIFY_OUTPUT_CHANGE + MONITOR_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(detectMonitors),

    [onXConnection] = CREATE_DEFAULT_EVENT_RULE(onXConnect),
    [RegisteringWindow] =CREATE_WILDCARD(AND(BIND(autoAddToWorkspace),BIND(updateEWMHClientList))),
    [TileWorkspace] = CREATE_DEFAULT_EVENT_RULE(unmarkState),
};

void addBasicRules(void){
    for(unsigned int i=0;i<NUMBER_OF_EVENT_RULES;i++)
        if(NORMAL_RULES[i].onMatch.type)
            addToList(getEventRules(i),&NORMAL_RULES[i]);
}


void addDefaultRules(void){

    addBasicRules();
    addDefaultDeviceRules();
    addAvoidDocksRule();
    addAutoTileRules();
    addPrintRule();
    addIgnoreRule();
    addFloatRules();
}
