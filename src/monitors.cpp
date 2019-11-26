#include <assert.h>
#include <stdlib.h>
#include <string.h>


#include <iostream>

#include "globals.h"
#include "logger.h"
#include "monitors.h"
#include "workspaces.h"
#include "windows.h"
#include "masters.h"

///list of all monitors
static ArrayList<Monitor*> monitors;


ArrayList<Monitor*>& getAllMonitors(void) {
    return monitors;
}

uint32_t MONITOR_DUPLICATION_POLICY = SAME_DIMS;
uint32_t MONITOR_DUPLICATION_RESOLUTION = TAKE_PRIMARY | TAKE_LARGER;


std::ostream& operator<<(std::ostream& strm, const Monitor& m) {
    strm << "{id:" << m.getID() << (m.isPrimary() ? "*" : "") << ", name:" << m.getName();
    if(m.getWorkspace())
        strm << ", Workspace:" << m.getWorkspace()->getID();
    strm << ", base:" << m.getBase();
    if(m.getBase() != m.getViewport()) {
        strm << ", viewport: " << m.getViewport();
    }
    return strm  << "}";
}


void resizeAllMonitorsToAvoidAllDocks(void) {
    for(Monitor* m : getAllMonitors())
        m->resizeToAvoidAllDocks();
}
bool Monitor::resizeToAvoidAllDocks() {
    reset();
    bool result = false;
    for(WindowInfo* winInfo : getAllWindows())
        if(winInfo->isDock())
            result |= resizeToAvoidDock(winInfo);
    return result;
}

bool Monitor::resizeToAvoidDock(WindowInfo* winInfo) {
    assert(winInfo && winInfo->isDock());
    if(winInfo->getWorkspace() && winInfo->getWorkspace()->getMonitor() != this ||
        !winInfo->getWorkspace() && winInfo->hasMask(PRIMARY_MONITOR_MASK) && !isPrimary())
        return 0;
    bool changed = 0;
    auto properties = winInfo->getDockProperties();
    for(int i = 0; i < 4; i++) {
        int dim = properties[i];
        if(dim == 0)
            continue;
        bool fromPositiveSide = (i == DOCK_LEFT) || (i == DOCK_TOP);
        bool offset = (i == DOCK_TOP) || (i == DOCK_BOTTOM);
        int start = properties[i * 2 + 4];
        int end = properties[i * 2 + 4 + 1];
        if(end == 0)
            end = offset ? getRootWidth() : getRootHeight();
        assert(start <= end);
        short int values[] = {0, 0, 0, 0};
        values[offset] = fromPositiveSide ? 0 : (offset ? getRootHeight() : getRootWidth()) - dim;
        values[!offset] = start;
        values[offset + 2] = dim;
        values[!offset + 2] = end - start;
        if(!view.intersects(*(Rect*)values))
            continue;
        int intersectionWidth = fromPositiveSide ?
            dim - (&view.x)[offset] :
            (&view.x)[offset] + (&view.width)[offset] - values[offset];
        assert(intersectionWidth > 0);
        (&view.width)[offset] -= intersectionWidth;
        if(fromPositiveSide)
            (&view.x)[offset] = dim;
        changed = 1;
    }
    return changed;
}
void addRootMonitor() {
    getAllMonitors().add(new Monitor(1, {0, 0, getRootWidth(), getRootHeight()}, 0), ADD_UNIQUE);
}
void removeDuplicateMonitors(void) {
    if(!MONITOR_DUPLICATION_POLICY || !MONITOR_DUPLICATION_RESOLUTION) {
        LOG(LOG_LEVEL_TRACE, "MONITOR_DUPLICATION_POLICY or MONITOR_DUPLICATION_RESOLUTION is not set\n");
        return;
    }
    for(uint32_t i = 0; i < getAllMonitors().size(); i++) {
        Monitor* m1 = getAllMonitors()[i];
        for(uint32_t n = i + 1; n < getAllMonitors().size(); n++) {
            Monitor* m2 = getAllMonitors()[n];
            bool dup =
                (MONITOR_DUPLICATION_POLICY & SAME_DIMS) && (m1->getBase() == m2->getBase()) ||
                (MONITOR_DUPLICATION_POLICY & INTERSECTS) && m1->getBase().intersects(m2->getBase()) ||
                (MONITOR_DUPLICATION_POLICY & CONTAINS) && (m1->getBase().contains(m2->getBase()) ||
                    m2->getBase().contains(m1->getBase()));
            (MONITOR_DUPLICATION_POLICY & CONTAINS_PROPER)&& (m1->getBase().containsProper(m2->getBase()) ||
                m2->getBase().containsProper(m1->getBase()));
            if(!dup)
                continue;
            Monitor* monitorToRemove = NULL;
            LOG(LOG_LEVEL_DEBUG, "Monitors %u and %u are duplicates\n", m1->getID(), m2->getID());
            if(MONITOR_DUPLICATION_RESOLUTION & TAKE_SMALLER)
                monitorToRemove = m1->getBase().isLargerThan(m2->getBase()) ? m1 : m2;
            if(MONITOR_DUPLICATION_RESOLUTION & TAKE_LARGER)
                monitorToRemove = m2->getBase().isLargerThan(m1->getBase()) ? m1 : m2;
            if(MONITOR_DUPLICATION_RESOLUTION & TAKE_PRIMARY)
                monitorToRemove = m1->isPrimary() ? m2 : m2->isPrimary() ? m1 : monitorToRemove;;
            if(monitorToRemove) {
                LOG(LOG_LEVEL_DEBUG, "removing monitor %u because it is a duplicates of %u", monitorToRemove->getID(),
                    monitorToRemove->getID() ^ m1->getID() ^ m2->getID());
                delete getAllMonitors().removeElement(monitorToRemove);
            }
        }
    }
}

void Monitor::assignWorkspace(Workspace* workspace) {
    if(!workspace && !getAllWorkspaces().empty()) {
        Workspace* base = ::getWorkspace(0);
        if(base->isVisible()) {
            workspace = base->getNextWorkspace(1, HIDDEN | NON_EMPTY);
            if(!workspace)
                workspace = base->getNextWorkspace(1, HIDDEN);
        }
        else workspace = base;
    }
    if(workspace)
        workspace->setMonitor(this);
}
void assignUnusedMonitorsToWorkspaces(void) {
    for(Monitor* m : getAllMonitors()) {
        if(!m->getWorkspace())
            m->assignWorkspace();
    }
}
Monitor::~Monitor() {
    if(getWorkspace())
        getWorkspace()->setMonitor(NULL);
}

static MonitorID primaryMonitor = 0;
void setPrimary(Monitor* monitor) {
    primaryMonitor = monitor ? monitor->getID() : 0;
}
Monitor* getPrimaryMonitor() {
    return getAllMonitors().find(primaryMonitor);
}

void Monitor::setPrimary(bool primary) {
    if(isPrimary() != primary)
        ::setPrimary(primary ? this : NULL);
}
bool Monitor::isPrimary() const {
    return primaryMonitor == getID();
}

void Monitor::reset() {
    memcpy(&view, &base, sizeof(base));
}
void Monitor::setBase(Rect rect) {
    base = rect;
    reset();
}
Workspace* Monitor::getWorkspace(void) const {
    for(Workspace* workspace : getAllWorkspaces())
        if(workspace->getMonitor() == this)
            return workspace;
    return NULL;
}

static uint16_t* rootDim;
void setRootDims(uint16_t* s) {
    rootDim = s;
}
uint16_t getRootWidth(void) {
    return rootDim ? rootDim[0] : 0;
}

uint16_t getRootHeight(void) {
    return rootDim ? rootDim[1] : 0;
}
void swapMonitors(WorkspaceID index1, WorkspaceID index2) {
    Monitor* temp = getWorkspace(index1)->getMonitor();
    getWorkspace(index1)->setMonitor(getWorkspace(index2)->getMonitor());
    getWorkspace(index2)->setMonitor(temp);
}
