#include <assert.h>

#include "bindings.h"
#include "devices.h"
#include "functions.h"
#include "globals.h"
#include "layouts.h"
#include "util/logger.h"
#include "masters.h"
#include "monitors.h"
#include "system.h"
#include "xutil/test-functions.h"
#include "xutil/window-properties.h"
#include "windows.h"
#include "wmfunctions.h"
#include "workspaces.h"

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

WindowInfo* findWindow(const FindWindowArg* rule, const ArrayList* searchList, bool includeNonActivatable) {
    FOR_EACH(WindowInfo*, winInfo, searchList) {
        if((includeNonActivatable || isActivatable(winInfo)) && rule->func(winInfo, rule->arg)) {
            return winInfo;
        }
    }
    return NULL;
}
WindowInfo* findAndRaise(const FindWindowArg* rule, WindowAction action, const FindAndRaiseArg arg) {
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
    FindWindowArg arg = {getMatchType(matchType), .arg.str = s};
    if(!findAndRaise(&arg, ACTION_ACTIVATE, (FindAndRaiseArg) {0})) {
        return spawn(cmd);
    }
    return 0;
}
bool activateNextUrgentWindow() {
    FindWindowArg arg = {hasMask, URGENT_MASK};
    return findAndRaise(&arg, ACTION_ACTIVATE, (FindAndRaiseArg) {0});
}
bool popHiddenWindow() {
    FindWindowArg arg = {hasMask, HIDDEN_MASK};
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
void activateWorkspaceUnderMouse(void) {
    short pos[2];
    Master* master = getActiveMaster();
    Monitor* smallestMonitor = NULL;
    if(getMousePosition(getPointerID(master), root, pos)) {
        Rect rect = {pos[0], pos[1], 1, 1};
        FOR_EACH(Monitor*, m, getAllMonitors()) {
            if(intersects(m->base, rect) && getWorkspaceOfMonitor(m))
                if(!smallestMonitor || getArea(smallestMonitor->base) > getArea(m->base))
                    smallestMonitor = m;
        }
    }
    if(smallestMonitor)
        switchToWorkspace(getWorkspaceOfMonitor(smallestMonitor)->id);
}

