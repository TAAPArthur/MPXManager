#include <assert.h>

#include "bindings.h"
#include "devices.h"
#include "ext.h"
#include "functions.h"
#include "globals.h"
#include "layouts.h"
#include "logger.h"
#include "masters.h"
#include "monitors.h"
#include "system.h"
#include "test-functions.h"
#include "window-properties.h"
#include "windows.h"
#include "wmfunctions.h"
#include "workspaces.h"

static int getNextIndexInStack(const ArrayList<WindowInfo*>& stack, int delta, bool filter = 0,
    WindowInfo* winInfo = getFocusedWindow()) {
    if(stack.empty())
        return -1;
    int index = winInfo ? stack.indexOf(winInfo) : -1;
    if(index == -1)
        index = delta > 0 ? -1 : 0;
    int dir = delta == 0 ? 0 : delta > 0 ? 1 : -1;
    for(int i = stack.size() - 1; i >= 0; i--) {
        int n = stack.getNextIndex(index, delta);
        if(stack[n]->isInteractable() && (!filter || stack[n]->isActivatable())) {
            return n;
        }
        delta += dir;
    }
    return -1;
}

bool hasInteractiveWindow(const ArrayList<WindowInfo*>& stack) {
    for(WindowInfo* winInfo : stack)
        if(winInfo->isInteractable())
            return 1;
    return 0;
}

void cycleWindows(int delta) {
    Master* master = getActiveMaster();
    master->setFocusStackFrozen(1);
    int index = getNextIndexInStack(master->getWindowStack(), -delta, 1, master->getFocusedWindow());
    if(index >= 0)
        activateWindow(master->getWindowStack()[index]);
}

void endCycleWindows(Master* master) {
    master->setFocusStackFrozen(0);
}

//////Run or Raise code



static void applyAction(WindowInfo* winInfo, WindowAction action) {
    switch(action) {
        default:
        case ACTION_RAISE:
            raiseWindow(winInfo->getID());
            break;
        case ACTION_FOCUS:
            focusWindow(winInfo);
            break;
        case ACTION_ACTIVATE:
            activateWindow(winInfo);
            break;
        case ACTION_LOWER:
            lowerWindow(winInfo->getID());
            break;
    }
}
static ArrayList<WindowID>* getWindowCache(Master* m) {
    static Index<ArrayList<uint32_t>> key;
    return get(key, m);
}

WindowInfo* findWindow(const BoundFunction& rule, const ArrayList<WindowInfo*>& searchList,
    ArrayList<WindowID>* ignoreList, bool includeNonActivatable) {
    for(WindowInfo* winInfo : searchList)
        if((!ignoreList || !ignoreList->find(winInfo->getID())) && (includeNonActivatable || winInfo->isActivatable()) &&
            rule(winInfo))
            return winInfo;
    return NULL;
}
WindowInfo* findAndRaise(const BoundFunction& rule, WindowAction action, bool checkLocalFirst, bool cache,
    Master* master) {
    ArrayList<WindowID>* windowsToIgnore = cache ? getWindowCache(getActiveMaster()) : NULL;
    if(windowsToIgnore)
        if(cache && windowsToIgnore && master->getFocusedWindow())
            if(!rule(master->getFocusedWindow())) {
                LOG(LOG_LEVEL_DEBUG, "clearing window cache\n");
                windowsToIgnore->clear();
            }
            else if(windowsToIgnore->addUnique(master->getFocusedWindow()->getID())) {
                windowsToIgnore->clear();
                windowsToIgnore->add(master->getFocusedWindow()->getID());
                LOG(LOG_LEVEL_DEBUG, "clearing window cache and adding focused window\n");
            }
    WindowInfo* target = checkLocalFirst ? findWindow(rule, master->getWindowStack(), windowsToIgnore) : NULL;
    if(!target) {
        target = findWindow(rule, getAllWindows(), windowsToIgnore);
        if(!target && windowsToIgnore && !windowsToIgnore->empty()) {
            for(WindowID win : *windowsToIgnore) {
                WindowInfo* winInfo = getWindowInfo(win);
                if(winInfo && winInfo->isMappable() && rule(winInfo)) {
                    target = winInfo;
                    break;
                }
            }
            windowsToIgnore->clear();
            LOG(LOG_LEVEL_DEBUG, "found window by bypassing ignore list\n");
        }
        else
            LOG(LOG_LEVEL_DEBUG, "found window globally\n");
    }
    else LOG(LOG_LEVEL_DEBUG, "found window locally\n");
    if(target) {
        assert(rule(target));
        logger.debug() << "Applying action " << action << " to " << *target << std::endl;
        applyAction(target, action);
        if(windowsToIgnore)
            windowsToIgnore->add(target->getID());
    }
    else
        LOG(LOG_LEVEL_DEBUG, "could not find window\n");
    return target;
}
bool matchesClass(WindowInfo* winInfo, std::string str) {
    return winInfo->getClassName() == str || winInfo->getInstanceName() == str;
}
bool matchesTitle(WindowInfo* winInfo, std::string str) {
    return winInfo->getTitle() == str;
}
bool raiseOrRun(std::string s, std::string cmd, bool matchOnClass, bool silent) {
    if(s[0] == '$') {
        auto c = getenv(s.substr(1).c_str());
        s = c ? c : "";
    }
    const BoundFunction f = {matchOnClass ? matchesClass : matchesTitle, s, "raiseOrRunCheck",  PASSTHROUGH_IF_TRUE};
    if(!findAndRaise(f)) {
        spawn(cmd.c_str(), silent);
        return 0;
    }
    return 1;
}



int focusBottom(const ArrayList<WindowInfo*>& stack) {
    return hasInteractiveWindow(stack) ? activateWindow(stack.back()) : 0;
}
int focusTop(const ArrayList<WindowInfo*>& stack) {
    return hasInteractiveWindow(stack) ? activateWindow(stack[0]) : 0;
}
void shiftTop(ArrayList<WindowInfo*>& stack) {
    if(hasInteractiveWindow(stack))
        stack.shiftToHead(getNextIndexInStack(stack, stack[0] == getFocusedWindow()));
}
void swapWithTop(ArrayList<WindowInfo*>& stack) {
    if(hasInteractiveWindow(stack))
        stack.swap(0, getNextIndexInStack(stack, 0));
}
void swapPosition(int dir, ArrayList<WindowInfo*>& stack) {
    if(hasInteractiveWindow(stack))
        stack.swap(getNextIndexInStack(stack, 0), getNextIndexInStack(stack, dir));
}
int shiftFocus(int dir, ArrayList<WindowInfo*>& stack) {
    int index = getNextIndexInStack(stack, dir, 1);
    return index != -1 ? activateWindow(stack[index]) : 0;
}

void sendWindowToWorkspaceByName(WindowInfo* winInfo, std::string name) {
    Workspace* w = getWorkspaceByName(name);
    if(w)
        winInfo->moveToWorkspace(w->getID());
}
void centerMouseInWindow(WindowInfo* winInfo) {
    Rect dims = winInfo->getGeometry();
    if(getFocusedWindow())
        movePointer(dims.width / 2, dims.height / 2, winInfo->getID());
}
void activateWorkspaceUnderMouse(void) {
    short pos[2];
    Master* master = getActiveMaster();
    if(getMousePosition(master->getPointerID(), root, pos)) {
        Rect rect = {pos[0], pos[1], 1, 1};
        for(Monitor* m : getAllMonitors()) {
            if(m->getBase().intersects(rect) && m->getWorkspace()) {
                switchToWorkspace(m->getWorkspace()->getID());
                return;
            }
        }
    }
}
