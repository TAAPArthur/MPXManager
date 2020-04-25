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
    lowerWindow(getWindowDivider(1));
    raiseWindow(getWindowDivider(0));
    addDefaultMaster();
    initCurrentMasters();
    assert(getActiveMaster() != NULL);
    detectMonitors();
    assignUnusedMonitorsToWorkspaces();
    applyEventRules(SCREEN_CHANGE);
    registerForEvents();
    scan(root);
}

void onHiearchyChangeEvent(void) {
    xcb_input_hierarchy_event_t* event = (xcb_input_hierarchy_event_t*)getLastEvent();
    if(event->flags & (XCB_INPUT_HIERARCHY_MASK_MASTER_ADDED | XCB_INPUT_HIERARCHY_MASK_SLAVE_ADDED)) {
        DEBUG("detected new master");
        initCurrentMasters();
        return;
    }
    xcb_input_hierarchy_info_iterator_t iter = xcb_input_hierarchy_infos_iterator(event);
    while(iter.rem) {
        if(iter.data->flags & XCB_INPUT_HIERARCHY_MASK_MASTER_REMOVED) {
            LOG(LOG_LEVEL_DEBUG, "Master %d %d has been removed", iter.data->deviceid, iter.data->attachment);
            delete getAllMasters().removeElement(iter.data->deviceid);
        }
        else if(iter.data->flags & XCB_INPUT_HIERARCHY_MASK_SLAVE_REMOVED) {
            LOG(LOG_LEVEL_DEBUG, "Slave %d %d has been removed", iter.data->deviceid, iter.data->attachment);
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
    ERROR("error received in event loop");
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
bool addDoNotManageOverrideRedirectWindowsRule(bool remove) {
    return getEventRules(PRE_REGISTER_WINDOW).add({
        +[](WindowInfo * winInfo) {if(winInfo->isOverrideRedirectWindow())winInfo->setNotManageable();},
        FUNC_NAME,
    }, remove);
}
bool addIgnoreOverrideRedirectWindowsRule(bool remove) {
    return getEventRules(PRE_REGISTER_WINDOW).add({
        +[](WindowInfo * winInfo) {return !winInfo->isOverrideRedirectWindow();},
        FUNC_NAME,
        PASSTHROUGH_IF_TRUE,
        HIGH_PRIORITY,
    }, remove);
}

bool addIgnoreInputOnlyWindowsRule(bool remove) {
    return getEventRules(PRE_REGISTER_WINDOW).add({
        +[](WindowInfo * winInfo) {return !winInfo->isInputOnly();},
        FUNC_NAME,
        PASSTHROUGH_IF_TRUE,
        HIGH_PRIORITY
    }, remove);
}
void onCreateEvent(void) {
    xcb_create_notify_event_t* event = (xcb_create_notify_event_t*)getLastEvent();
    LOG(LOG_LEVEL_TRACE, "Detected create event for Window %d", event->window);
    if(getWindowInfo(event->window)) {
        LOG(LOG_LEVEL_TRACE, "Window %d is already in our records; Ignoring create event", event->window);
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
    LOG(LOG_LEVEL_TRACE, "Detected destroy event for Window %d", event->window);
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
    LOG(LOG_LEVEL_TRACE, "Detected map event for Window %d", event->window);
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo) {
        bool alreadlyMapped = winInfo->hasMask(MAPPABLE_MASK);
        winInfo->addMask(MAPPABLE_MASK | MAPPED_MASK);
        if(!alreadlyMapped)
            applyEventRules(CLIENT_MAP_ALLOW, winInfo);
        if(winInfo->isDock())
            applyEventRules(SCREEN_CHANGE);
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
    LOG(LOG_LEVEL_TRACE, "Detected unmap event for Window %d", event->window);
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo) {
        updateFocusForAllMasters(winInfo);
        winInfo->removeMask(FULLY_VISIBLE_MASK | MAPPED_MASK);
        // used to indicate that the window is no longer mappable
        if(isSyntheticEvent() || winInfo->isOverrideRedirectWindow()) {
            INFO("Marking window as withdrawn " << event->window);
            winInfo->removeMask(MAPPABLE_MASK);
            applyEventRules(CLIENT_MAP_DISALLOW, winInfo);
        }
        if(winInfo->isDock())
            applyEventRules(SCREEN_CHANGE);
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
    LOG(LOG_LEVEL_DEBUG, "Window %d was focused", event->event);
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
    LOG(LOG_LEVEL_DEBUG, "Window %d was unfocused", event->event);
    if(winInfo)
        resetBorder(winInfo->getID());
}

void onPropertyEvent(void) {
    xcb_property_notify_event_t* event = (xcb_property_notify_event_t*)getLastEvent();
    WindowInfo* winInfo = getWindowInfo(event->window);
    // only reload properties if a window is mapped
    if(winInfo && winInfo->isMappable()) {
        if(event->atom == ewmh->_NET_WM_NAME || event->atom == XCB_ATOM_WM_NAME)
            loadWindowTitle(winInfo);
        if(event->atom == ewmh->_NET_WM_STRUT || event->atom == ewmh->_NET_WM_STRUT_PARTIAL) {
            if(winInfo->isDock())
                loadDockProperties(winInfo);
        }
        else if(event->atom == XCB_ATOM_WM_HINTS)
            loadWindowHints(winInfo);
        else {
            DEBUG(getAtomsAsString(&event->atom, 1));
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
void addApplyBindingsRule(bool remove) {
    getEventRules(DEVICE_EVENT).add({+[]{return checkBindings(getLastUserEvent());}, DEFAULT_EVENT_NAME(checkBindings)},
        remove);
}
void onDeviceEvent(void) {
    xcb_input_key_press_event_t* event = (xcb_input_key_press_event_t*)getLastEvent();
    LOG(LOG_LEVEL_TRACE, "device event seq: %d type: %d id %d (%d) flags %d windows: %d %d %d",
        event->sequence, event->event_type, event->deviceid, event->sourceid, event->flags,
        event->root, event->event, event->child);
    setActiveMasterByDeviceID(event->deviceid);
    setLastUserEvent({(Modifier)event->mods.effective, (Detail)event->detail, 1U << event->event_type,
            (bool)((event->flags & XCB_INPUT_KEY_EVENT_FLAGS_KEY_REPEAT) ? 1 : 0),
            .mode = getActiveMaster()->getCurrentMode(),
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
        INFO("We lost the WM_SELECTION; another window manager is taking over: " << event->owner);
        requestShutdown();
        return 0;
    }
    return 1;
}

void grabDeviceBindings() {
    for(Binding* binding : getDeviceBindings())
        binding->grab();
    LOG(LOG_LEVEL_DEBUG, "Grabbing %d buttons/keys", getDeviceBindings().size());
}

void registerForEvents() {
    if(ROOT_EVENT_MASKS)
        registerForWindowEvents(root, ROOT_EVENT_MASKS);
    grabDeviceBindings();
    LOG(LOG_LEVEL_DEBUG, "listening for device event; masks: %d", ROOT_DEVICE_EVENT_MASKS);
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
        LOG(LOG_LEVEL_DEBUG, "Listening for events %d on %d", deviceMask, winInfo->getID());
    }
    return result;
}

void addAutoTileRules(bool remove) {
    getEventRules(PERIODIC).add(PASSTHROUGH_EVENT(updateState, ALWAYS_PASSTHROUGH), remove);
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
        if(winInfo->isOutOfSyncWithWorkspace())
            updateWindowWorkspaceState(winInfo);
}

void addSyncMapStateRules(bool remove) {
    getEventRules(MONITOR_WORKSPACE_CHANGE).add(DEFAULT_EVENT(syncMappedState), remove);
    getEventRules(WINDOW_WORKSPACE_CHANGE).add(DEFAULT_EVENT(updateWindowWorkspaceState), remove);
    getEventRules(TILE_WORKSPACE).add(DEFAULT_EVENT(updateMapState), remove);
}

void setDefaultStackPosition(WindowInfo* winInfo) {
    if(winInfo->getParent() == root)
        raiseWindow(winInfo, 0, !winInfo->hasMask(BELOW_MASK));
}

void addBasicRules(bool remove) {
    getEventRules(0).add(DEFAULT_EVENT(onError), remove);
    getEventRules(IDLE).add(DEFAULT_EVENT([] {applyEventRules(PERIODIC);}), remove);
    getEventRules(XCB_VISIBILITY_NOTIFY).add(DEFAULT_EVENT(onVisibilityEvent), remove);
    getEventRules(XCB_CREATE_NOTIFY).add(DEFAULT_EVENT(onCreateEvent), remove);
    getEventRules(XCB_DESTROY_NOTIFY).add(DEFAULT_EVENT(onDestroyEvent), remove);
    getEventRules(XCB_UNMAP_NOTIFY).add(DEFAULT_EVENT(onUnmapEvent), remove);
    getEventRules(XCB_MAP_NOTIFY).add(DEFAULT_EVENT(onMapEvent), remove);
    getEventRules(XCB_MAP_REQUEST).add(DEFAULT_EVENT(onMapRequestEvent), remove);
    getEventRules(XCB_REPARENT_NOTIFY).add(DEFAULT_EVENT(onReparentEvent), remove);
    getEventRules(XCB_CONFIGURE_REQUEST).add(DEFAULT_EVENT(onConfigureRequestEvent), remove);
    getEventRules(XCB_CONFIGURE_NOTIFY).add(DEFAULT_EVENT(onConfigureNotifyEvent), remove);
    getEventRules(XCB_PROPERTY_NOTIFY).add(DEFAULT_EVENT(onPropertyEvent), remove);
    getEventRules(XCB_SELECTION_CLEAR).add(DEFAULT_EVENT(onSelectionClearEvent), remove);
    getEventRules(XCB_GE_GENERIC).add(DEFAULT_EVENT(onGenericEvent), remove);
    getEventRules(XCB_INPUT_FOCUS_IN + GENERIC_EVENT_OFFSET).add(DEFAULT_EVENT(onFocusInEvent), remove);
    getEventRules(XCB_INPUT_FOCUS_OUT + GENERIC_EVENT_OFFSET).add(DEFAULT_EVENT(onFocusOutEvent), remove);
    getEventRules(XCB_INPUT_HIERARCHY + GENERIC_EVENT_OFFSET).add(DEFAULT_EVENT(onHiearchyChangeEvent), remove);
    getEventRules(X_CONNECTION).add(DEFAULT_EVENT(onXConnect, HIGH_PRIORITY), remove);
    getEventRules(X_CONNECTION).add(DEFAULT_EVENT(assignDefaultLayoutsToWorkspace, HIGH_PRIORITY), remove);
    getEventRules(CLIENT_MAP_ALLOW).add(DEFAULT_EVENT(loadWindowProperties), remove);
    // From the ICCCM spec,
    // "The user will not be able to move, resize, restack, or transfer the input
    // focus to override-redirect windows, since the window manager is not managing them"
    // so it seems like a bug for a OR window to have INPUT_MASK set
    getEventRules(CLIENT_MAP_ALLOW).add(DEFAULT_EVENT(+[](WindowInfo * winInfo) { if(winInfo->isNotManageable()) winInfo->removeMask(INPUT_MASK);}),
    remove);
    addSyncMapStateRules(remove);
    addDoNotManageOverrideRedirectWindowsRule(remove);
    addIgnoreInputOnlyWindowsRule(remove);
    addIgnoreOverrideRedirectWindowsRule(remove);
    getEventRules(PRE_REGISTER_WINDOW).add(DEFAULT_EVENT(listenForNonRootEventsFromWindow), remove);
    getEventRules(POST_REGISTER_WINDOW).add(DEFAULT_EVENT(setDefaultStackPosition), remove);
    addApplyBindingsRule(remove);
    getBatchEventRules(SCREEN_CHANGE).add(DEFAULT_EVENT(detectMonitors, HIGH_PRIORITY), remove);
    getBatchEventRules(SCREEN_CHANGE).add(DEFAULT_EVENT(resizeAllMonitorsToAvoidAllDocks), remove);
    getBatchEventRules(SCREEN_CHANGE).add(DEFAULT_EVENT(assignUnusedMonitorsToWorkspaces, LOW_PRIORITY), remove);
    for(int i = XCB_INPUT_KEY_PRESS; i <= XCB_INPUT_MOTION; i++) {
        getEventRules(GENERIC_EVENT_OFFSET + i).add(DEFAULT_EVENT(onDeviceEvent), remove);
    }
}

