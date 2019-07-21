/**
 * @file functions.c
 * @copybrief functions.h
 */

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
#include "window-properties.h"
#include "windows.h"
#include "wmfunctions.h"
#include "workspaces.h"

static int getNextIndexInStack(const ArrayList<WindowInfo*>& stack, int delta,
                               WindowInfo* winInfo = getFocusedWindow()) {
    if(stack.empty())
        return -1;
    int index = 0;
    if(winInfo)
        index = stack.indexOf(winInfo);
    if(index == -1)
        index = 0;
    int dir = delta == 0 ? 0 : delta > 0 ? 1 : -1;
    for(int i = 0; i < stack.size(); i++) {
        int n = stack.getNextIndex(index, delta);
        if(stack[n]->isActivatable()) {
            return n;
        }
        delta += dir;
    }
    return -1;
}
void cycleWindows(int delta, Master* master) {
    master->setFocusStackFrozen(1);
    int index = getNextIndexInStack(master->getWindowStack(), -delta, master->getFocusedWindow());
    if(index >= 0)
        activateWindow(master->getWindowStack()[index]);
}

void endCycleWindows(Master* master) {
    master->setFocusStackFrozen(0);
}

//////Run or Raise code



static void applyAction(WindowInfo* winInfo, WindowAction action) {
    switch(action) {
        case ACTION_RAISE:
            raiseWindowInfo(winInfo);
            break;
        case ACTION_FOCUS:
            focusWindow(winInfo);
            break;
        case ACTION_ACTIVATE:
            activateWindow(winInfo);
            break;
        case ACTION_LOWER:
            lowerWindowInfo(winInfo);
            break;
    }
}
static ArrayList<WindowID>* getWindowCache(Master* m) {
    static Index<ArrayList<uint32_t>> key;
    return get(key, m);
}

WindowInfo* findWindow(const BoundFunction& rule, ArrayList<WindowInfo*>& searchList, ArrayList<WindowID>* ignoreList) {
    for(WindowInfo* winInfo : searchList)
        if((!ignoreList || !ignoreList->find(winInfo->getID())) && winInfo->isMappable() && rule(winInfo))
            return winInfo;
    return NULL;
}
WindowInfo* findAndRaise(const BoundFunction& rule, WindowAction action, bool checkLocalFirst, bool cache,
                         Master* master) {
    ArrayList<WindowID>* windowsToIgnore = cache ? getWindowCache(getActiveMaster()) : NULL;
    if(cache && windowsToIgnore && master->getFocusedWindow())
        if(!rule(master->getFocusedWindow())) {
            LOG(LOG_LEVEL_DEBUG, "clearing window cache\n");
            windowsToIgnore->clear();
        }
        else if(windowsToIgnore->size() > 1 && windowsToIgnore->addUnique(master->getFocusedWindow()->getID()) == 0) {
            windowsToIgnore->clear();
            windowsToIgnore->add(master->getFocusedWindow()->getID());
            LOG(LOG_LEVEL_DEBUG, "clearing window cache and adding focused window\n");
            std::cout << *windowsToIgnore << "\n" << master->getFocusedWindow()->getID() << "\n";
        }
    WindowInfo* target = checkLocalFirst ? findWindow(rule, master->getWindowStack(), windowsToIgnore) : NULL;
    if(!target) {
        target = findWindow(rule, getAllWindows(), windowsToIgnore);
        if(!target && windowsToIgnore && !windowsToIgnore->empty()) {
            target = findWindow(rule, master->getWindowStack(), NULL);
            windowsToIgnore->clear();
        }
        else
            LOG(LOG_LEVEL_DEBUG, "found window globally\n");
    }
    else LOG(LOG_LEVEL_DEBUG, "found window locally\n");
    if(target) {
        applyAction(target, action);
        if(windowsToIgnore)
            windowsToIgnore->add(target->getID());
    }
    else
        LOG(LOG_LEVEL_DEBUG, "could not find window\n");
    return target;
}

WindowInfo* getNextWindowInStack(int dir, const ArrayList<WindowInfo*>& stack) {
    int index = getNextIndexInStack(stack, dir);
    return index != -1 ? stack[index] : NULL;
}


int focusBottom(const ArrayList<WindowInfo*>& stack) {
    return !stack.empty() ? activateWindow(stack.back()) : 0;
}
int focusTop(const ArrayList<WindowInfo*>& stack) {
    return !stack.empty() ? activateWindow(stack[0]) : 0;
}
void shiftTop(ArrayList<WindowInfo*>& stack) {
    if(!stack.empty())
        stack.shiftToHead(getNextIndexInStack(stack, 0));
}
void swapWithTop(ArrayList<WindowInfo*>& stack) {
    if(!stack.empty())
        stack.swap(0, getNextIndexInStack(stack, 0));
}
void swapPosition(int dir, ArrayList<WindowInfo*>& stack) {
    if(!stack.empty())
        stack.swap(getNextIndexInStack(stack, 0),            getNextIndexInStack(stack, dir));
}
int shiftFocus(int dir, ArrayList<WindowInfo*>& stack) {
    if(!stack.empty())
        return activateWindow(stack[getNextIndexInStack(stack, dir)]);
    return 0;
}

void sendWindowToWorkspaceByName(WindowInfo* winInfo, std::string name) {
    Workspace* w = getWorkspaceByName(name);
    if(w)
        winInfo->moveToWorkspace(w->getID());
}
bool activateWorkspaceUnderMouse(void) {
    short pos[2];
    Master* master = getActiveMaster();
    getMousePosition(master->getPointerID(), root, pos);
    Rect rect = {pos[0], pos[1], 1, 1};
    for(Monitor* m : getAllMonitors()) {
        if(m->getBase().intersects(rect) && m->getWorkspace()) {
            activateWorkspace(m->getWorkspace()->getID());
            return 1;
        }
    }
    return 0;
}
