#include <assert.h>

#include "bindings.h"
#include "boundfunction.h"
#include "devices.h"
#include "xutil/device-grab.h"
#include "globals.h"
#include "layouts.h"
#include "util/logger.h"
#include "masters.h"
#include "monitors.h"
#include "mywm-structs.h"
#include "system.h"
#include "time.h"
#include "user-events.h"
#include "xutil/window-properties.h"
#include "windows.h"
#include "wm-rules.h"
#include "wmfunctions.h"
#include "workspaces.h"
#include "xevent.h"

void initState() {
    addDefaultMaster();
    initCurrentMasters();
    assert(getActiveMaster() != NULL);
    detectMonitors();
}
void onXConnect(void) {
    applyEventRules(SCREEN_CHANGE, NULL);
    ownSelection(MPX_WM_SELECTION_ATOM);
    registerForEvents();
    getWindowDivider(0);
    scan(root);
}

void onHiearchyChangeEvent(void) {
    /* TODO renable
    xcb_input_hierarchy_event_t* event = (xcb_input_hierarchy_event_t*)getLastEvent();
    if(event->flags & (XCB_INPUT_HIERARCHY_MASK_MASTER_ADDED | XCB_INPUT_HIERARCHY_MASK_SLAVE_ADDED)) {
        DEBUG("detected new master");
        initCurrentMasters();
        return;
    }
    xcb_input_hierarchy_info_iterator_t iter = xcb_input_hierarchy_infos_iterator(event);
    while(iter.rem) {
        if(iter.data->flags & XCB_INPUT_HIERARCHY_MASK_MASTER_REMOVED) {
            DEBUG("Master %d %d has been removed", iter.data->deviceid, iter.data->attachment);
            delete getAllMasters().removeElement(iter.data->deviceid);
        }
        else if(iter.data->flags & XCB_INPUT_HIERARCHY_MASK_SLAVE_REMOVED) {
            DEBUG("Slave %d %d has been removed", iter.data->deviceid, iter.data->attachment);
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
    */
}


void onConfigureNotifyEvent(xcb_configure_notify_event_t* event) {
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo) {
        setGeometry(winInfo, &event->x);
        applyEventRules(WINDOW_MOVE, winInfo);
    }
    if(event->window == root)
        applyEventRules(SCREEN_CHANGE, NULL);
}
void onConfigureRequestEvent(xcb_configure_request_event_t* event) {
    short values[5];
    int n = 0;
    for(uint8_t i = 0U; i < LEN(values); i++)
        if(event->value_mask & (1 << i))
            values[n++] = (&event->x)[i];
    processConfigureRequest(event->window, values, event->sibling, event->stack_mode, event->value_mask);
}
void onCreateEvent(xcb_create_notify_event_t* event) {
    TRACE("Detected create event for Window %d", event->window);
    if(getWindowInfo(event->window)) {
        TRACE("Window %d is already in our records; Ignoring create event", event->window);
        return;
    }
    if(registerWindow(event->window, event->parent, NULL)) {
        WindowInfo* winInfo = getWindowInfo(event->window);
        setGeometry(winInfo, &event->x);
        applyEventRules(WINDOW_MOVE, winInfo);
        if(!hasMask(winInfo, ABOVE_MASK))
            raiseWindowInfo(winInfo, 0);
    }
}
void onDestroyEvent(xcb_destroy_notify_event_t* event) {
    TRACE("Detected destroy event for Window %d", event->window);
    unregisterWindow(getWindowInfo(event->window), 0);
}


void onDeviceEvent(xcb_input_key_press_event_t* event) {
    TRACE("device event seq: %d type: %d id %d (%d) flags %d windows: %d %d %d Detail %d Mod %d (%d)",
        event->sequence, event->event_type, event->deviceid, event->sourceid, event->flags,
        event->root, event->event, event->child, event->detail, event->mods.effective, event->mods.base);
    setActiveMasterByDeviceID(event->deviceid);
    int i;
    int list[] = {0, event->root, event->event, event->child};
    WindowInfo* winInfo = NULL;
    for(i = LEN(list) - 1; i >= 1 && !list[i] && !winInfo; i--);
    winInfo = getWindowInfo(list[i]);
    BindingEvent bindingEvent = {(Modifier)event->mods.effective, (Detail)event->detail, 1U << event->event_type,
                     (bool)((event->flags & XCB_INPUT_KEY_EVENT_FLAGS_KEY_REPEAT) ? 1 : 0),
                     .mode = getActiveMaster()->mode,
                     .winInfo = winInfo
                 };
    applyEventRules(DEVICE_EVENT, &bindingEvent);
}

void onFocusInEvent(xcb_input_focus_in_event_t* event) {
    setActiveMasterByDeviceID(event->deviceid);
    WindowInfo* winInfo = getWindowInfo(event->event);
    DEBUG("Window %d was focused", event->event);
    if(winInfo && isFocusable(winInfo)) {
        if(!hasMask(winInfo, NO_RECORD_FOCUS_MASK))
            onWindowFocus(winInfo->id);
        removeMask(winInfo, URGENT_MASK);
        setBorder(winInfo->id);
    }
}

void onFocusOutEvent(xcb_input_focus_out_event_t* event) {
    setActiveMasterByDeviceID(event->deviceid);
    WindowInfo* winInfo = getWindowInfo(event->event);
    DEBUG("Window %d was unfocused", event->event);
    if(winInfo)
        resetBorder(winInfo->id);
}
void onMapEvent(xcb_map_notify_event_t* event) {
    TRACE("Detected map event for Window %d", event->window);
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo) {
        bool alreadlyMapped = hasMask(winInfo, MAPPABLE_MASK);
        addMask(winInfo, MAPPABLE_MASK | MAPPED_MASK);
        if(!alreadlyMapped)
            applyEventRules(CLIENT_MAP_ALLOW, winInfo);
        if(winInfo->dock)
            applyEventRules(SCREEN_CHANGE, NULL);
    }
}
void onMapRequestEvent(xcb_map_request_event_t* event) {
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo) {
        addMask(winInfo, MAPPABLE_MASK);
        applyEventRules(CLIENT_MAP_ALLOW, winInfo);
        if(getWorkspaceOfWindow(winInfo))
            return;
    }
    mapWindow(event->window);
}

void onPropertyEvent(xcb_property_notify_event_t* event) {
    WindowInfo* winInfo = getWindowInfo(event->window);
    // only reload properties if a window is mapped
    if(winInfo && hasMask(winInfo, MAPPED_MASK)) {
        if(event->atom == ewmh->_NET_WM_NAME || event->atom == XCB_ATOM_WM_NAME)
            loadWindowTitle(winInfo);
        else if(event->atom == XCB_ATOM_WM_HINTS)
            loadWindowHints(winInfo);
    }
}
bool onSelectionClearEvent(xcb_selection_clear_event_t* event) {
    // TODO fix
    if(event->owner == getPrivateWindow() && (event->selection == MPX_WM_SELECTION_ATOM ||
            event->selection == WM_SELECTION_ATOM)) {
        INFO("We lost the WM_SELECTION; another window manager is taking over: %d", event->owner);
        requestShutdown();
        return 0;
    }
    return 1;
}
void onUnmapEvent(xcb_unmap_notify_event_t* event) {
    TRACE("Detected unmap event for Window %d", event->window);
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo) {
        updateFocusForAllMasters(winInfo);
        removeMask(winInfo, MAPPED_MASK);
        // used to indicate that the window is no longer mappable
        if(isSyntheticEvent(event) || winInfo->overrideRedirect) {
            INFO("Marking window as withdrawn %d", event->window);
            removeMask(winInfo, MAPPABLE_MASK);
            applyEventRules(CLIENT_MAP_DISALLOW, winInfo);
        }
        if(winInfo->dock)
            applyEventRules(SCREEN_CHANGE, NULL);
    }
}

void addApplyBindingsRule() {
    addEvent(DEVICE_EVENT, DEFAULT_EVENT(checkBindings));
}

void registerForEvents() {
    if(ROOT_EVENT_MASKS)
        registerForWindowEvents(root, ROOT_EVENT_MASKS);
    grabAllBindings(NULL, 0, 0);
    DEBUG("listening for device event; masks: %d", ROOT_DEVICE_EVENT_MASKS);
    if(ROOT_DEVICE_EVENT_MASKS)
        passiveGrab(root, ROOT_DEVICE_EVENT_MASKS);
}
bool listenForNonRootEventsFromWindow(WindowInfo* winInfo) {
    uint32_t mask = winInfo->eventMasks ? winInfo->eventMasks : NON_ROOT_EVENT_MASKS;
    bool result = 0;
    if(registerForWindowEvents(winInfo->id, mask) == 0) {
        result = 1;
        uint32_t deviceMask = winInfo->deviceEventMasks ? winInfo->deviceEventMasks : NON_ROOT_DEVICE_EVENT_MASKS;
        passiveGrab(winInfo->id, deviceMask);
        DEBUG("Listening for events %d on %d", deviceMask, winInfo->id);
    }
    return result;
}

void addNonDocksToActiveWorkspace(WindowInfo* winInfo) {
    if(!getWorkspaceOfWindow(winInfo) && !winInfo->dock) {
        moveToWorkspace(winInfo, getActiveWorkspaceIndex());
    }
}

void addAutoTileRules() {
    addEvent(WORKSPACE_WINDOW_ADD, DEFAULT_EVENT(markWorkspaceOfWindowDirty));
    addEvent(WORKSPACE_WINDOW_REMOVE, DEFAULT_EVENT(markWorkspaceOfWindowDirty));
    addBatchEvent(IDLE, DEFAULT_EVENT(retileAllDirtyWorkspaces));
    addBatchEvent(IDLE, DEFAULT_EVENT(saveAllWindowMasks, LOWER_PRIORITY));
}
void assignDefaultLayoutsToWorkspace() {
    FOR_EACH(Workspace*, w, getAllWorkspaces()) {
        if(w->layouts.size == 0 && getLayout(w) == NULL) {
            FOR_EACH(Layout*, l, getRegisteredLayouts()) {
                addLayout(w, l);
            }
            cycleLayouts(w, 0);
        }
    }
}

void updateAllWindowWorkspaceState() {
    FOR_EACH(WindowInfo*, winInfo, getAllWindows()) {
        updateWindowWorkspaceState(winInfo);
    }
}

void addSyncMapStateRules() {
    addBatchEvent(MONITOR_WORKSPACE_CHANGE, DEFAULT_EVENT(updateAllWindowWorkspaceState));
    addBatchEvent(WORKSPACE_WINDOW_ADD, DEFAULT_EVENT(updateAllWindowWorkspaceState));
    addBatchEvent(TILE_WORKSPACE, DEFAULT_EVENT(updateAllWindowWorkspaceState, LOWER_PRIORITY));
}

void addBasicRules() {
    addEvent(IDLE, DEFAULT_EVENT(applyBatchEventRules));
    addEvent(0, DEFAULT_EVENT(logError));
    addEvent(XCB_CREATE_NOTIFY, DEFAULT_EVENT(onCreateEvent));
    addEvent(XCB_DESTROY_NOTIFY, DEFAULT_EVENT(onDestroyEvent));
    addEvent(XCB_UNMAP_NOTIFY, DEFAULT_EVENT(onUnmapEvent));
    addEvent(XCB_MAP_NOTIFY, DEFAULT_EVENT(onMapEvent));
    addEvent(XCB_MAP_REQUEST, DEFAULT_EVENT(onMapRequestEvent));
    addEvent(XCB_CONFIGURE_REQUEST, DEFAULT_EVENT(onConfigureRequestEvent));
    addEvent(XCB_CONFIGURE_NOTIFY, DEFAULT_EVENT(onConfigureNotifyEvent));
    addEvent(XCB_PROPERTY_NOTIFY, DEFAULT_EVENT(onPropertyEvent));
    addEvent(XCB_SELECTION_CLEAR, DEFAULT_EVENT(onSelectionClearEvent));
    addEvent(XCB_INPUT_FOCUS_IN + GENERIC_EVENT_OFFSET, DEFAULT_EVENT(onFocusInEvent));
    addEvent(XCB_INPUT_FOCUS_OUT + GENERIC_EVENT_OFFSET, DEFAULT_EVENT(onFocusOutEvent));
    addEvent(XCB_INPUT_HIERARCHY + GENERIC_EVENT_OFFSET, DEFAULT_EVENT(onHiearchyChangeEvent));
    addEvent(X_CONNECTION, DEFAULT_EVENT(assignDefaultLayoutsToWorkspace, HIGHER_PRIORITY));
    addEvent(X_CONNECTION, DEFAULT_EVENT(initState, HIGHEST_PRIORITY));
    addEvent(X_CONNECTION, DEFAULT_EVENT(assignUnusedMonitorsToWorkspaces, HIGH_PRIORITY));
    addEvent(X_CONNECTION, DEFAULT_EVENT(onXConnect, HIGH_PRIORITY));
    addEvent(CLIENT_MAP_ALLOW, DEFAULT_EVENT(loadWindowProperties));
    addEvent(POST_REGISTER_WINDOW, DEFAULT_EVENT(listenForNonRootEventsFromWindow, HIGHER_PRIORITY));
    addBatchEvent(SCREEN_CHANGE, DEFAULT_EVENT(detectMonitors, HIGH_PRIORITY));
    addBatchEvent(SCREEN_CHANGE, DEFAULT_EVENT(resizeAllMonitorsToAvoidAllDocks));
    addBatchEvent(SCREEN_CHANGE, DEFAULT_EVENT(assignUnusedMonitorsToWorkspaces, LOW_PRIORITY));
    for(int i = XCB_INPUT_KEY_PRESS; i <= XCB_INPUT_MOTION; i++) {
        addEvent(GENERIC_EVENT_OFFSET + i, DEFAULT_EVENT(onDeviceEvent));
    }
    addXIEventSupport();
    addApplyBindingsRule(remove);
    addIgnoreOverrideRedirectAndInputOnlyWindowsRule();
    addSyncMapStateRules();
    /* TODO readd
    addDoNotManageOverrideRedirectWindowsRule();
    */
}

bool ignoreOverrideRedirectAndInputOnlyWindowWindowsRule(WindowInfo* winInfo) {
    if(isOverrideRedirectWindow(winInfo) || isInputOnlyWindow(winInfo)) {
        unregisterWindow(winInfo, 1);
        return BF_STOP;
    }
    return BF_CONT;
}

void addIgnoreOverrideRedirectAndInputOnlyWindowsRule() {
    addEvent(POST_REGISTER_WINDOW, DEFAULT_EVENT(ignoreOverrideRedirectAndInputOnlyWindowWindowsRule, HIGHER_PRIORITY));
}


/* TODO Enable
bool addDoNotManageOverrideRedirectWindowsRule(bool remove) {
    return getEventRules(PRE_REGISTER_WINDOW).add({
        +[](WindowInfo * winInfo) {if(isOverrideRedirectWindow(winInfo))setNotManageable(winInfo);},
        FUNC_NAME,
    }, remove);
}
bool addIgnoreOverrideRedirectWindowsRule(bool remove) {
    return getEventRules(PRE_REGISTER_WINDOW).add({
        +[](WindowInfo * winInfo) {return !isOverrideRedirectWindow(winInfo);},
        FUNC_NAME,
        PASSTHROUGH_IF_TRUE,
        HIGH_PRIORITY,
    }, remove);
}

bool addIgnoreInputOnlyWindowsRule(bool remove) {
    return getEventRules(PRE_REGISTER_WINDOW).add({
        +[](WindowInfo * winInfo) {return !isInputOnly(winInfo);},
        FUNC_NAME,
        PASSTHROUGH_IF_TRUE,
        HIGH_PRIORITY
    }, remove);
}
*/
