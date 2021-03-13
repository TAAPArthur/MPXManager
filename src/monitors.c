#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "util/logger.h"
#include "util/rect.h"
#include "monitors.h"
#include "windows.h"
#include "workspaces.h"

///list of all monitors
static ArrayList monitors;


ArrayList* getAllMonitors(void) {
    return &monitors;
}

__DEFINE_GET_X_BY_NAME(Monitor)


Monitor* getMonitorByID(MonitorID id) {
    return findElement(getAllMonitors(), &id, sizeof(MonitorID));
}

uint32_t MONITOR_DUPLICATION_POLICY = SAME_DIMS | CONSIDER_ONLY_NONFAKES;
uint32_t MONITOR_DUPLICATION_RESOLUTION = TAKE_PRIMARY | TAKE_LARGER;

Monitor* newMonitor(MonitorID id, Rect base, const char* name, bool fake) {
    Monitor* monitor = (Monitor*)malloc(sizeof(Monitor));
    Monitor temp = {.id = id, .base = base, .view = base, .fake = fake};
    memmove(monitor, &temp, sizeof(Monitor));
    strncpy(monitor->name, name, MAX_NAME_LEN - 1);
    addElement(getAllMonitors(), monitor);
    return monitor;
}
Monitor* addFakeMonitorWithName(Rect bounds, const char* name) {
    MonitorID id;
    for(id = 2;; id++)
        if(!findElement(getAllMonitors(), &id, sizeof(MonitorID)))
            break;
    Monitor* m = newMonitor(id, bounds, name, 1);
    return m;
}
void freeMonitor(Monitor* monitor) {
    if(getWorkspaceOfMonitor(monitor))
        setMonitor(getWorkspaceOfMonitor(monitor), NULL);
    removeElement(&monitors, monitor, sizeof(MonitorID));
    free(monitor);
}


void setStackingWindow(Monitor* mon, WindowID win) { mon->stackingWindow = win;}
WindowID getStackingWindow(Monitor* mon) { return mon->stackingWindow;}
bool isFakeMonitor(Monitor* mon) { return mon->fake;}

bool isMonitorActive(Monitor* monitor) {
    return !monitor->inactive && monitor->view.width && monitor->view.height;
}

/**
 *
 * Adjusts the viewport so that it doesn't intersect winInfo
 *
 * @param winInfo WindowInfo
 *
 * @return 1 iff the size changed
 */
bool resizeToAvoidDock(Monitor* monitor, WindowInfo* winInfo) {
    assert(winInfo && winInfo->dock);
    if(getWorkspaceOfWindow(winInfo) && getWorkspaceOfWindow(winInfo) != getWorkspaceOfMonitor(monitor) ||
        !getWorkspaceOfWindow(winInfo) && hasMask(winInfo, PRIMARY_MONITOR_MASK) && !isPrimary(monitor)) {
        TRACE("Monitor %d is skipping dock %d", monitor->id, winInfo->id);
        return 0;
    }
    DockType i = winInfo->dockProperties.type;
    int thickness = winInfo->dockProperties.thickness;
    if(!thickness || !hasMask(winInfo, MAPPED_MASK)) {
        TRACE("Dock %d has is disabled Thickness %d; Mapped %d", winInfo->id, thickness, hasMask(winInfo, MAPPED_MASK));
        return 0;
    }
    int start = winInfo->dockProperties.start;
    int end = winInfo->dockProperties.end;
    bool fromPositiveSide = (i == DOCK_LEFT) || (i == DOCK_TOP);
    bool offset = (i == DOCK_TOP) || (i == DOCK_BOTTOM);
    if(end == 0)
        end = offset ? getRootWidth() : getRootHeight();
    assert(start <= end);
    short int values[] = {0, 0, 0, 0};
    values[offset] = fromPositiveSide ? 0 : (offset ? getRootHeight() : getRootWidth()) - thickness;
    values[!offset] = start;
    values[offset + 2] = thickness;
    values[!offset + 2] = end - start;
    if(!intersects(monitor->view, *((Rect*)values)))
        return 0;
    int intersectionWidth = fromPositiveSide ?
        thickness - (&monitor->view.x)[offset] :
        (&monitor->view.x)[offset] + (&monitor->view.width)[offset] - values[offset];
    assert(intersectionWidth > 0);
    INFO("%d %d", intersectionWidth, (&monitor->view.width)[offset]);
    (&monitor->view.width)[offset] = (&monitor->view.width)[offset] > intersectionWidth ?
        (&monitor->view.width)[offset] - intersectionWidth : 0;
    INFO("%d %d", intersectionWidth, (&monitor->view.width)[offset]);
    if(fromPositiveSide)
        (&monitor->view.x)[offset] = thickness;
    INFO("Monitor %d was resized because of %d (thickness %d)", monitor->id, *((WindowID*)winInfo), thickness);
    return 1;
}

void resizeAllMonitorsToAvoidAllDocks(void) {
    FOR_EACH(Monitor*, monitor, getAllMonitors()) {
        monitor->view = monitor->base;
        FOR_EACH(WindowInfo*, winInfo, getAllWindows()) {
            if(winInfo->dock && hasMask(winInfo, MAPPED_MASK))
                resizeToAvoidDock(monitor, winInfo);
        }
    }
}

void resizeAllMonitorsToAvoidDock(WindowInfo* winInfo) {
    FOR_EACH(Monitor*, m, getAllMonitors()) {
        resizeToAvoidDock(m, winInfo);
    }
}

Monitor* addRootMonitor() {
    return addFakeMonitorWithName((Rect) {0, 0, getRootWidth(), getRootHeight()}, "ROOT");
}
void removeDuplicateMonitors(void) {
    if(!MONITOR_DUPLICATION_POLICY || !MONITOR_DUPLICATION_RESOLUTION) {
        TRACE("MONITOR_DUPLICATION_POLICY or MONITOR_DUPLICATION_RESOLUTION is not set");
        return;
    }
    for(int32_t i = getAllMonitors()->size - 1; i >= 0; i--) {
        Monitor* m1 = getElement(getAllMonitors(), i);
        for(int32_t n = getAllMonitors()->size - 1; n > i; n--) {
            Monitor* m2 = getElement(getAllMonitors(), n);
            if((m1->fake || m2->fake) && (MONITOR_DUPLICATION_POLICY & CONSIDER_ONLY_NONFAKES))
                continue;
            bool dup =
                (MONITOR_DUPLICATION_POLICY & SAME_DIMS) && memcmp(&m1->base, &m2->base, sizeof(Rect)) == 0 ||
                (MONITOR_DUPLICATION_POLICY & INTERSECTS) && intersects(m1->base, m2->base) ||
                (MONITOR_DUPLICATION_POLICY & CONTAINS) && (contains(m1->base, m2->base) || contains(m2->base, m1->base)) ||
                (MONITOR_DUPLICATION_POLICY & CONTAINS_PROPER) && (containsProper(m1->base, m2->base) ||
                    containsProper(m2->base, m1->base));
            if(!dup)
                continue;
            Monitor* monitorToRemove = NULL;
            DEBUG("Monitors %u and %u are duplicates", m1->id, m2->id);
            if(MONITOR_DUPLICATION_RESOLUTION & TAKE_SMALLER)
                monitorToRemove = getArea(m1->base) > getArea(m2->base) ? m1 : m2;
            if(MONITOR_DUPLICATION_RESOLUTION & TAKE_LARGER)
                monitorToRemove = getArea(m1->base) < getArea(m2->base) ? m1 : m2;
            if(MONITOR_DUPLICATION_RESOLUTION & TAKE_PRIMARY)
                monitorToRemove = isPrimary(m1) ? m2 : isPrimary(m2) ? m1 : monitorToRemove;;
            if(monitorToRemove) {
                DEBUG("removing monitor %u because it is a duplicate of %u", monitorToRemove->id,
                    monitorToRemove->id ^ m1->id ^ m2->id);
                freeMonitor(monitorToRemove);
                if(monitorToRemove == m1)
                    break;
            }
        }
    }
}

static inline Workspace* _findWorkspace(bool empty) {
    FOR_EACH(Workspace*, workspace, getAllWorkspaces()) {
        if(!isWorkspaceVisible(workspace) && (!empty == !getWorkspaceWindowStack(workspace)->size))
            return workspace;
    }
    return NULL;
}
static inline Workspace* findWorkspace() {
    if(!isWorkspaceVisible(getActiveWorkspace()))
        return getActiveWorkspace();
    FOR_EACH(Master*, master, getAllMasters()) {
        if(!isWorkspaceVisible(getWorkspace(getMasterWorkspaceIndex(master))))
            return getWorkspace(getMasterWorkspaceIndex(master));
    }
    Workspace* workspace = _findWorkspace(1);
    if(!workspace)
        workspace = _findWorkspace(0);
    return workspace;
}
void assignEmptyWorkspace(Monitor* monitor) {
    assert(!getWorkspaceOfMonitor(monitor));
    Workspace* workspace = _findWorkspace(0);
    if(workspace)
        setMonitor(workspace, monitor);
}
void assignWorkspace(Monitor* monitor, Workspace* workspace) {
    assert(!getWorkspaceOfMonitor(monitor));
    if(!workspace)
        workspace = findWorkspace();
    if(workspace)
        setMonitor(workspace, monitor);
}
void assignUnusedMonitorsToWorkspaces(void) {
    FOR_EACH(Monitor*, m, getAllMonitors()) {
        if(!getWorkspaceOfMonitor(m)) {
            DEBUG("Trying to auto assign workspace to Monitor: %d", m->id);
            assignWorkspace(m, NULL);
        }
    }
}

static MonitorID primaryMonitor = 0;
Monitor* getPrimaryMonitor() {
    return findElement(getAllMonitors(), &primaryMonitor, sizeof(MonitorID));
}

void setPrimary(MonitorID id) {
    primaryMonitor = id;
}
bool isPrimary(Monitor* monitor) {
    return primaryMonitor == monitor->id;
}

void setBase(Monitor* monitor, const Rect rect) {
    monitor->base = monitor->view = rect;
}
Workspace* getWorkspaceOfMonitor(Monitor* monitor) {
    FOR_EACH(Workspace*, workspace, getAllWorkspaces()) {
        if(getMonitor(workspace) == monitor)
            return workspace;
    }
    return NULL;
}

static const uint16_t* rootDim;
void setRootDims(const uint16_t* s) {
    rootDim = s;
}
uint16_t getRootWidth(void) {
    return rootDim ? rootDim[0] : 0;
}

uint16_t getRootHeight(void) {
    return rootDim ? rootDim[1] : 0;
}

void clearAllFakeMonitors(void) {
    FOR_EACH_R(Monitor*, m, getAllMonitors()) {
        if(m->fake)
            freeMonitor(m);
    }
}
