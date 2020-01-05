#include <assert.h>

#include "bindings.h"
#include "devices.h"
#include "globals.h"
#include "layouts.h"
#include "logger.h"
#include "masters.h"
#include "monitors.h"
#include "mywm-structs.h"
#include "state.h"
#include "system.h"
#include "time.h"
#include "user-events.h"
#include "window-properties.h"
#include "windows.h"
#include "wm-rules.h"
#include "wmfunctions.h"
#include "workspaces.h"
#include "xsession.h"

void onXConnect(void) {
    addDefaultMaster();
    initCurrentMasters();
    assert(getActiveMaster() != NULL);
    detectMonitors();
    registerForEvents();
    scan(root);
}

void onHiearchyChangeEvent(void) {
    xcb_input_hierarchy_event_t* event = (xcb_input_hierarchy_event_t*)getLastEvent();
    if(event->flags & (XCB_INPUT_HIERARCHY_MASK_MASTER_ADDED | XCB_INPUT_HIERARCHY_MASK_SLAVE_ADDED)) {
        LOG(LOG_LEVEL_DEBUG, "detected new master\n");
        initCurrentMasters();
        return;
    }
    xcb_input_hierarchy_info_iterator_t iter = xcb_input_hierarchy_infos_iterator(event);
    while(iter.rem) {
        if(iter.data->flags & XCB_INPUT_HIERARCHY_MASK_MASTER_REMOVED) {
            LOG(LOG_LEVEL_DEBUG, "Master %d %d has been removed\n", iter.data->deviceid, iter.data->attachment);
            delete getAllMasters().removeElement(iter.data->deviceid);
        }
        else if(iter.data->flags & XCB_INPUT_HIERARCHY_MASK_SLAVE_REMOVED) {
            LOG(LOG_LEVEL_DEBUG, "Slave %d %d has been removed\n", iter.data->deviceid, iter.data->attachment);
            delete getAllSlaves().removeElement(iter.data->deviceid);
        }
        else if(iter.data->flags & XCB_INPUT_HIERARCHY_MASK_SLAVE_ATTACHED) {
            getAllSlaves().find(iter.data->deviceid)->setMasterID(iter.data->attachment);
        }
        else if(iter.data->flags & XCB_INPUT_HIERARCHY_MASK_SLAVE_DETACHED) {
            getAllSlaves().find(iter.data->deviceid)->setMasterID(0);
        }
        xcb_input_hierarchy_info_next(&iter);
    }
}

void onError(void) {
    LOG(LOG_LEVEL_ERROR, "error received in event loop\n");
    logError((xcb_generic_error_t*)getLastEvent());
}
void onConfigureNotifyEvent(void) {
    xcb_configure_notify_event_t* event = (xcb_configure_notify_event_t*)getLastEvent();
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo) {
        winInfo->setGeometry(&event->x);
        applyEventRules(WINDOW_MOVE, winInfo);
    }
    if(event->window == root)
        applyEventRules(SCREEN_CHANGE);
}
void onConfigureRequestEvent(void) {
    xcb_configure_request_event_t* event = (xcb_configure_request_event_t*)getLastEvent();
    short values[5];
    int n = 0;
    for(uint8_t i = 0U; i < LEN(values); i++)
        if(event->value_mask & (1 << i))
            values[n++] = (&event->x)[i];
    processConfigureRequest(event->window, values, event->sibling, event->stack_mode, event->value_mask);
}
bool addDoNotManageOverrideRedirectWindowsRule(AddFlag flag) {
    return getEventRules(PRE_REGISTER_WINDOW).add({
        +[](WindowInfo * winInfo) {if(winInfo->isOverrideRedirectWindow())winInfo->setNotManageable();},
        FUNC_NAME,
    }, flag);
}
bool addIgnoreOverrideRedirectWindowsRule(AddFlag flag) {
    return getEventRules(PRE_REGISTER_WINDOW).add({
        +[](WindowInfo * winInfo) {return !winInfo->isOverrideRedirectWindow();},
        FUNC_NAME,
        PASSTHROUGH_IF_TRUE
    }, flag);
}
void onCreateEvent(void) {
    xcb_create_notify_event_t* event = (xcb_create_notify_event_t*)getLastEvent();
    LOG(LOG_LEVEL_TRACE, "Detected create event for Window %d\n", event->window);
    if(getWindowInfo(event->window)) {
        LOG(LOG_LEVEL_TRACE, "Window %d is already in our records; Ignoring create event\n", event->window);
        return;
    }
    if(registerWindow(event->window, event->parent)) {
        WindowInfo* winInfo = getWindowInfo(event->window);
        winInfo->setGeometry(&event->x);
        applyEventRules(WINDOW_MOVE, winInfo);
    }
}
void onDestroyEvent(void) {
    xcb_destroy_notify_event_t* event = (xcb_destroy_notify_event_t*)getLastEvent();
    LOG(LOG_LEVEL_TRACE, "Detected destroy event for Window %d\n", event->window);
    unregisterWindow(getWindowInfo(event->window), 1);
}
void onVisibilityEvent(void) {
    xcb_visibility_notify_event_t* event = (xcb_visibility_notify_event_t*)getLastEvent();
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo)
        if(event->state == XCB_VISIBILITY_FULLY_OBSCURED)
            winInfo->removeMask(FULLY_VISIBLE_MASK);
        else if(event->state == XCB_VISIBILITY_UNOBSCURED)
            winInfo->addMask(FULLY_VISIBLE_MASK);
        else {
            winInfo->removeMask(FULLY_VISIBLE_MASK);
            winInfo->addMask(PARTIALLY_VISIBLE_MASK);
        }
}
void onMapEvent(void) {
    xcb_map_notify_event_t* event = (xcb_map_notify_event_t*)getLastEvent();
    LOG(LOG_LEVEL_TRACE, "Detected map event for Window %d\n", event->window);
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo) {
        bool alreadlyMapped = winInfo->hasMask(MAPPABLE_MASK);
        winInfo->addMask(MAPPABLE_MASK | MAPPED_MASK);
        if(!alreadlyMapped)
            applyEventRules(CLIENT_MAP_ALLOW, winInfo);
    }
}
void onMapRequestEvent(void) {
    xcb_map_request_event_t* event = (xcb_map_request_event_t*)getLastEvent();
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo) {
        winInfo->addMask(MAPPABLE_MASK);
        applyEventRules(CLIENT_MAP_ALLOW, winInfo);
        if(winInfo->getWorkspace())
            return;
    }
    mapWindow(event->window);
}
void onUnmapEvent(void) {
    xcb_unmap_notify_event_t* event = (xcb_unmap_notify_event_t*)getLastEvent();
    LOG(LOG_LEVEL_TRACE, "Detected unmap event for Window %d\n", event->window);
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo) {
        winInfo->removeMask(FULLY_VISIBLE_MASK | MAPPED_MASK);
        // used to indicate that the window is no longer mappable
        if(isSyntheticEvent() || winInfo->isOverrideRedirectWindow())
            winInfo->removeMask(MAPPABLE_MASK);
    }
}
void onReparentEvent(void) {
    xcb_reparent_notify_event_t* event = (xcb_reparent_notify_event_t*)getLastEvent();
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo)
        winInfo->setParent(event->parent);
    else if(registerWindow(event->window, event->parent)) {
        WindowInfo* winInfo = getWindowInfo(event->window);
        applyEventRules(WINDOW_MOVE, winInfo);
    }
}

void onFocusInEvent(void) {
    xcb_input_focus_in_event_t* event = (xcb_input_focus_in_event_t*)getLastEvent();
    setActiveMasterByDeviceID(event->deviceid);
    WindowInfo* winInfo = getWindowInfo(event->event);
    LOG(LOG_LEVEL_DEBUG, "Window %d was focused\n", event->event);
    if(winInfo && winInfo->isFocusAllowed()) {
        if(!winInfo->hasMask(NO_RECORD_FOCUS_MASK))
            getActiveMaster()->onWindowFocus(winInfo->getID());
        winInfo->removeMask(URGENT_MASK);
        setBorder(winInfo->getID());
    }
}

void onFocusOutEvent(void) {
    xcb_input_focus_out_event_t* event = (xcb_input_focus_out_event_t*)getLastEvent();
    setActiveMasterByDeviceID(event->deviceid);
    WindowInfo* winInfo = getWindowInfo(event->event);
    LOG(LOG_LEVEL_DEBUG, "Window %d was unfocused\n", event->event);
    if(winInfo)
        resetBorder(winInfo->getID());
}

void onPropertyEvent(void) {
    xcb_property_notify_event_t* event = (xcb_property_notify_event_t*)getLastEvent();
    WindowInfo* winInfo = getWindowInfo(event->window);
    // only reload properties if a window is mapped
    if(winInfo && winInfo->isMappable()) {
        if(event->atom == ewmh->_NET_WM_NAME || event->atom == WM_NAME)
            loadWindowTitle(winInfo);
        else if(event->atom == WM_HINTS)
            loadWindowHints(winInfo);
        else {
            LOG_RUN(LOG_LEVEL_DEBUG, dumpAtoms(&event->atom, 1));
            //loadWindowProperties(winInfo);
        }
    }
}


static WindowInfo* getTargetWindow(int root, int event, int child) {
    int i;
    int list[] = {0, root, event, child};
    for(i = LEN(list) - 1; i >= 1 && !list[i]; i--);
    return getWindowInfo(list[i]);
}
void addApplyBindingsRule(AddFlag flag) {
    getEventRules(DEVICE_EVENT).add({+[]{return checkBindings(getLastUserEvent());}, DEFAULT_EVENT_NAME(checkBindings)},
        flag);
}
void onDeviceEvent(void) {
    xcb_input_key_press_event_t* event = (xcb_input_key_press_event_t*)getLastEvent();
    LOG(LOG_LEVEL_TRACE, "device event seq: %d type: %d id %d (%d) flags %d windows: %d %d %d\n",
        event->sequence, event->event_type, event->deviceid, event->sourceid, event->flags,
        event->root, event->event, event->child);
    setActiveMasterByDeviceID(event->deviceid);
    setLastUserEvent({event->mods.effective, event->detail, 1U << event->event_type,
            (bool)((event->flags & XCB_INPUT_KEY_EVENT_FLAGS_KEY_REPEAT) ? 1 : 0),
            .winInfo = getTargetWindow(event->root, event->event, event->child),
        });
    applyEventRules(DEVICE_EVENT, NULL);
}

void onGenericEvent(void) {
    int type = loadGenericEvent((xcb_ge_generic_event_t*)getLastEvent());
    if(type)
        applyEventRules(type, NULL);
}

bool onSelectionClearEvent(void) {
    xcb_selection_clear_event_t* event = (xcb_selection_clear_event_t*)getLastEvent();
    if(event->owner == getPrivateWindow() && event->selection == WM_SELECTION_ATOM) {
        LOG(LOG_LEVEL_INFO, "We lost the WM_SELECTION; another window manager is taking over (%d)", event->owner);
        requestShutdown();
        return 0;
    }
    return 1;
}


void registerForEvents() {
    if(ROOT_EVENT_MASKS)
        registerForWindowEvents(root, ROOT_EVENT_MASKS);
    ArrayList<Binding*>& list = getDeviceBindings();
    LOG(LOG_LEVEL_DEBUG, "Grabbing %d buttons/keys\n", list.size());
    for(Binding* binding : list) {
        binding->grab();
    }
    LOG(LOG_LEVEL_DEBUG, "listening for device event; masks: %d\n", ROOT_DEVICE_EVENT_MASKS);
    if(ROOT_DEVICE_EVENT_MASKS)
        passiveGrab(root, ROOT_DEVICE_EVENT_MASKS);
}
bool listenForNonRootEventsFromWindow(WindowInfo* winInfo) {
    if(winInfo->isNotManageable())
        return 1;
    uint32_t mask = winInfo->getEventMasks() ? winInfo->getEventMasks() : NON_ROOT_EVENT_MASKS;
    bool result = 0;
    if(registerForWindowEvents(winInfo->getID(), mask) == 0) {
        result = 1;
        uint32_t deviceMask = winInfo->getDeviceEventMasks() ? winInfo->getDeviceEventMasks() : NON_ROOT_DEVICE_EVENT_MASKS;
        passiveGrab(winInfo->getID(), deviceMask);
        LOG(LOG_LEVEL_DEBUG, "Listening for events %d on %d\n", deviceMask, winInfo->getID());
    }
    return result;
}

void addAutoTileRules(AddFlag flag) {
    int events[] = {CLIENT_MAP_ALLOW, XCB_UNMAP_NOTIFY, XCB_DESTROY_NOTIFY,
            DEVICE_EVENT, XCB_CLIENT_MESSAGE, WINDOW_WORKSPACE_CHANGE,
            MONITOR_WORKSPACE_CHANGE, SCREEN_CHANGE,
        };
    for(auto event : events)
        getEventRules(event).add(DEFAULT_EVENT(markState), flag);
    getBatchEventRules(X_CONNECTION).add(PASSTHROUGH_EVENT(updateState, ALWAYS_PASSTHROUGH), flag);
    getEventRules(PERIODIC).add(PASSTHROUGH_EVENT(updateState, ALWAYS_PASSTHROUGH), flag);
    getEventRules(TILE_WORKSPACE).add(DEFAULT_EVENT(unmarkState), flag);
}
void assignDefaultLayoutsToWorkspace() {
    for(Workspace* w : getAllWorkspaces())
        if(w->getLayouts().size() == 0 && w->getActiveLayout() == NULL) {
            for(Layout* layout : getRegisteredLayouts())
                w->getLayouts().add(layout);
            w->setActiveLayout(getRegisteredLayouts()[0]);
        }
}
void updateMapState(Workspace* w) {
    for(WindowInfo* winInfo : w->getWindowStack())
        if((winInfo->isMappable() && (winInfo->getWorkspace()->isVisible() ^ winInfo->hasMask(MAPPED_MASK))))
            updateWindowWorkspaceState(winInfo);
}

void addSyncMapStateRules(AddFlag flag) {
    getEventRules(MONITOR_WORKSPACE_CHANGE).add(DEFAULT_EVENT(syncMappedState), flag);
    getEventRules(WINDOW_WORKSPACE_CHANGE).add(DEFAULT_EVENT(updateWindowWorkspaceState), flag);
    getEventRules(TILE_WORKSPACE).add(DEFAULT_EVENT(updateMapState), flag);
}

void addBasicRules(AddFlag flag) {
    getEventRules(0).add(DEFAULT_EVENT(onError), flag);
    getEventRules(XCB_VISIBILITY_NOTIFY).add(DEFAULT_EVENT(onVisibilityEvent), flag);
    getEventRules(XCB_CREATE_NOTIFY).add(DEFAULT_EVENT(onCreateEvent), flag);
    getEventRules(XCB_DESTROY_NOTIFY).add(DEFAULT_EVENT(onDestroyEvent), flag);
    getEventRules(XCB_UNMAP_NOTIFY).add(DEFAULT_EVENT(onUnmapEvent), flag);
    getEventRules(XCB_MAP_NOTIFY).add(DEFAULT_EVENT(onMapEvent), flag);
    getEventRules(XCB_MAP_REQUEST).add(DEFAULT_EVENT(onMapRequestEvent), flag);
    getEventRules(XCB_REPARENT_NOTIFY).add(DEFAULT_EVENT(onReparentEvent), flag);
    getEventRules(XCB_CONFIGURE_REQUEST).add(DEFAULT_EVENT(onConfigureRequestEvent), flag);
    getEventRules(XCB_CONFIGURE_NOTIFY).add(DEFAULT_EVENT(onConfigureNotifyEvent), flag);
    getEventRules(XCB_PROPERTY_NOTIFY).add(DEFAULT_EVENT(onPropertyEvent), flag);
    getEventRules(XCB_SELECTION_CLEAR).add(DEFAULT_EVENT(onSelectionClearEvent), flag);
    getEventRules(XCB_GE_GENERIC).add(DEFAULT_EVENT(onGenericEvent), flag);
    getEventRules(XCB_INPUT_FOCUS_IN + GENERIC_EVENT_OFFSET).add(DEFAULT_EVENT(onFocusInEvent), flag);
    getEventRules(XCB_INPUT_FOCUS_OUT + GENERIC_EVENT_OFFSET).add(DEFAULT_EVENT(onFocusOutEvent), flag);
    getEventRules(XCB_INPUT_HIERARCHY + GENERIC_EVENT_OFFSET).add(DEFAULT_EVENT(onHiearchyChangeEvent), flag);
    getEventRules(X_CONNECTION).add(DEFAULT_EVENT(onXConnect), flag);
    getEventRules(X_CONNECTION).add(DEFAULT_EVENT(assignDefaultLayoutsToWorkspace), flag);
    getEventRules(CLIENT_MAP_ALLOW).add(DEFAULT_EVENT(loadWindowProperties), flag);
    // From the ICCCM spec,
    // "The user will not be able to move, resize, restack, or transfer the input
    // focus to override-redirect windows, since the window manager is not managing them"
    // so it seems like a bug for a OR window to have INPUT_MASK set
    getEventRules(CLIENT_MAP_ALLOW).add(DEFAULT_EVENT(+[](WindowInfo * winInfo) { if(winInfo->isNotManageable()) winInfo->removeMask(INPUT_MASK);}),
    flag);
    addSyncMapStateRules(flag);
    addIgnoreOverrideRedirectWindowsRule(flag);
    addDoNotManageOverrideRedirectWindowsRule(flag);
    getEventRules(PRE_REGISTER_WINDOW).add(DEFAULT_EVENT(listenForNonRootEventsFromWindow), flag);
    addApplyBindingsRule(flag);
    getBatchEventRules(SCREEN_CHANGE).add(DEFAULT_EVENT(detectMonitors), flag);
    getBatchEventRules(SCREEN_CHANGE).add(DEFAULT_EVENT(resizeAllMonitorsToAvoidAllDocks), flag);
    for(int i = XCB_INPUT_KEY_PRESS; i <= XCB_INPUT_MOTION; i++) {
        getEventRules(GENERIC_EVENT_OFFSET + i).add(DEFAULT_EVENT(onDeviceEvent), flag);
    }
}

