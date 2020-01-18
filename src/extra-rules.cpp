#include "mywm-structs.h"


#include "bindings.h"
#include "devices.h"
#include "globals.h"
#include "layouts.h"
#include "logger.h"
#include "masters.h"
#include "user-events.h"
#include "monitors.h"
#include "extra-rules.h"
#include "state.h"
#include "system.h"
#include "windows.h"
#include "window-properties.h"
#include "wmfunctions.h"
#include "workspaces.h"
#include "xsession.h"
#include "time.h"


void addUnknownInputOnlyWindowIgnoreRule(AddFlag flag) {
    getEventRules(CLIENT_MAP_ALLOW).add(new BoundFunction(+[](WindowInfo * winInfo) {return winInfo->isImplicitType() && winInfo->isInputOnly() ? unregisterWindow(winInfo) : 0;},
    FUNC_NAME, PASSTHROUGH_IF_FALSE), flag);
}
static bool isBaseAreaLessThan(WindowInfo* winInfo, int area) {
    auto sizeHints = getWindowSizeHints(winInfo);
    return sizeHints && (sizeHints->flags & XCB_ICCCM_SIZE_HINT_P_SIZE) &&
        sizeHints->width * sizeHints->height <= area;
}

void addIgnoreSmallWindowRule(AddFlag flag) {
    getEventRules(CLIENT_MAP_ALLOW).add(new BoundFunction(+[](WindowInfo * winInfo) {return isBaseAreaLessThan(winInfo, 5) ? unregisterWindow(winInfo) : 0;},
    FUNC_NAME, PASSTHROUGH_IF_FALSE), flag);
}
void (*printStatusMethod)();
void addPrintStatusRule(AddFlag flag) {
    getEventRules(IDLE).add(new BoundFunction(+[]() {
        if(printStatusMethod && STATUS_FD)
            printStatusMethod();
    }, FUNC_NAME), flag);
}
void addDesktopRule(AddFlag flag) {
    getEventRules(CLIENT_MAP_ALLOW).add(new BoundFunction(+[](WindowInfo * winInfo) {
        if(winInfo->getType() == ewmh->_NET_WM_WINDOW_TYPE_DESKTOP) {
            winInfo->addMask(IGNORE_WORKSPACE_MASKS_MASK | NO_TILE_MASK | MAXIMIZED_MASK |
                BELOW_MASK | STICKY_MASK);
            winInfo->setTilingOverrideEnabled(1 | 2 | 16);
        }
    }, FUNC_NAME), flag);
    getEventRules(WINDOW_WORKSPACE_CHANGE).add(new BoundFunction(+[](WindowInfo * winInfo) {
        if(winInfo->hasMask(STICKY_MASK) && winInfo->getType() == ewmh->_NET_WM_WINDOW_TYPE_DESKTOP)
            updateFocusForAllMasters(winInfo);
    }, "_desktopTransferFocus"), flag);
}
void addFloatRule(AddFlag flag) {
    getEventRules(CLIENT_MAP_ALLOW).add(new BoundFunction(+[](WindowInfo * winInfo) {
        if(winInfo->getType() != ewmh->_NET_WM_WINDOW_TYPE_NORMAL &&
            winInfo->getType()  != ewmh->_NET_WM_WINDOW_TYPE_DESKTOP) floatWindow(winInfo);
    }, FUNC_NAME), flag);
}

void addAvoidDocksRule(AddFlag flag) {
    getEventRules(PROPERTY_LOAD).add(new BoundFunction(+[](WindowInfo * winInfo) {
        assert(winInfo->getType());
        if(winInfo->getType() == ewmh->_NET_WM_WINDOW_TYPE_DOCK) {
            LOG(LOG_LEVEL_DEBUG, "Marking window as dock\n");
            winInfo->setDock();
            winInfo->addMask(EXTERNAL_CONFIGURABLE_MASK);
            removeBorder(winInfo->getID());
            loadDockProperties(winInfo);
            winInfo->removeFromWorkspace();
        }
    }, FUNC_NAME), flag);
}
void addNoDockFocusRule(AddFlag flag) {
    getEventRules(CLIENT_MAP_ALLOW).add(new BoundFunction(+[](WindowInfo * winInfo) { if(winInfo->isDock()) winInfo->removeMask(INPUT_MASK | WM_TAKE_FOCUS_MASK);},
    FUNC_NAME), flag);
}
void addFocusFollowsMouseRule(AddFlag flag) {
    NON_ROOT_DEVICE_EVENT_MASKS |= XCB_INPUT_XI_EVENT_MASK_ENTER;
    getEventRules(GENERIC_EVENT_OFFSET + XCB_INPUT_ENTER).add(DEFAULT_EVENT(focusFollowMouse), flag);
}
void focusFollowMouse() {
    xcb_input_enter_event_t* event = (xcb_input_enter_event_t*)getLastEvent();
    setActiveMasterByDeviceID(event->deviceid);
    WindowID win = event->event;
    WindowInfo* winInfo = getWindowInfo(win);
    LOG(LOG_LEVEL_DEBUG, "focus following mouse %d win %d\n", getActiveMaster()->getID(), win);
    if(winInfo)
        focusWindow(winInfo);
}
static int nonAlwaysOnTopOrBottomWindowMoved;
static void markAlwaysOnTop(WindowInfo* winInfo) {
    if(!winInfo->hasPartOfMask(ALWAYS_ON_TOP_MASK | ALWAYS_ON_BOTTOM_MASK))
        nonAlwaysOnTopOrBottomWindowMoved = 1;
}
static void enforceAlwaysOnTop() {
    if(nonAlwaysOnTopOrBottomWindowMoved) {
        for(WindowInfo* winInfo : getAllWindows()) {
            if(winInfo->hasMask(ALWAYS_ON_BOTTOM_MASK))
                lowerWindow(winInfo->getID());
            else if(winInfo->hasMask(ALWAYS_ON_TOP_MASK))
                raiseWindow(winInfo->getID());
        }
        nonAlwaysOnTopOrBottomWindowMoved = 0;
    }
}
void addAlwaysOnTopBottomRules(AddFlag flag) {
    getEventRules(WINDOW_MOVE).add(DEFAULT_EVENT(markAlwaysOnTop), flag);
    getBatchEventRules(WINDOW_MOVE).add(DEFAULT_EVENT(enforceAlwaysOnTop), flag);
}

static void stickyPrimaryMonitor() {
    Monitor* primary = getPrimaryMonitor();
    if(primary)
        for(WindowInfo* winInfo : getAllWindows())
            if(winInfo->hasMask(PRIMARY_MONITOR_MASK)) {
                if(winInfo->getWorkspace() && primary->getWorkspace())
                    winInfo->moveToWorkspace(primary->getWorkspace()->getID());
                if(winInfo->getTilingOverrideMask())
                    arrangeNonTileableWindow(winInfo, primary);
            }
}
void addStickyPrimaryMonitorRule(AddFlag flag) {
    getBatchEventRules(CLIENT_MAP_ALLOW).add(DEFAULT_EVENT(stickyPrimaryMonitor), flag);
    getBatchEventRules(SCREEN_CHANGE).add(DEFAULT_EVENT(stickyPrimaryMonitor), flag);
}


void addDieOnIdleRule(AddFlag flag) {
    getEventRules(TRUE_IDLE).add(DEFAULT_EVENT(+[]() {quit(0);}), flag);
}
void addShutdownOnIdleRule(AddFlag flag) {
    getEventRules(TRUE_IDLE).add(DEFAULT_EVENT(requestShutdown), flag);
}

void keepTransientsOnTop(WindowInfo* winInfo) {
    if(!winInfo->hasMask(ALWAYS_ON_TOP_MASK)) {
        WindowID id = winInfo->getID();
        for(WindowInfo* winInfo2 : getAllWindows()) {
            if(winInfo2->getTransientFor() == id)
                raiseWindow(winInfo2->getID(), id);
        }
    }
}
void addKeepTransientsOnTopRule(AddFlag flag) {
    getEventRules(WINDOW_MOVE).add(DEFAULT_EVENT(keepTransientsOnTop), flag);
}

void autoFocus() {
    xcb_map_notify_event_t* event = (xcb_map_notify_event_t*)getLastEvent();
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(!winInfo || !getUserTime(winInfo->getID()) || !winInfo->isNotInInvisibleWorkspace())
        return;
    long delta = getTime() - winInfo->getCreationTime();
    if(!winInfo->isNotManageable() && winInfo->hasMask(INPUT_MASK)) {
        if(delta < AUTO_FOCUS_NEW_WINDOW_TIMEOUT) {
            Master* master = getClientMaster(winInfo->getID());
            if(master && focusWindow(winInfo, master)) {
                LOG(LOG_LEVEL_INFO, "auto focused window %d (Detected %ldms ago)\n", winInfo->getID(), delta);
                raiseWindow(winInfo->getID());
                getActiveMaster()->onWindowFocus(winInfo->getID());
            }
        }
        else
            LOG(LOG_LEVEL_DEBUG, "did not auto focus window %d (Detected %ldms ago)\n", winInfo->getID(), delta);
    }
}
void addAutoFocusRule(AddFlag flag) {
    getEventRules(XCB_MAP_NOTIFY).add(DEFAULT_EVENT(autoFocus), flag);
}
void addScanChildrenRule(AddFlag flag) {
    getEventRules(POST_REGISTER_WINDOW).add(DEFAULT_EVENT(scan), flag);
}
static bool isNotRepeatedKey() {
    xcb_input_key_press_event_t* event = (xcb_input_key_press_event_t*)getLastEvent();
    return !(event->flags & XCB_INPUT_KEY_EVENT_FLAGS_KEY_REPEAT);
}
void addIgnoreKeyRepeat(AddFlag flag) {
    getEventRules(XCB_INPUT_KEY_PRESS).add(DEFAULT_EVENT(isNotRepeatedKey), flag);
}
static bool unregisterNonTopLevelWindows() {
    xcb_reparent_notify_event_t* event = (xcb_reparent_notify_event_t*)getLastEvent();
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(event->parent != root && winInfo) {
        unregisterWindow(winInfo, 0);
        return 0;
    }
    return 1;
}
void addIgnoreNonTopLevelWindowsRule(AddFlag flag) {
    getEventRules(XCB_REPARENT_NOTIFY).add(DEFAULT_EVENT(unregisterNonTopLevelWindows), flag);
}

static void moveNonTileableWindowsToWorkspaceBounds(WindowInfo* winInfo) {
    if(!winInfo->isTileable()) {
        RectWithBorder rect = getRealGeometry(winInfo);
        if(rect.isAtOrigin()) {
            Workspace* w = winInfo->getWorkspace();
            if(w && w->isVisible()) {
                rect.translate({0, 0}, w->getMonitor()->getViewport().getTopLeftCorner());
                setWindowPosition(winInfo->getID(), rect);
            }
        }
    }
}
void addMoveNonTileableWindowsToWorkspaceBounds() {
    getEventRules(CLIENT_MAP_ALLOW).add(DEFAULT_EVENT(moveNonTileableWindowsToWorkspaceBounds));
}
static void convertNonManageableWindowMask(WindowInfo* winInfo) {
    if(winInfo->getType() == ewmh->_NET_WM_WINDOW_TYPE_NORMAL && winInfo->isNotManageable())
        if(winInfo->hasMask(BELOW_MASK))
            winInfo->addMask(ALWAYS_ON_BOTTOM_MASK);
        else if(winInfo->hasMask(ABOVE_MASK))
            winInfo->addMask(ALWAYS_ON_TOP_MASK);
}
void addConvertNonManageableWindowMask() {
    getEventRules(CLIENT_MAP_ALLOW).add(DEFAULT_EVENT(convertNonManageableWindowMask));
}
