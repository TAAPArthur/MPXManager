/**
 * @file default-rules.c
 * @copybrief default-rules.h
 *
 */

#include <assert.h>


#include "bindings.h"
#include "default-rules.h"
#include "devices.h"
#include "events.h"
#include "globals.h"
#include "layouts.h"
#include "logger.h"
#include "masters.h"
#include "monitors.h"
#include "mywm-util.h"
#include "state.h"
#include "windows.h"
#include "wmfunctions.h"
#include "workspaces.h"
#include "xsession.h"

static Rule ignoreUnknownWindowRule = CREATE_WILDCARD(BIND(hasMask, IMPLICIT_TYPE), .passThrough = PASSTHROUGH_IF_FALSE,
                                      .negateResult = 1);
void addUnknownWindowRule(void){
    prependToList(getEventRules(RegisteringWindow), &ignoreUnknownWindowRule);
}
void addUnknownWindowIgnoreRule(void){
    prependToList(getEventRules(ProcessingWindow), &ignoreUnknownWindowRule);
}
static void tileChangeWorkspaces(void){
    updateState(tileWorkspace, syncMonitorMapState);
}
void autoAddToWorkspace(WindowInfo* winInfo){
    int index = LOAD_SAVED_STATE ? getSavedWorkspaceIndex(winInfo->id) : getActiveWorkspaceIndex();
    moveWindowToWorkspace(winInfo, index);
}
void addAutoTileRules(void){
    static Rule autoTileRule = CREATE_DEFAULT_EVENT_RULE(markState);
    static Rule clearStateRule = CREATE_DEFAULT_EVENT_RULE(unmarkState);
    static Rule tileWorkspace = CREATE_DEFAULT_EVENT_RULE(tileChangeWorkspaces);
    int events[] = {XCB_MAP_NOTIFY, XCB_UNMAP_NOTIFY, XCB_DESTROY_NOTIFY,
                    XCB_INPUT_KEY_PRESS + GENERIC_EVENT_OFFSET, XCB_INPUT_KEY_RELEASE + GENERIC_EVENT_OFFSET,
                    XCB_INPUT_BUTTON_PRESS + GENERIC_EVENT_OFFSET, XCB_INPUT_BUTTON_RELEASE + GENERIC_EVENT_OFFSET,
                    XCB_CLIENT_MESSAGE,
                    onScreenChange,
                   };
    for(int i = 0; i < LEN(events); i++)
        prependToList(getEventRules(events[i]), &autoTileRule);
    addToList(getEventRules(onXConnection), &tileWorkspace);
    addToList(getEventRules(Periodic), &tileWorkspace);
    addToList(getEventRules(TileWorkspace), &clearStateRule);
}

void addDefaultDeviceRules(void){
    static Rule deviceEventRule = CREATE_DEFAULT_EVENT_RULE(onDeviceEvent);
    for(int i = XCB_INPUT_KEY_PRESS; i <= XCB_INPUT_MOTION; i++){
        addToList(getEventRules(GENERIC_EVENT_OFFSET + i), &deviceEventRule);
    }
}

static void printStatus(void){
    if(printStatusMethod && STATUS_FD)
        printStatusMethod();
}
void addPrintRule(void){
    static Rule printRule = CREATE_DEFAULT_EVENT_RULE(printStatus);
    addToList(getEventRules(Idle), &printRule);
}
static Rule desktopRule = {"_NET_WM_WINDOW_TYPE_DESKTOP", TYPE | LITERAL, BOTH(BIND(addMask, NO_ACTIVATE_MASK | NO_RECORD_FOCUS | IGNORE_WORKSPACE_MASKS_MASK | NO_TILE_MASK | MAXIMIZED_MASK | BELOW_MASK | STICKY_MASK), BIND(enableTilingOverride, 3)), .passThrough = NO_PASSTHROUGH};
void addDesktopRule(void){
    addToList(getEventRules(RegisteringWindow), &desktopRule);
}
void addFloatRules(void){
    static Rule dialogRule = {"_NET_WM_WINDOW_TYPE_NORMAL", TYPE | NEGATE | LITERAL, BIND(floatWindow)};
    addToList(getEventRules(RegisteringWindow), &dialogRule);
}

static Rule avoidDocksRule = {"_NET_WM_WINDOW_TYPE_DOCK", TYPE | LITERAL, BOTH(BIND(loadDockProperties), BIND(markAsDock), BIND(addMask, EXTERNAL_CONFIGURABLE_MASK))};
void addAvoidDocksRule(void){
    addToList(getEventRules(PropertyLoad), &avoidDocksRule);
}
void addNoDockFocusRule(void){
    static Rule disallowDocksFocusRule = {"_NET_WM_WINDOW_TYPE_DOCK", TYPE | LITERAL, BIND(removeMask, INPUT_MASK)};
    addToList(getEventRules(PropertyLoad), &disallowDocksFocusRule);
}
void addFocusFollowsMouseRule(void){
    NON_ROOT_DEVICE_EVENT_MASKS |= XCB_INPUT_XI_EVENT_MASK_ENTER;
    static Rule focusFollowsMouseRule = CREATE_DEFAULT_EVENT_RULE(focusFollowMouse);
    addToList(getEventRules(GENERIC_EVENT_OFFSET + XCB_INPUT_ENTER), &focusFollowsMouseRule);
}

void onXConnect(void){
    scan(root);
    //for each master try to make set the focused window to the currently focused window (or closet one by id)
    FOR_EACH(Master*, master, getAllMasters()){
        setActiveMaster(master);
        onWindowFocus(getActiveFocus(master->id));
    }
}

void onHiearchyChangeEvent(void){
    xcb_input_hierarchy_event_t* event = getLastEvent();
    if(event->flags & XCB_INPUT_HIERARCHY_MASK_MASTER_ADDED){
        LOG(LOG_LEVEL_DEBUG, "detected new master\n");
        initCurrentMasters();
    }
    if(event->flags & XCB_INPUT_HIERARCHY_MASK_MASTER_REMOVED){
        xcb_input_hierarchy_info_iterator_t iter = xcb_input_hierarchy_infos_iterator(event);
        while(iter.rem){
            if(iter.data->flags & XCB_INPUT_HIERARCHY_MASK_MASTER_REMOVED){
                LOG(LOG_LEVEL_DEBUG, "Master %d %d has been removed\n", iter.data->deviceid, iter.data->attachment);
                removeMaster(iter.data->deviceid);
            }
            xcb_input_hierarchy_info_next(&iter);
        }
    }
}

void onError(void){
    LOG(LOG_LEVEL_ERROR, "error received in event loop\n");
    logError(getLastEvent());
}
void onConfigureNotifyEvent(void){
    xcb_configure_notify_event_t* event = getLastEvent();
    if(event->override_redirect)return;
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo){
        setGeometry(winInfo, &event->x);
        applyEventRules(onWindowMove, winInfo);
    }
}
void onConfigureRequestEvent(void){
    xcb_configure_request_event_t* event = getLastEvent();
    short values[5];
    int n = 0;
    for(int i = 0; i < LEN(values); i++)
        if(event->value_mask & (1 << i))
            values[n++] = (&event->x)[i];
    processConfigureRequest(event->window, values, event->sibling, event->stack_mode, event->value_mask);
}
static void onWindowDetection(WindowID id, WindowID parent, short* geo){
    WindowInfo* winInfo = createWindowInfo(id);
    winInfo->creationTime = getTime();
    winInfo->parent = parent;
    if(processNewWindow(winInfo)){
        if(geo)
            setGeometry(winInfo, geo);
        applyEventRules(onWindowMove, winInfo);
    }
}
void onCreateEvent(void){
    xcb_create_notify_event_t* event = getLastEvent();
    if(event->override_redirect || getWindowInfo(event->window))return;
    if(IGNORE_SUBWINDOWS && event->parent != root)return;
    onWindowDetection(event->window, event->parent, &event->x);
}
void onDestroyEvent(void){
    xcb_destroy_notify_event_t* event = getLastEvent();
    deleteWindow(event->window);
}
void onVisibilityEvent(void){
    xcb_visibility_notify_event_t* event = getLastEvent();
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo)
        if(event->state == XCB_VISIBILITY_FULLY_OBSCURED)
            removeMask(winInfo, FULLY_VISIBLE);
        else if(event->state == XCB_VISIBILITY_UNOBSCURED)
            addMask(winInfo, FULLY_VISIBLE);
        else {
            removeMask(winInfo, FULLY_VISIBLE);
            addMask(winInfo, PARTIALLY_VISIBLE);
        }
}
void onMapEvent(void){
    xcb_map_notify_event_t* event = getLastEvent();
    updateMapState(event->window, 1);
}
void onMapRequestEvent(void){
    xcb_map_request_event_t* event = getLastEvent();
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo){
        loadWindowProperties(winInfo);
        addMask(winInfo, MAPPABLE_MASK);
    }
    attemptToMapWindow(event->window);
}
void onUnmapEvent(void){
    xcb_unmap_notify_event_t* event = getLastEvent();
    updateMapState(event->window, 0);
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo){
        removeMask(winInfo, FULLY_VISIBLE);
        if(isSyntheticEvent())
            removeMask(winInfo, MAPPABLE_MASK);
    }
}
void onReparentEvent(void){
    xcb_reparent_notify_event_t* event = getLastEvent();
    if(IGNORE_SUBWINDOWS && !event->override_redirect){
        if(event->parent == root)
            onWindowDetection(event->window, event->parent, NULL);
        else
            deleteWindow(event->window);
    }
}

void focusFollowMouse(void){
    xcb_input_enter_event_t* event = getLastEvent();
    setActiveMasterByDeviceId(event->deviceid);
    WindowID win = event->event;
    WindowInfo* winInfo = getWindowInfo(win);
    LOG(LOG_LEVEL_DEBUG, "focus following mouse %d win %d\n", getActiveMaster()->id, win);
    if(winInfo)
        focusWindowInfo(winInfo);
}
void onFocusInEvent(void){
    xcb_input_focus_in_event_t* event = getLastEvent();
    setActiveMasterByDeviceId(event->deviceid);
    WindowInfo* winInfo = getWindowInfo(event->event);
    if(winInfo){
        if(!hasMask(winInfo, NO_RECORD_FOCUS))
            updateFocusState(winInfo);
        setBorder(winInfo->id);
    }
    setActiveWindowProperty(event->event);
}

void onFocusOutEvent(void){
    xcb_input_focus_out_event_t* event = getLastEvent();
    setActiveMasterByDeviceId(event->deviceid);
    WindowInfo* winInfo = getWindowInfo(event->event);
    if(winInfo)
        resetBorder(winInfo->id);
}

void onPropertyEvent(void){
    xcb_property_notify_event_t* event = getLastEvent();
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo){
        if(event->atom == ewmh->_NET_WM_NAME || event->atom == ewmh->_NET_WM_VISIBLE_NAME)
            loadTitleInfo(winInfo);
        else if(event->atom == ewmh->_NET_WM_STRUT || event->atom == ewmh->_NET_WM_STRUT_PARTIAL){
            loadDockProperties(winInfo);
            markState();
        }
    }
}

void onClientMessage(void){
    xcb_client_message_event_t* event = getLastEvent();
    xcb_client_message_data_t data = event->data;
    xcb_window_t win = event->window;
    Atom message = event->type;
    LOG(LOG_LEVEL_DEBUG, "Received client message/request for window: %d\n", win);
    if(message == ewmh->_NET_CURRENT_DESKTOP)
        switchToWorkspace(data.data32[0]);
    else if(message == ewmh->_NET_ACTIVE_WINDOW){
        //data: source,timestamp,current active window
        WindowInfo* winInfo = getWindowInfo(win);
        if(allowRequestFromSource(winInfo, data.data32[0])){
            if(data.data32[2])//use the current master if not set
                setActiveMasterByDeviceId(getClientKeyboard(data.data32[2]));
            LOG(LOG_LEVEL_DEBUG, "Activating window %d for master %d due to client request\n", win, getActiveMaster()->id);
            activateWindow(winInfo);
        }
    }
    else if(message == ewmh->_NET_SHOWING_DESKTOP){
        LOG(LOG_LEVEL_DEBUG, "Chaining showing desktop to %d\n\n", data.data32[0]);
        setShowingDesktop(getActiveWorkspaceIndex(), data.data32[0]);
    }
    else if(message == ewmh->_NET_CLOSE_WINDOW){
        LOG(LOG_LEVEL_DEBUG, "Killing window %d\n\n", win);
        WindowInfo* winInfo = getWindowInfo(win);
        if(!winInfo)
            killWindow(win);
        else if(allowRequestFromSource(winInfo, data.data32[0])){
            killWindowInfo(winInfo);
        }
    }
    else if(message == ewmh->_NET_RESTACK_WINDOW){
        WindowInfo* winInfo = getWindowInfo(win);
        LOG(LOG_LEVEL_TRACE, "Restacking Window %d sibling %d detail %d\n\n", win, data.data32[1], data.data32[2]);
        if(allowRequestFromSource(winInfo, data.data32[0]))
            processConfigureRequest(win, NULL, data.data32[1], data.data32[2],
                                    XCB_CONFIG_WINDOW_STACK_MODE | XCB_CONFIG_WINDOW_SIBLING);
    }
    else if(message == ewmh->_NET_REQUEST_FRAME_EXTENTS){
        xcb_ewmh_set_frame_extents(ewmh, win, DEFAULT_BORDER_WIDTH, DEFAULT_BORDER_WIDTH, DEFAULT_BORDER_WIDTH,
                                   DEFAULT_BORDER_WIDTH);
    }
    else if(message == ewmh->_NET_MOVERESIZE_WINDOW){
        LOG(LOG_LEVEL_DEBUG, "Move/Resize window request %d\n\n", win);
        int mask = data.data8[1] & 15;
        int source = (data.data8[1] >> 4) & 15;
        short values[4];
        for(int i = 1; i <= 4; i++)
            values[i - 1] = data.data32[i];
        WindowInfo* winInfo = getWindowInfo(win);
        if(allowRequestFromSource(winInfo, source)){
            processConfigureRequest(win, values, 0, 0, mask);
        }
    }/*
    else if(message==ewmh->_NET_WM_MOVERESIZE){
    }*/
    //change window's workspace
    else if(message == ewmh->_NET_WM_DESKTOP){
        LOG(LOG_LEVEL_DEBUG, "Changing window workspace %d\n\n", data.data32[0]);
        WindowInfo* winInfo = getWindowInfo(win);
        int destIndex = data.data32[0];
        if(allowRequestFromSource(winInfo, data.data32[1]))
            if(destIndex == -1){
                addMask(winInfo, STICKY_MASK);
                floatWindow(winInfo);
                xcb_ewmh_set_wm_desktop(ewmh, winInfo->id, destIndex);
            }
            else
                moveWindowToWorkspace(getWindowInfo(win), destIndex);
    }
    else if(message == ewmh->_NET_WM_STATE){
        LOG(LOG_LEVEL_DEBUG, "Settings client window manager state %d\n", data.data32[3]);
        WindowInfo* winInfo = getWindowInfo(win);
        int num = data.data32[2] == 0 ? 1 : 2;
        if(winInfo){
            if(allowRequestFromSource(winInfo, data.data32[3]))
                setWindowStateFromAtomInfo(winInfo, (xcb_atom_t*) &data.data32[1], num, data.data32[0]);
        }
        else {
            WindowInfo fakeWinInfo = {.mask = 0, .id = win};
            loadSavedAtomState(&fakeWinInfo);
            setWindowStateFromAtomInfo(&fakeWinInfo, (xcb_atom_t*) &data.data32[1], num, data.data32[0]);
        }
    }
    else if(message == ewmh->WM_PROTOCOLS){
        if(data.data32[0] == ewmh->_NET_WM_PING){
            if(win == root){
                LOG(LOG_LEVEL_DEBUG, "Pong received\n");
                WindowInfo* winInfo = getWindowInfo(data.data32[2]);
                if(winInfo){
                    LOG(LOG_LEVEL_DEBUG, "Updated ping timestamp for window %d\n\n", winInfo->id);
                    winInfo->pingTimeStamp = getTime();
                }
            }
        }
    }
    else if(message == ewmh->_NET_NUMBER_OF_DESKTOPS){
        if(data.data32[0] > 0){
            int delta = data.data32[0] - getNumberOfWorkspaces();
            if(delta > 0)
                addWorkspaces(delta);
            else {
                int newLastIndex = data.data32[0] - 1;
                if(getActiveWorkspaceIndex() > newLastIndex)
                    setActiveWorkspaceIndex(newLastIndex);
                for(int i = data.data32[0]; i < getNumberOfWorkspaces(); i++){
                    FOR_EACH_REVERSED(WindowInfo*, winInfo, getWindowStack(getWorkspaceByIndex(i))){
                        moveWindowToWorkspace(winInfo, newLastIndex);
                    }
                }
                removeWorkspaces(-delta);
            }
            assert(data.data32[0] == getNumberOfWorkspaces());
            assignUnusedMonitorsToWorkspaces();
        }
    }
    /*
    //ignored (we are allowed to)
    case ewmh->_NET_DESKTOP_GEOMETRY:
    case ewmh->_NET_DESKTOP_VIEWPORT:
    */
}

static WindowInfo* getTargetWindow(int root, int event, int child){
    int i;
    int list[] = {0, root, event, child};
    for(i = LEN(list) - 1; i >= 1 && !list[i]; i--);
    return getWindowInfo(list[i]);
}
void onDeviceEvent(void){
    xcb_input_key_press_event_t* event = getLastEvent();
    LOG(LOG_LEVEL_VERBOSE, "device event seq: %d type: %d id %d (%d) flags %d windows: %d %d %d\n",
        event->sequence, event->event_type, event->deviceid, event->sourceid, event->flags,
        event->root, event->event, event->child);
    if((event->flags & XCB_INPUT_KEY_EVENT_FLAGS_KEY_REPEAT) && IGNORE_KEY_REPEAT)
        return;
    setActiveMasterByDeviceId(event->deviceid);
    setLastKnowMasterPosition(event->root_x >> 16, event->root_y >> 16);
    checkBindings(event->detail, event->mods.effective,
                  1 << event->event_type,
                  getTargetWindow(event->root, event->event, event->child),
                  (event->flags & XCB_INPUT_KEY_EVENT_FLAGS_KEY_REPEAT) ? 1 : 0);
}

void onGenericEvent(void){
    int type = loadGenericEvent(getLastEvent());
    if(type)
        applyEventRules(type, NULL);
}

void onSelectionClearEvent(void){
    xcb_selection_clear_event_t* event = getLastEvent();
    if(event->owner == compliantWindowManagerIndicatorWindow  && event->selection == WM_SELECTION_ATOM){
        LOG(LOG_LEVEL_INFO, "We lost the WM_SELECTION; another window manager is taking over (%d)", event->owner);
        quit(0);
    }
}


void onStartup(void){
    resetContext();
    if(RUN_AS_WM)
        addDefaultRules();
    if(startUpMethod)
        startUpMethod();
    for(int i = 0; i < getNumberOfWorkspaces(); i++)
        if(!getActiveLayoutOfWorkspace(i))
            addLayoutsToWorkspace(i, DEFAULT_LAYOUTS, NUMBER_OF_DEFAULT_LAYOUTS);
    for(int i = 0; i < NUMBER_OF_EVENT_RULES; i++){
        FOR_EACH(Rule*, rule, getEventRules(i)) initRule(rule);
    }
    connectToXserver();
    if(postStartUpMethod)
        postStartUpMethod();
}

static int nonAlwaysOnTopOrBottomWindowMoved;
static void markAlwaysOnTop(WindowInfo* winInfo){
    if(!hasPartOfMask(winInfo, ALWAYS_ON_TOP | ALWAYS_ON_BOTTOM))
        nonAlwaysOnTopOrBottomWindowMoved = 1;
}
static void enforceAlwaysOnTop(void){
    if(nonAlwaysOnTopOrBottomWindowMoved){
        FOR_EACH(WindowInfo*, winInfo2, getAllWindows()){
            if(hasMask(winInfo2, ALWAYS_ON_BOTTOM))
                lowerWindowInfo(winInfo2);
        }
        FOR_EACH(WindowInfo*, winInfo2, getAllWindows()){
            if(hasMask(winInfo2, ALWAYS_ON_TOP))
                raiseWindowInfo(winInfo2);
        }
        nonAlwaysOnTopOrBottomWindowMoved = 0;
    }
}

static Rule NORMAL_RULES[NUMBER_OF_EVENT_RULES] = {
    [0] = CREATE_DEFAULT_EVENT_RULE(onError),
    [XCB_VISIBILITY_NOTIFY] = CREATE_DEFAULT_EVENT_RULE(onVisibilityEvent),
    [XCB_CREATE_NOTIFY] = CREATE_DEFAULT_EVENT_RULE(onCreateEvent),

    [XCB_DESTROY_NOTIFY] = CREATE_DEFAULT_EVENT_RULE(onDestroyEvent),
    [XCB_UNMAP_NOTIFY] = CREATE_DEFAULT_EVENT_RULE(onUnmapEvent),
    [XCB_MAP_NOTIFY] = CREATE_DEFAULT_EVENT_RULE(onMapEvent),
    [XCB_MAP_REQUEST] = CREATE_DEFAULT_EVENT_RULE(onMapRequestEvent),
    [XCB_REPARENT_NOTIFY] = CREATE_DEFAULT_EVENT_RULE(onReparentEvent),

    [XCB_CONFIGURE_REQUEST] = CREATE_DEFAULT_EVENT_RULE(onConfigureRequestEvent),
    [XCB_CONFIGURE_NOTIFY] = CREATE_DEFAULT_EVENT_RULE(onConfigureNotifyEvent),


    [XCB_PROPERTY_NOTIFY] = CREATE_DEFAULT_EVENT_RULE(onPropertyEvent),
    [XCB_SELECTION_CLEAR] = CREATE_DEFAULT_EVENT_RULE(onSelectionClearEvent),
    [XCB_CLIENT_MESSAGE] = CREATE_DEFAULT_EVENT_RULE(onClientMessage),

    [XCB_GE_GENERIC] = CREATE_DEFAULT_EVENT_RULE(onGenericEvent),

    [XCB_INPUT_FOCUS_IN + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onFocusInEvent),
    [XCB_INPUT_FOCUS_OUT + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onFocusOutEvent),
    [XCB_INPUT_HIERARCHY + GENERIC_EVENT_OFFSET] = CREATE_DEFAULT_EVENT_RULE(onHiearchyChangeEvent),

    [onXConnection] = CREATE_DEFAULT_EVENT_RULE(onXConnect),
    [RegisteringWindow] = CREATE_WILDCARD(OR(BIND(isDock), BIND(autoAddToWorkspace))),
    [onScreenChange] = CREATE_DEFAULT_EVENT_RULE(detectMonitors),
    [onWindowMove] = CREATE_DEFAULT_EVENT_RULE(markAlwaysOnTop),
};
static Rule BATCH_RULES[NUMBER_OF_EVENT_RULES] = {
    [onWindowMove] = CREATE_DEFAULT_EVENT_RULE(enforceAlwaysOnTop),
};

void addSomeBasicRules(int* arr, int len){
    for(unsigned int i = 0; i < len; i++)
        if(NORMAL_RULES[arr[i]].onMatch.type)
            addToList(getEventRules(arr[i]), &NORMAL_RULES[arr[i]]);
    for(unsigned int i = 0; i < len; i++)
        if(BATCH_RULES[arr[i]].onMatch.type)
            addToList(getEventRules(arr[i]), &BATCH_RULES[arr[i]]);
}
void addBasicRules(void){
    NON_ROOT_DEVICE_EVENT_MASKS |= XCB_INPUT_XI_EVENT_MASK_FOCUS_OUT | XCB_INPUT_XI_EVENT_MASK_FOCUS_IN;
    for(unsigned int i = 0; i < NUMBER_OF_EVENT_RULES; i++)
        if(NORMAL_RULES[i].onMatch.type)
            addToList(getEventRules(i), &NORMAL_RULES[i]);
    for(unsigned int i = 0; i < NUMBER_OF_EVENT_RULES; i++)
        if(BATCH_RULES[i].onMatch.type)
            addToList(getBatchEventRules(i), &BATCH_RULES[i]);
}

void addDieOnIdleRule(void){
    static Rule dieOnIdleRule = CREATE_WILDCARD(BIND(exit, 0));
    addToList(getEventRules(Idle), &dieOnIdleRule);
}

void addDefaultRules(void){
    addBasicRules();
    addAutoTileRules();
    addPrintRule();
    addDefaultDeviceRules();
    addAvoidDocksRule();
    addDesktopRule();
    addFloatRules();
}
