#include "bindings.h"
#include "devices.h"
#include "extra-rules.h"
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
#include "wmfunctions.h"
#include "workspaces.h"
#include "xsession.h"

void (*printStatusMethod)();
void addPrintStatusRule(bool remove) {
    getEventRules(IDLE).add(new BoundFunction(+[]() {
        if(printStatusMethod && STATUS_FD)
            printStatusMethod();
    }, FUNC_NAME), remove);
}
void addDesktopRule(bool remove) {
    getEventRules(CLIENT_MAP_ALLOW).add(new BoundFunction(+[](WindowInfo * winInfo) {
        if(winInfo->getType() == ewmh->_NET_WM_WINDOW_TYPE_DESKTOP) {
            winInfo->addMask(IGNORE_WORKSPACE_MASKS_MASK | NO_TILE_MASK | MAXIMIZED_MASK |
                BELOW_MASK | STICKY_MASK);
            winInfo->setTilingOverrideEnabled(1 | 2 | 16);
        }
    }, FUNC_NAME), remove);
    getEventRules(WINDOW_WORKSPACE_CHANGE).add(new BoundFunction(+[](WindowInfo * winInfo) {
        if(winInfo->hasMask(STICKY_MASK) && winInfo->getType() == ewmh->_NET_WM_WINDOW_TYPE_DESKTOP)
            updateFocusForAllMasters(winInfo);
    }, "_desktopTransferFocus"), remove);
}

void addAvoidDocksRule(bool remove) {
    getEventRules(PROPERTY_LOAD).add(new BoundFunction(+[](WindowInfo * winInfo) {
        assert(winInfo->getType());
        if(winInfo->getType() == ewmh->_NET_WM_WINDOW_TYPE_DOCK) {
            DEBUG("Marking window as dock");
            winInfo->setDock();
            winInfo->addMask(EXTERNAL_CONFIGURABLE_MASK);
            removeBorder(winInfo->getID());
            loadDockProperties(winInfo);
            winInfo->removeFromWorkspace();
        }
    }, FUNC_NAME), remove);
}

static void stickyPrimaryMonitor() {
    Monitor* primary = getPrimaryMonitor();
    if(primary)
        for(WindowInfo* winInfo : getAllWindows())
            if(winInfo->hasMask(PRIMARY_MONITOR_MASK)) {
                if(winInfo->getWorkspace() && primary->getWorkspace())
                    winInfo->moveToWorkspace(primary->getWorkspace()->getID());
                else
                    arrangeNonTileableWindow(winInfo, primary);
            }
}
void addStickyPrimaryMonitorRule(bool remove) {
    getBatchEventRules(CLIENT_MAP_ALLOW).add(DEFAULT_EVENT(stickyPrimaryMonitor), remove);
    getBatchEventRules(SCREEN_CHANGE).add(DEFAULT_EVENT(stickyPrimaryMonitor), remove);
}

void keepTransientsOnTop(WindowInfo* winInfo) {
    WindowID id = winInfo->getID();
    for(WindowInfo* winInfo2 : getAllWindows()) {
        if(winInfo2->getTransientFor() == id)
            raiseWindow(winInfo2, id);
    }
}
void addKeepTransientsOnTopRule(bool remove) {
    getEventRules(WINDOW_MOVE).add(DEFAULT_EVENT(keepTransientsOnTop), remove);
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
                LOG(LOG_LEVEL_INFO, "auto focused window %d (Detected %ldms ago)", winInfo->getID(), delta);
                raiseWindow(winInfo);
                getActiveMaster()->onWindowFocus(winInfo->getID());
            }
        }
        else
            LOG(LOG_LEVEL_DEBUG, "did not auto focus window %d (Detected %ldms ago)", winInfo->getID(), delta);
    }
}
void addAutoFocusRule(bool remove) {
    getEventRules(XCB_MAP_NOTIFY).add(DEFAULT_EVENT(autoFocus), remove);
}
static bool isNotRepeatedKey() {
    xcb_input_key_press_event_t* event = (xcb_input_key_press_event_t*)getLastEvent();
    return !(event->flags & XCB_INPUT_KEY_EVENT_FLAGS_KEY_REPEAT);
}
void addIgnoreKeyRepeat(bool remove) {
    getEventRules(XCB_INPUT_KEY_PRESS).add(DEFAULT_EVENT_HIGH(isNotRepeatedKey), remove);
}

static void moveNonTileableWindowsToWorkspaceBounds(WindowInfo* winInfo) {
    if(!winInfo->isTileable()) {
        RectWithBorder rect = getRealGeometry(winInfo);
        if(rect.isAtOrigin()) {
            Workspace* w = winInfo->getWorkspace();
            if(w && w->isVisible()) {
                rect.translate(w->getMonitor()->getViewport().getTopLeftCorner());
                setWindowPosition(winInfo->getID(), rect);
            }
        }
    }
}
void addMoveNonTileableWindowsToWorkspaceBounds() {
    getEventRules(CLIENT_MAP_ALLOW).add(DEFAULT_EVENT(moveNonTileableWindowsToWorkspaceBounds));
}


void addShutdownOnIdleRule(bool remove) {
    getEventRules(TRUE_IDLE).add(DEFAULT_EVENT(requestShutdown), remove);
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

void addIgnoreNonTopLevelWindowsRule(bool remove) {
    getEventRules(XCB_REPARENT_NOTIFY).add(DEFAULT_EVENT(unregisterNonTopLevelWindows), remove);
}
