#include <assert.h>

#include "bindings.h"
#include "devices.h"
#include "functions.h"
#include "globals.h"
#include "layouts.h"
#include "masters.h"
#include "monitors.h"
#include "system.h"
#include "util/logger.h"
#include "windows.h"
#include "wmfunctions.h"
#include "workspaces.h"
#include "xevent.h"
#include "xutil/device-grab.h"
#include "xutil/test-functions.h"
#include "xutil/window-properties.h"

static int getNextIndexInStack(const ArrayList* stack, int delta, bool(*filter)(WindowInfo*)) {
    WindowInfo* winInfo = getFocusedWindow();
    int index = winInfo ? getIndex(stack, winInfo, sizeof(WindowID)) : -1;
    if(index == -1)
        index = delta > 0 ? -1 : 0;
    FOR_EACH(WindowInfo*, winInfo, stack) {
        index = getNextIndex(stack, index, delta);
        if(isActivatable(winInfo) && (!filter || filter(winInfo))) {
            return index;
        }
    }
    DEBUG("Could not find window");
    return -1;
}
static WindowInfo* getNextWindowInStack(const ArrayList* stack, int delta, bool(*filter)(WindowInfo*)) {
    int index = getNextIndexInStack(stack, delta, filter);
    return index != -1 ? getElement(stack, index) : NULL;
}

void cycleWindowsMatching(int delta, bool(*filter)(WindowInfo*)) {
    WindowInfo* winInfo = getNextWindowInStack(getActiveMasterWindowStack(), delta, filter);
    if(winInfo)
        activateWindow(winInfo);
}

//////Run or Raise code



static void applyAction(WindowInfo* winInfo, WindowAction action) {
    switch(action) {
        case ACTION_ACTIVATE:
            activateWindow(winInfo);
            break;
        case ACTION_FOCUS:
            focusWindowInfo(winInfo, getActiveMaster());
            break;
        case ACTION_LOWER:
            lowerWindowInfo(winInfo, 0);
            break;
        case ACTION_NONE:
            break;
        case ACTION_RAISE:
            raiseWindowInfo(winInfo, 0);
            break;
    }
}

WindowInfo* findWindow(const WindowFunctionArg* rule, const ArrayList* searchList, bool includeNonActivatable) {
    FOR_EACH(WindowInfo*, winInfo, searchList) {
        if((includeNonActivatable || isActivatable(winInfo)) && rule->func(winInfo, rule->arg)) {
            return winInfo;
        }
    }
    return NULL;
}
WindowInfo* findAndRaise(const WindowFunctionArg* rule, WindowAction action, const FindAndRaiseArg arg) {
    WindowInfo* target = NULL;
    if(!arg.skipMasterStack)
        target = findWindow(rule, getMasterWindowStack(getActiveMaster()), arg.includeNonActivatable);
    if(!target) {
        target = findWindow(rule, getAllWindows(), arg.includeNonActivatable);
        if(target)
            DEBUG("found window globally");
    }
    else
        DEBUG("found window locally");
    if(target) {
        applyAction(target, action);
    }
    else
        DEBUG("could not find window");
    return target;
}
bool matchesClass(WindowInfo* winInfo, const char* str) {
    INFO("MatchesClass %d ''%s '%s' vs  %s", winInfo->id, winInfo->className, winInfo->instanceName, str);
    return strcmp(winInfo->className, str) == 0 || strcmp(winInfo->instanceName, str) == 0;
}
bool matchesTitle(WindowInfo* winInfo, const char* str) {
    return strcmp(winInfo->title, str) == 0;
}
bool matchesRole(WindowInfo* winInfo, const char* str) {
    return strcmp(winInfo->role, str) == 0;
}

static inline bool(*getMatchType(RaiseOrRunType matchType))(WindowInfo*, const char*) {
    switch(matchType) {
        default:
        case MATCHES_CLASS:
            return matchesClass;
        case MATCHES_TITLE:
            return matchesTitle;
        case MATCHES_ROLE:
            return matchesRole;
    }
}
int raiseOrRun2(const char* s, const char* cmd, RaiseOrRunType matchType) {
    if(s[0] == '$') {
        const char* c = getenv(s + 1);
        s = c ? c : "";
    }
    WindowFunctionArg arg = {getMatchType(matchType), .arg.str = s};
    if(!findAndRaise(&arg, ACTION_ACTIVATE, (FindAndRaiseArg) {0})) {
        return spawn(cmd);
    }
    return 0;
}
bool activateNextUrgentWindow() {
    WindowFunctionArg arg = {hasMask, URGENT_MASK};
    return findAndRaise(&arg, ACTION_ACTIVATE, (FindAndRaiseArg) {0});
}
bool popHiddenWindow() {
    WindowFunctionArg arg = {hasMask, HIDDEN_MASK};
    WindowInfo* winInfo = findAndRaise(&arg, ACTION_NONE, (FindAndRaiseArg) {.includeNonActivatable = 1});
    if(winInfo) {
        removeMask(winInfo, HIDDEN_MASK);
        applyAction(winInfo, ACTION_ACTIVATE);
    }
    return winInfo ? 1 : 0;
}

void shiftTopAndFocus() {
    ArrayList* stack = getActiveWindowStack();
    if(stack->size) {
        shiftToHead(stack, getNextIndexInStack(stack, getHead(stack) == getFocusedWindow(), NULL));
        activateWindow(getHead(stack));
    }
}
void swapPosition(int dir) {
    ArrayList* stack = getActiveWindowStack();
    if(stack->size) {
        swapElements(stack, getNextIndexInStack(stack, 0, NULL), stack, getNextIndexInStack(stack, dir, NULL));
    }
}
void shiftFocus(int dir, bool(*filter)(WindowInfo*)) {
    ArrayList* stack = getActiveWindowStack();
    WindowInfo* winInfo = getNextWindowInStack(stack, dir, filter);
    if(winInfo)
        activateWindow(winInfo);
}

void sendWindowToWorkspaceByName(WindowInfo* winInfo, const char* name) {
    Workspace* w = getWorkspaceByName(name);
    if(w)
        moveToWorkspace(winInfo, w->id);
}
void centerMouseInWindow(WindowInfo* winInfo) {
    warpPointer(winInfo->geometry.width / 2, winInfo->geometry.height / 2, winInfo->id, getActiveMasterPointerID());
}
Monitor* getSmallestMonitorContainingRect(const Rect rect) {
    Monitor* smallestMonitor = NULL;
    FOR_EACH(Monitor*, m, getAllMonitors()) {
        if(intersects(m->base, rect) && getWorkspaceOfMonitor(m))
            if(!smallestMonitor || getArea(smallestMonitor->base) > getArea(m->base))
                smallestMonitor = m;
    }
    return smallestMonitor;
}
void activateWorkspaceUnderMouse(void) {
    short pos[2];
    Master* master = getActiveMaster();
    Monitor* smallestMonitor = NULL;
    if(getMousePosition(getPointerID(master), root, pos)) {
        Rect rect = {pos[0], pos[1], 0, 0};
        smallestMonitor = getSmallestMonitorContainingRect(rect);
    }
    if(smallestMonitor)
        switchToWorkspace(getWorkspaceOfMonitor(smallestMonitor)->id);
}




typedef struct RefWindowMouse {
    WindowID win;
    Rect ref;
    short mousePos[2];
    char change;
    bool move;
    //bool btn;
    uint32_t lastSeqNumber;
} RefWindowMouse;
static RefWindowMouse* getRef() {
    return getActiveMaster()->windowMoveResizer;
}
static void removeRef() {
    free(getActiveMaster()->windowMoveResizer);
    getActiveMaster()->windowMoveResizer = NULL;
}
static inline Rect calculateNewPosition(const RefWindowMouse* refStruct, const short newMousePos[2],
    bool* hasChanged) {
    *hasChanged = 0;
    Rect _result = refStruct->ref;
    short* result = &_result.x;
    int mask[] = {1, 2};
    for(int i = 0; i < 2; i++) {
        short delta = newMousePos[i] - refStruct->mousePos[i];
        if(!(refStruct->change & mask[i]) && delta) {
            *hasChanged = 1;
            if(refStruct->move)
                result[i] += delta;
            else if((signed)(delta + result[2 + i]) < 0) {
                result[i] += delta + result[i + 2];
                result[i + 2 ] = -delta - result[i + 2];
            }
            else
                result[i + 2] += delta;
            if(result[i + 2] == 0)
                result[i + 2] = 1;
        }
    }
    return _result;
}
bool shouldUpdate(RefWindowMouse* refStruct) {
    if(getLastDetectedEventSequenceNumber() <= refStruct->lastSeqNumber) {
        return 0;
    }
    refStruct->lastSeqNumber = getLastDetectedEventSequenceNumber();
    return 1;
}
/*
bool detectWindowMoveResizeButtonRelease(Master* m) {
    xcb_input_button_release_event_t* event = (xcb_input_button_release_event_t*)getLastEvent();
    auto ref = getRef(m);
    if(ref) {
        if(ref->btn && ref->btn == event->detail) {
            commitWindowMoveResize(m);
            return 0;
        }
    }
    return 1;
}
*/

void startWindowMoveResize(WindowInfo* winInfo, bool move, int change) {
    if(!getRef()) {
        WindowID win = winInfo->id;
        DEBUG("Starting WM move/resize; Master: %d", getActiveMasterKeyboardID());
        short pos[2] = {0, 0};
        getMousePosition(getActiveMasterPointerID(), root, pos);
        RefWindowMouse temp = {.win = win, .ref = getRealGeometry(win), {pos[0], pos[1]}, .change = change, move};
        getActiveMaster()->windowMoveResizer = malloc(sizeof(RefWindowMouse));
        *((RefWindowMouse*)getActiveMaster()->windowMoveResizer) = temp;
    }
}
void commitWindowMoveResize() {
    DEBUG("Committing WM move/resize; Master: %d", getActiveMasterKeyboardID());
    /* TODO
    if(ref->btn)
        ungrabDevice(m->getPointerID());
    */
    removeRef();
}
void cancelWindowMoveResize() {
    RefWindowMouse* ref = getRef();
    if(ref) {
        DEBUG("Canceling WM move/resize; Master: %d", getActiveMasterKeyboardID());
        setWindowPosition(ref->win, ref->ref);
        /* TODO
        if(ref->btn)
            ungrabDevice(m->getPointerID());
            */
        removeRef();
    }
}

void updateWindowMoveResize() {
    RefWindowMouse* ref = getRef();
    if(ref) {
        TRACE("Updating WM move/resize; Master: %d", getActiveMasterKeyboardID());
        if(!shouldUpdate(ref))
            return;
        short pos[2];
        if(getMousePosition(getActiveMasterPointerID(), root, pos)) {
            bool change = 0;
            Rect r = calculateNewPosition(ref, pos, &change);
            assert(r.width && r.height);
            if(change)
                setWindowPosition(ref->win, r);
        }
    }
}
