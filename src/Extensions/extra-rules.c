#include "../bindings.h"
#include "../boundfunction.h"
#include "../devices.h"
#include "../globals.h"
#include "../layouts.h"
#include "../masters.h"
#include "../monitors.h"
#include "../functions.h"
#include "../mywm-structs.h"
#include "../system.h"
#include "../user-events.h"
#include "../util/logger.h"
#include "../windows.h"
#include "../wmfunctions.h"
#include "../workspaces.h"
#include "../xevent.h"
#include "../xutil/window-properties.h"
#include "../xutil/xsession.h"
#include "extra-rules.h"


void printStatusMethod(void) {
    if(STATUS_FD == 0)
        return;
    if(!HIDE_WM_STATUS) {
        if(isLogging(LOG_LEVEL_DEBUG)) {
            dprintf(STATUS_FD, "[%d]: ", getIdleCount());
        }
        if(getActiveMode())
            dprintf(STATUS_FD, "%d: ", getActiveMode());
        if(getAllMasters()->size > 1)
            dprintf(STATUS_FD, "%dM %dA ", getAllMasters()->size, getActiveMasterKeyboardID());
        FOR_EACH(Workspace*, w, getAllWorkspaces()) {
            const char* color;
            if(hasWindowWithMask(w, URGENT_MASK))
                color = "cyan";
            else if(isWorkspaceVisible(w))
                color = "green";
            else if(hasWindowWithMask(w, MAPPABLE_MASK))
                color = "yellow";
            else continue;
            dprintf(STATUS_FD, "^fg(%s)%s%s:%s^fg() ", color, w->name, w == getActiveWorkspace() ? "*" : "",
                getLayout(w) ? getLayout(w)->name : "");
        }
        if(getActiveMaster()->bindings.size)
            dprintf(STATUS_FD, "[(%d)] ", getActiveMaster()->bindings.size);
    }
    if(getFocusedWindow() && isNotInInvisibleWorkspace(getFocusedWindow())) {
        if(isLogging(LOG_LEVEL_DEBUG))
            dprintf(STATUS_FD, "%0x ", getFocusedWindow()->id);
        dprintf(STATUS_FD, "^fg(%s)%s^fg()", "green", getFocusedWindow()->title);
    }
    else {
        dprintf(STATUS_FD, "Focused on %x (root: %x)", getActiveFocus(), root);
    }
    dprintf(STATUS_FD, "\n");
}
void addPrintStatusRule() {
    addEvent(TRUE_IDLE, DEFAULT_EVENT(printStatusMethod));
}
/* TODO
void addDesktopRule() {
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
*/


static void stickyPrimaryMonitor() {
    Monitor* primary = getPrimaryMonitor();
    if(primary) {
        FOR_EACH(WindowInfo*, winInfo, getAllWindows()) {
            if(hasMask(winInfo, PRIMARY_MONITOR_MASK)) {
                if(getWorkspaceOfWindow(winInfo) && getWorkspaceOfMonitor(primary))
                    moveToWorkspace(winInfo, getWorkspaceOfMonitor(primary)->id);
                else
                    arrangeNonTileableWindow(winInfo, primary);
            }
        }
    }
}
void addStickyPrimaryMonitorRule() {
    addBatchEvent(CLIENT_MAP_ALLOW, DEFAULT_EVENT(stickyPrimaryMonitor));
    addBatchEvent(SCREEN_CHANGE, DEFAULT_EVENT(stickyPrimaryMonitor));
}

void keepTransientsOnTop(WindowInfo* winInfo) {
    WindowID id = winInfo->id;
    FOR_EACH(WindowInfo*, winInfo2, getAllWindows()) {
        if(winInfo2->transientFor == id)
            raiseWindowInfo(winInfo2, id);
    }
}
void addKeepTransientsOnTopRule() {
    addEvent(WINDOW_MOVE, DEFAULT_EVENT(keepTransientsOnTop));
}
void shiftWindowToPositionInWorkspaceStack(WindowInfo* winInfo, InsertWindowPosition arg) {
    ArrayList* stack = getWorkspaceWindowStack(getWorkspaceOfWindow(winInfo));
    if(winInfo->creationTime && peek(stack) == winInfo) {
        int index = getFocusedWindow() ? getIndex(stack, getFocusedWindow(), sizeof(WindowID)) : -1;
        if(index == -1 && arg != HEAD_OF_STACK)
            return;
        switch(arg) {
            case HEAD_OF_STACK:
                shiftToHead(stack, stack->size - 1);
                break;
            case BEFORE_FOCUSED:
                shiftToPos(stack, stack->size - 1, index);
                break;
            case AFTER_FOCUSED:
                shiftToPos(stack, stack->size - 1, index + 1);
        }
    }
}
void addInsertWindowsAtPositionRule(InsertWindowPosition arg) {
    addEvent(WORKSPACE_WINDOW_ADD, DEFAULT_EVENT(shiftWindowToPositionInWorkspaceStack, HIGH_PRIORITY, .arg = {arg}));
}

static bool isNotRepeatedKey(xcb_input_key_press_event_t* event) {
    return !(event->flags & XCB_INPUT_KEY_EVENT_FLAGS_KEY_REPEAT);
}
void addIgnoreKeyRepeat() {
    addEvent(XCB_INPUT_KEY_PRESS, FILTER_EVENT(isNotRepeatedKey, HIGHER_PRIORITY, .abort = 1));
}

static void moveNonTileableWindowsToWorkspaceBounds(WindowInfo* winInfo) {
    if(!isTileable(winInfo)) {
        Rect rect = getRealGeometry(winInfo->id);
        if(rect.x == 0 && rect.y == 0) {
            Workspace* w = getWorkspaceOfWindow(winInfo);
            if(w && isWorkspaceVisible(w)) {
                Rect view = getMonitor(w)->view;
                rect.x += view.x;
                rect.y += view.y;
                setWindowPosition(winInfo->id, rect);
            }
        }
    }
}
void addMoveNonTileableWindowsToWorkspaceBounds() {
    addEvent(CLIENT_MAP_ALLOW, DEFAULT_EVENT(moveNonTileableWindowsToWorkspaceBounds));
}

static bool unregisterNonTopLevelWindows(xcb_reparent_notify_event_t* event) {
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(event->parent != root && winInfo) {
        unregisterWindow(winInfo, 0);
        return 0;
    }
    return 1;
}

void addIgnoreNonTopLevelWindowsRule() {
    addEvent(XCB_REPARENT_NOTIFY, FILTER_EVENT(unregisterNonTopLevelWindows, .abort = 1));
}
void onReparentEvent(xcb_reparent_notify_event_t* event) {
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo)
        winInfo->parent = event->parent;
    else if(registerWindow(event->window, event->parent, NULL)) {
        WindowInfo* winInfo = getWindowInfo(event->window);
        applyEventRules(WINDOW_MOVE, winInfo);
    }
}
void addDetectReparentEventRule() {
    addEvent(XCB_REPARENT_NOTIFY, DEFAULT_EVENT(onReparentEvent));
}


void autoFloatNonNormalWindows(WindowInfo* winInfo) {
    if(winInfo->type != ewmh->_NET_WM_WINDOW_TYPE_NORMAL &&
        winInfo->type  != ewmh->_NET_WM_WINDOW_TYPE_DESKTOP) floatWindow(winInfo);
}
void addFloatRule() {
    addEvent(CLIENT_MAP_ALLOW, DEFAULT_EVENT(autoFloatNonNormalWindows));
}
