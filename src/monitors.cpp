#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "logger.h"
#include "masters.h"
#include "monitors.h"
#include "windows.h"
#include "workspaces.h"

///list of all monitors
static ArrayList<Monitor*> monitors;


ArrayList<Monitor*>& getAllMonitors(void) {
    return monitors;
}

uint32_t MONITOR_DUPLICATION_POLICY = SAME_DIMS;
uint32_t MONITOR_DUPLICATION_RESOLUTION = TAKE_PRIMARY | TAKE_LARGER;


std::ostream& operator>>(std::ostream& strm, const Monitor& m) {
    return strm << "id:" << m.getID() << (m.isPrimary() ? "*" : "") << (m.isFake() ? "?" : "") << ", name:" << m.getName();
}

std::ostream& operator<<(std::ostream& strm, const Monitor& m) {
    strm >> m;
    if(m.getWorkspace())
        strm << ", Workspace:" << m.getWorkspace()->getID();
    strm << ", base:" << m.getBase();
    if(m.getBase() != m.getViewport()) {
        strm << ", viewport: " << m.getViewport();
    }
    return strm;
}


void resizeAllMonitorsToAvoidAllDocks(void) {
    for(Monitor* m : getAllMonitors())
        m->resizeToAvoidAllDocks();
}
bool Monitor::resizeToAvoidAllDocks() {
    reset();
    bool result = false;
    for(WindowInfo* winInfo : getAllWindows())
        if(winInfo->isDock() && winInfo->hasMask(MAPPED_MASK))
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
        if((&view.width)[offset] == 0)
            (&view.width)[offset] = 1;
        if(fromPositiveSide)
            (&view.x)[offset] = dim;
        changed = 1;
    }
    if(changed)
        INFO("Monitor " << getID() << " was resized because of " << winInfo->getID() << "; new size is: " << getViewport());
    return changed;
}
Monitor* addRootMonitor() {
    return addFakeMonitor({0, 0, getRootWidth(), getRootHeight()}, "ROOT");
}
void removeDuplicateMonitors(void) {
    if(!MONITOR_DUPLICATION_POLICY || !MONITOR_DUPLICATION_RESOLUTION) {
        TRACE("MONITOR_DUPLICATION_POLICY or MONITOR_DUPLICATION_RESOLUTION is not set");
        return;
    }
    for(int32_t i = getAllMonitors().size() - 1; i >= 0; i--) {
        Monitor* m1 = getAllMonitors()[i];
        for(int32_t n = getAllMonitors().size() - 1; n > i; n--) {
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
            LOG(LOG_LEVEL_DEBUG, "Monitors %u and %u are duplicates", m1->getID(), m2->getID());
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
                if(monitorToRemove == m1)
                    break;
            }
        }
    }
}
static inline Workspace* findWorkspace() {
    if(!getActiveWorkspace()->isVisible())
        return getActiveWorkspace();
    for(Master* master : getAllMasters())
        if(!getWorkspace(master->getWorkspaceIndex())->isVisible())
            return getWorkspace(master->getWorkspaceIndex());
    const Workspace* base = ::getWorkspace(getNumberOfWorkspaces() - 1);
    auto* workspace = base->getNextWorkspace(1, HIDDEN | NON_EMPTY);
    if(!workspace)
        workspace = base->getNextWorkspace(1, HIDDEN);
    return workspace;
}
void Monitor::assignWorkspace(Workspace* workspace) {
    assert(!getWorkspace());
    if(!workspace)
        workspace = findWorkspace();
    if(workspace)
        workspace->setMonitor(this);
}
void assignUnusedMonitorsToWorkspaces(void) {
    for(Monitor* m : getAllMonitors()) {
        if(!m->getWorkspace()) {
            DEBUG("Trying to auto assign workspace to Monitor: " << *m);
            m->assignWorkspace();
        }
    }
}
Monitor::~Monitor() {
    if(getWorkspace())
        getWorkspace()->setMonitor(NULL);
}

static MonitorID primaryMonitor = 0;
Monitor* getPrimaryMonitor() {
    return getAllMonitors().find(primaryMonitor);
}

void Monitor::setPrimary(bool primary) {
    if(isPrimary() != primary)
        primaryMonitor = primary ? getID() : 0;
}
bool Monitor::isPrimary() const {
    return primaryMonitor == getID();
}

void Monitor::reset() {
    memcpy(&view, &base, sizeof(base));
}
void Monitor::setBase(const Rect& rect) {
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
void removeAllFakeMonitors() {
    for(int i = getAllMonitors().size() - 1; i >= 0; i--)
        if(getAllMonitors()[i]->isFake())
            delete getAllMonitors().remove(i);
}
Monitor* addFakeMonitor(Rect bounds, std::string name) {
    MonitorID id;
    for(id = 2;; id++)
        if(!getAllMonitors().find(id))
            break;
    Monitor* m = new Monitor(id, bounds, 0, name, 1);
    getAllMonitors().add(m);
    return m;
}
