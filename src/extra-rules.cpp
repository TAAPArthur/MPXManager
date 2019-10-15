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


void addUnknownWindowIgnoreRule(void) {
    getEventRules(ClientMapAllow).add(new BoundFunction(+[](WindowInfo * winInfo) {return winInfo->hasMask(IMPLICIT_TYPE) ? unregisterWindow(winInfo) : 0;},
    FUNC_NAME, PASSTHROUGH_IF_FALSE));
}
static bool isBaseAreaLessThan(WindowInfo* winInfo, int area) {
    auto sizeHints = getWindowSizeHints(winInfo);
    return sizeHints && (sizeHints->flags & XCB_ICCCM_SIZE_HINT_P_SIZE) &&
           sizeHints->base_width * sizeHints->base_height <= area;
}

void addIgnoreSmallWindowRule(void) {
    getEventRules(ClientMapAllow).add(new BoundFunction(+[](WindowInfo * winInfo) {return isBaseAreaLessThan(winInfo, 100) ? unregisterWindow(winInfo) : 0;},
    FUNC_NAME, PASSTHROUGH_IF_FALSE));
}
void (*printStatusMethod)(void);
void addPrintStatusRule(void) {
    getEventRules(Idle).add(new BoundFunction(+[]() {
        if(printStatusMethod && STATUS_FD)
            printStatusMethod();
    }, FUNC_NAME));
}
void addDesktopRule(void) {
    getEventRules(ClientMapAllow).add(new BoundFunction(+[](WindowInfo * winInfo) {
        if(winInfo->getType() == ewmh->_NET_WM_WINDOW_TYPE_DESKTOP) {
            winInfo->addMask(NO_ACTIVATE_MASK | NO_RECORD_FOCUS | IGNORE_WORKSPACE_MASKS_MASK | NO_TILE_MASK | MAXIMIZED_MASK |
                             BELOW_MASK |        STICKY_MASK);
            winInfo->setTilingOverrideEnabled(3);
        }
    },
    FUNC_NAME
                                                       ));
}
void addFloatRule(void) {
    getEventRules(ClientMapAllow).add(new BoundFunction(+[](WindowInfo * winInfo) {
        if(winInfo->getType() != ewmh->_NET_WM_WINDOW_TYPE_NORMAL) floatWindow(winInfo);
    }, FUNC_NAME));
}

void addAvoidDocksRule(void) {
    getEventRules(ClientMapAllow).add(new BoundFunction(+[](WindowInfo * winInfo) {
        if(winInfo->getType() == ewmh->_NET_WM_WINDOW_TYPE_DOCK) {
            loadDockProperties(winInfo);
            winInfo->setDock();
            winInfo->addMask(EXTERNAL_CONFIGURABLE_MASK);
            winInfo->removeFromWorkspace();
            removeBorder(winInfo->getID());
        }
    }, FUNC_NAME));
}
void addNoDockFocusRule(void) {
    getEventRules(ClientMapAllow).add(new BoundFunction(+[](WindowInfo * winInfo) { if(winInfo->isDock()) winInfo->removeMask(INPUT_MASK);},
    FUNC_NAME));
}
void addFocusFollowsMouseRule(void) {
    NON_ROOT_DEVICE_EVENT_MASKS |= XCB_INPUT_XI_EVENT_MASK_ENTER;
    getEventRules(GENERIC_EVENT_OFFSET + XCB_INPUT_ENTER).add(DEFAULT_EVENT(focusFollowMouse));
}
void focusFollowMouse(void) {
    xcb_input_enter_event_t* event = (xcb_input_enter_event_t*)getLastEvent();
    setActiveMasterByDeviceId(event->deviceid);
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
static void enforceAlwaysOnTop(void) {
    if(nonAlwaysOnTopOrBottomWindowMoved) {
        for(WindowInfo* winInfo : getAllWindows()) {
            if(winInfo->hasMask(ALWAYS_ON_BOTTOM))
                lowerWindowInfo(winInfo);
        }
        for(WindowInfo* winInfo : getAllWindows()) {
            if(winInfo->hasMask(ALWAYS_ON_TOP))
                raiseWindowInfo(winInfo);
        }
        nonAlwaysOnTopOrBottomWindowMoved = 0;
    }
}
void addDieOnIdleRule(void) {
    getEventRules(TrueIdle).add(new BoundFunction(+[]() {quit(0);}, FUNC_NAME));
}
void addShutdownOnIdleRule(void) {
    getEventRules(TrueIdle).add(DEFAULT_EVENT(requestShutdown));
}
/* TODO comeback too
void keepTransientsOnTop(WindowInfo* winInfo) {
    if(!winInfo->hasMask(ALWAYS_ON_TOP)) {
        int id = winInfo->getID();
        for(WindowInfo* winInfo : getAllWindows()) {
            if(winInfo->isInteractable() && winInfo->getTransientFor() == id)
                raiseWindowInfo(winInfo);
        }
    }
}
*/

void autoFocus() {
    xcb_map_notify_event_t* event = (xcb_map_notify_event_t*)getLastEvent();
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(!winInfo)
        return;
    long delta = getTime() - winInfo->getCreationTime();
    if(winInfo->hasMask(INPUT_MASK)) {
        if(delta < AUTO_FOCUS_NEW_WINDOW_TIMEOUT) {
            if(focusWindow(winInfo)) {
                LOG(LOG_LEVEL_DEBUG, "auto focused window %d (Detected %ldms ago)\n", winInfo->getID(), delta);
                raiseWindowInfo(winInfo);
                getActiveMaster()->onWindowFocus(winInfo->getID());
            }
        }
        else
            LOG(LOG_LEVEL_TRACE, "did not auto focus window %d (Detected %ldms ago)\n", winInfo->getID(), delta);
    }
}
void addAutoFocusRule() {
    getEventRules(XCB_MAP_NOTIFY).add(DEFAULT_EVENT(autoFocus));
}
void addAlwaysOnTopBottomRules() {
    getEventRules(onWindowMove).add(DEFAULT_EVENT(markAlwaysOnTop));
    getBatchEventRules(onWindowMove).add(DEFAULT_EVENT(enforceAlwaysOnTop));
}
void addScanChildrenRule(AddFlag flag) {
    getEventRules(PostRegisterWindow).add(DEFAULT_EVENT(scan), flag);
}
