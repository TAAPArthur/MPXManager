/**
 * @file workspaces.c
 * @copybrief workspaces.h
 *
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "globals.h"
#include "workspaces.h"
#include "mywm-structs.h"
#include "windows.h"


///list of all workspaces
static ArrayList<Workspace*> workspaces;
const ArrayList<Workspace*>& getAllWorkspaces() {
    return workspaces;
}


std::ostream& operator<<(std::ostream& strm, const Workspace& w) {
    return strm << "{ID:" << w.getID() << (w.isVisible() ? "*" : "") << ", name:" << w.getName() << ", windows: " >>
           w.windows << ", Layout: " << w.getActiveLayout() << "}";
}

void addWorkspaces(int num) {
    for(int i = 0; i < num; i++)
        workspaces.add(new Workspace());
}
void removeWorkspaces(int num) {
    for(int i = 0; i < num && getNumberOfWorkspaces() > 1; i++)
        delete workspaces.pop();
}
void removeAllWorkspaces() {
    workspaces.deleteElements();
}

void Workspace::setMonitor(Monitor* m) {
    monitor = m;
};
int getNumberOfWorkspaces() {
    return getAllWorkspaces().size();
}
//workspace methods

Workspace* Workspace::getNextWorkspace(int dir, int mask) {
    int empty = ((mask >> 2) & 3) - 1;
    int hidden = (mask & 3) - 1;
    int index = id;
    for(int i = 0; i < getNumberOfWorkspaces(); i++) {
        index = getAllWorkspaces().getNextIndex(index, dir);
        if((hidden == -1 || hidden == !getWorkspace(index)->isVisible()) &&
                (empty == -1 || empty == getWorkspace(index)->getWindowStack().empty())
          )
            return getWorkspace(index);
    }
    return NULL;
}

Workspace* getWorkspaceByName(std::string name) {
    for(Workspace* w : getAllWorkspaces())
        if(w->getName() == name)
            return w;
    return NULL;
}
void Workspace::setLayoutOffset(int offset) {
    layoutOffset = offset;
}
void Workspace::cycleLayouts(int dir) {
    auto& layouts = getLayouts();
    if(layouts.empty())
        return;
    int pos = layouts.getNextIndex(layoutOffset, dir);
    setActiveLayout(layouts[pos]);
    setLayoutOffset(pos);
}
int Workspace::toggleLayout(Layout* layout) {
    if(layout != getActiveLayout()) {
        setActiveLayout(layout);
    }
    else {
        cycleLayouts(0);
        if(layout == getActiveLayout())
            return 0;
    }
    return 1;
}
bool Workspace::hasWindowWithMask(WindowMask mask) {
    for(WindowInfo* winInfo : getWindowStack())
        if(winInfo->hasMask(mask))
            return 1;
    return 0;
}
