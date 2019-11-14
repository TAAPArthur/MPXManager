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
    getEventRules(ClientMapAllow).add(new BoundFunction(+[](WindowInfo * winInfo) {return winInfo->isImplicitType() && winInfo->isInputOnly() ? unregisterWindow(winInfo) : 0;},
    FUNC_NAME, PASSTHROUGH_IF_FALSE), flag);
}
static bool isBaseAreaLessThan(WindowInfo* winInfo, int area) {
    auto sizeHints = getWindowSizeHints(winInfo);
    return sizeHints && (sizeHints->flags & XCB_ICCCM_SIZE_HINT_P_SIZE) &&
        sizeHints->width * sizeHints->height <= area;
}

void addIgnoreSmallWindowRule(AddFlag flag) {
    getEventRules(ClientMapAllow).add(new BoundFunction(+[](WindowInfo * winInfo) {return isBaseAreaLessThan(winInfo, 5) ? unregisterWindow(winInfo) : 0;},
    FUNC_NAME, PASSTHROUGH_IF_FALSE), flag);
}
void (*printStatusMethod)();
void addPrintStatusRule(AddFlag flag) {
    getEventRules(Idle).add(new BoundFunction(+[]() {
        if(printStatusMethod && STATUS_FD)
            printStatusMethod();
    }, FUNC_NAME), flag);
}
void addDesktopRule(AddFlag flag) {
    getEventRules(ClientMapAllow).add(new BoundFunction(+[](WindowInfo * winInfo) {
        if(winInfo->getType() == ewmh->_NET_WM_WINDOW_TYPE_DESKTOP) {
            winInfo->addMask(NO_ACTIVATE_MASK | NO_RECORD_FOCUS | IGNORE_WORKSPACE_MASKS_MASK | NO_TILE_MASK | MAXIMIZED_MASK |
                BELOW_MASK | STICKY_MASK);
            winInfo->setTilingOverrideEnabled(3);
        }
    },
    FUNC_NAME), flag);
}
void addFloatRule(AddFlag flag) {
    getEventRules(ClientMapAllow).add(new BoundFunction(+[](WindowInfo * winInfo) {
        if(winInfo->getType() != ewmh->_NET_WM_WINDOW_TYPE_NORMAL) floatWindow(winInfo);
    }, FUNC_NAME), flag);
}

void addAvoidDocksRule(AddFlag flag) {
    getEventRules(ClientMapAllow).add(new BoundFunction(+[](WindowInfo * winInfo) {
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
    getEventRules(ClientMapAllow).add(new BoundFunction(+[](WindowInfo * winInfo) { if(winInfo->isDock()) winInfo->removeMask(INPUT_MASK | WM_TAKE_FOCUS_MASK);},
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
    if(!winInfo->hasPartOfMask(ALWAYS_ON_TOP | ALWAYS_ON_BOTTOM))
        nonAlwaysOnTopOrBottomWindowMoved = 1;
}
static void enforceAlwaysOnTop() {
    if(nonAlwaysOnTopOrBottomWindowMoved) {
        for(WindowInfo* winInfo : getAllWindows()) {
            if(winInfo->hasMask(ALWAYS_ON_BOTTOM))
                lowerWindow(winInfo->getID());
            else if(winInfo->hasMask(ALWAYS_ON_TOP))
                raiseWindow(winInfo->getID());
        }
        nonAlwaysOnTopOrBottomWindowMoved = 0;
    }
}
void addDieOnIdleRule(AddFlag flag) {
    getEventRules(TrueIdle).add(DEFAULT_EVENT(+[]() {quit(0);}), flag);
}
void addShutdownOnIdleRule(AddFlag flag) {
    getEventRules(TrueIdle).add(DEFAULT_EVENT(requestShutdown), flag);
}

void keepTransientsOnTop(WindowInfo* winInfo) {
    if(!winInfo->hasMask(ALWAYS_ON_TOP)) {
        WindowID id = winInfo->getID();
        for(WindowInfo* winInfo2 : getAllWindows()) {
            if(winInfo2->isInteractable() && winInfo2->getTransientFor() == id)
                raiseWindow(winInfo2->getID(), id);
        }
    }
}
void addKeepTransientsOnTopRule(AddFlag flag) {
    getEventRules(onWindowMove).add(DEFAULT_EVENT(keepTransientsOnTop), flag);
}

void autoFocus() {
    xcb_map_notify_event_t* event = (xcb_map_notify_event_t*)getLastEvent();
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(!winInfo || !getUserTime(winInfo->getID()))
        return;
    long delta = getTime() - winInfo->getCreationTime();
    if(!winInfo->isSpecial() && winInfo->hasMask(INPUT_MASK)) {
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
void addAlwaysOnTopBottomRules(AddFlag flag) {
    getEventRules(onWindowMove).add(DEFAULT_EVENT(markAlwaysOnTop), flag);
    getBatchEventRules(onWindowMove).add(DEFAULT_EVENT(enforceAlwaysOnTop), flag);
}
void addScanChildrenRule(AddFlag flag) {
    getEventRules(PostRegisterWindow).add(DEFAULT_EVENT(scan), flag);
}
static bool isNotRepeatedKey() {
    xcb_input_key_press_event_t* event = (xcb_input_key_press_event_t*)getLastEvent();
    return !(event->flags & XCB_INPUT_KEY_EVENT_FLAGS_KEY_REPEAT);
}
void addIgnoreKeyRepeat(AddFlag flag) {
    getEventRules(XCB_INPUT_KEY_PRESS).add(DEFAULT_EVENT(isNotRepeatedKey), flag);
}
static void setDefaultBorderWidth(WindowInfo* winInfo) {
    if(!winInfo->isInputOnly() && winInfo->getWorkspace())
        configureWindow(winInfo->getID(), XCB_CONFIG_WINDOW_BORDER_WIDTH, &DEFAULT_BORDER_WIDTH);
}
void addDefaultBorderRule(AddFlag flag) {
    getEventRules(PostRegisterWindow).add(DEFAULT_EVENT(setDefaultBorderWidth), flag);
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
    getEventRules(ClientMapAllow).add(DEFAULT_EVENT(moveNonTileableWindowsToWorkspaceBounds));
}
