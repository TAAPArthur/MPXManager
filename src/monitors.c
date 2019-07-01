/**
 * @file monitors.c
 * @copybrief monitors.h
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifndef NO_XRANDR
    #include <xcb/randr.h>
#endif
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>


#include "globals.h"
#include "logger.h"
#include "monitors.h"
#include "spawn.h"
#include "workspaces.h"
#include "xsession.h"

/**
 * List of docks.
 * Docks are a special kind of window that isn't managed by us
 */
static ArrayList docks;


///list of all monitors
static ArrayList monitors;

MonitorDuplicationPolicy MONITOR_DUPLICATION_POLICY = SAME_DIMS;
MonitorDuplicationResolution MONITOR_DUPLICATION_RESOLUTION = TAKE_PRIMARY | TAKE_LARGER;

/**
 * Checks to if two lines intersect
 * (either both vertical or both horizontal
 * @param P1 starting point 1
 * @param D1 displacement 1
 * @param P2 staring point 2
 * @param D2 displacement 2
 * @return 1 iff the line intersect
 */
static int intersects1D(int P1, int D1, int P2, int D2){
    return P1 < P2 + D2 && P1 + D1 > P2;
}

int intersects(Rect arg1, Rect arg2){
    return (intersects1D(arg1.x, arg1.width, arg2.x, arg2.width) &&
            intersects1D(arg1.y, arg1.height, arg2.y, arg2.height));
}
int isLarger(Rect arg1, Rect arg2){
    return arg1.width * arg1.height > arg2.width * arg2.height;
}
int contains(Rect arg1, Rect arg2){
    return (arg1.x < arg2.x && arg2.x + arg2.width < arg1.x + arg1.width &&
            arg1.y < arg2.y && arg2.y + arg2.height < arg1.y + arg1.height);
}

void setDockArea(WindowInfo* info, int numberofProperties, int* properties){
    memset(info->properties, 0, sizeof(info->properties));
    memcpy(info->properties, properties, numberofProperties * sizeof(int));
}
int loadDockProperties(WindowInfo* info){
    LOG(LOG_LEVEL_TRACE, "Reloading dock properties\n");
    xcb_window_t win = info->id;
    xcb_ewmh_wm_strut_partial_t strut;
    if(xcb_ewmh_get_wm_strut_partial_reply(ewmh,
                                           xcb_ewmh_get_wm_strut_partial(ewmh, win), &strut, NULL)){
        setDockArea(info, sizeof(xcb_ewmh_wm_strut_partial_t) / sizeof(int), (int*)&strut);
    }
    else if(xcb_ewmh_get_wm_strut_reply(ewmh,
                                        xcb_ewmh_get_wm_strut(ewmh, win),
                                        (xcb_ewmh_get_extents_reply_t*) &strut, NULL))
        setDockArea(info, sizeof(xcb_ewmh_get_extents_reply_t) / sizeof(int), (int*)&strut);
    else {
        LOG(LOG_LEVEL_TRACE, "could not read struct data\n");
        setDockArea(info, 0, NULL);
        return 0;
    }
    if(info->dock)
        resizeAllMonitorsToAvoidAllStructs();
    return 1;
}
int getRootWidth(void){
    return screen->width_in_pixels;
}
int getRootHeight(void){
    return screen->height_in_pixels;
}
static Rect ROOT_BOUNDS;
const short* getRootBounds(){
    ROOT_BOUNDS = (Rect){0, 0, getRootWidth(), getRootHeight()};
    return &ROOT_BOUNDS.x;
}


static void resizeMonitorToAvoidAllStructs(Monitor* m){
    resetMonitor(m);
    FOR_EACH(WindowInfo*, dock, getAllDocks()) resizeMonitorToAvoidStruct(m, dock);
}
void resizeAllMonitorsToAvoidAllStructs(void){
    FOR_EACH(Monitor*, mon, getAllMonitors()) resizeMonitorToAvoidAllStructs(mon);
}
void resizeAllMonitorsToAvoidStruct(WindowInfo* winInfo){
    FOR_EACH(Monitor*, mon, getAllMonitors()) resizeMonitorToAvoidStruct(mon, winInfo);
}

int resizeMonitorToAvoidStruct(Monitor* m, WindowInfo* winInfo){
    assert(m);
    assert(winInfo);
    int changed = 0;
    for(int i = 0; i < 4; i++){
        int dim = winInfo->properties[i];
        if(dim == 0)
            continue;
        int start = winInfo->properties[i * 2 + 4];
        int end = winInfo->properties[i * 2 + 4 + 1];
        assert(start <= end);
        int fromPositiveSide = i == DOCK_LEFT || i == DOCK_TOP;
        int offset = i == DOCK_LEFT || i == DOCK_RIGHT ? 0 : 1;
        if(winInfo->onlyOnPrimary){
            if(end == 0)
                end = (&m->base.x)[offset];
            if(!m->primary)
                continue;
        }
        if(end == 0)
            end = getRootBounds()[2 + ((offset + 1) % 2)];
        short int values[] = {0, 0, 0, 0};
        values[offset] = fromPositiveSide ? 0 :
                         (winInfo->onlyOnPrimary ?
                          (&m->base.width)[offset] : getRootBounds()[2 + offset])
                         - dim;
        values[(offset + 1) % 2] = start;
        values[offset + 2] = dim;
        values[(offset + 1) % 2 + 2] = end - start;
        if(!intersects(m->view, *(Rect*)values))
            continue;
        int intersectionWidth = fromPositiveSide ?
                                dim - (&m->view.x)[offset] :
                                (&m->view.x)[offset] + (&m->view.width)[offset] - values[offset];
        assert(intersectionWidth > 0);
        (&m->view.width)[offset] -= intersectionWidth;
        if(fromPositiveSide)
            (&m->view.x)[offset] = dim;
        changed++;
    }
    return changed;
}
void clearFakeMonitors(void){
#ifdef NO_XRANDR
    detectMonitors();
#else
    waitForChild(spawn("xsane-xrandr clear &>/dev/null"));
#endif
}
void createMonitor(Rect bounds){
#ifdef NO_XRANDR
    static int fakeMonitorIDCounter;
    if(!fakeMonitorIDCounter)fakeMonitorIDCounter = 2;
    updateMonitor(fakeMonitorIDCounter++, bounds, 1);
#else
    char buffer[255];
    sprintf(buffer, "xsane-xrandr add-monitor %d %d %d %d  &>/dev/null", bounds.x, bounds.y, bounds.width, bounds.height);
    waitForChild(spawn(buffer));
#endif
}
void removeDuplicateMonitors(void){
    if(!MONITOR_DUPLICATION_POLICY || !MONITOR_DUPLICATION_RESOLUTION)
        return;
    for(int i = getSize(getAllMonitors()) - 1; i >= 0; i--){
        Monitor* m1 = getElement(getAllMonitors(), i);
        for(int n = i + 1; n < getSize(getAllMonitors()); n++){
            Monitor* m2 = getElement(getAllMonitors(), n);
            int dup = 0;
            int sameBounds = (memcmp(&m1->base, &m2->base, sizeof(Rect)) == 0);
            if(sameBounds)
                dup = MONITOR_DUPLICATION_POLICY & SAME_DIMS ? 1 : 0;
            else if(MONITOR_DUPLICATION_POLICY & INTERSECTS)
                dup = intersects(m1->base, m2->base);
            if(MONITOR_DUPLICATION_POLICY & CONTAINS)
                dup = contains(m1->base, m2->base) || contains(m2->base, m1->base);
            Monitor* monitorToRemove = NULL;
            if(!dup)
                continue;
            LOG(LOG_LEVEL_DEBUG, "Monitors %u and %u are duplicates\n", m1->id, m2->id);
            if(MONITOR_DUPLICATION_RESOLUTION & TAKE_SMALLER)
                monitorToRemove = isLarger(m1->base, m2->base) ? m2 : m1;
            if(MONITOR_DUPLICATION_RESOLUTION & TAKE_LARGER)
                monitorToRemove = isLarger(m1->base, m2->base) ? m1 : m2;
            if(MONITOR_DUPLICATION_RESOLUTION & TAKE_PRIMARY)
                monitorToRemove = isPrimary(m1) ? m2 : isPrimary(m2) ? m1 : monitorToRemove;;
            if(monitorToRemove){
                LOG(LOG_LEVEL_DEBUG, "removing monitor %u because it is a duplicates of %u", monitorToRemove->id,
                    ((Monitor*)((long)monitorToRemove ^ (long)m1 ^ (long)m2))->id);
                removeMonitor(monitorToRemove->id);
            }
        }
    }
}

static void assignWorkspace(Monitor* m, Workspace* workspace){
    if(!workspace)
        if(!isWorkspaceVisible(getActiveWorkspaceIndex()))
            workspace = getActiveWorkspace();
        else {
            workspace = getNextWorkspace(1, HIDDEN | NON_EMPTY);
            if(!workspace){
                workspace = getNextWorkspace(1, HIDDEN);
            }
        }
    if(workspace)
        workspace->monitor = m;
}
void assignUnusedMonitorsToWorkspaces(void){
    FOR_EACH_REVERSED(Monitor*, m, getAllMonitors()){
        if(!getWorkspaceFromMonitor(m))
            assignWorkspace(m, NULL);
    }
}
void detectMonitors(void){
#ifdef NO_XRANDR
    updateMonitor(1, *(Rect*)getRootBounds(), 0);
    removeDuplicateMonitors();
#else
    LOG(LOG_LEVEL_DEBUG, "refreshing monitors\n");
    xcb_randr_get_monitors_cookie_t cookie = xcb_randr_get_monitors(dis, root, 1);
    xcb_randr_get_monitors_reply_t* monitors = xcb_randr_get_monitors_reply(dis, cookie, NULL);
    assert(monitors);
    xcb_randr_monitor_info_iterator_t iter = xcb_randr_get_monitors_monitors_iterator(monitors);
    ArrayList monitorNames = {0};
    while(iter.rem){
        xcb_randr_monitor_info_t* monitorInfo = iter.data;
        updateMonitor(monitorInfo->name,  *(Rect*)&monitorInfo->x, 0);
        Monitor* m = find(getAllMonitors(), &monitorInfo->name, sizeof(MonitorID));
        assert(m);
        if(m->name)
            free(m->name);
        m->name = NULL;
        getAtomValue(m->id, &m->name);
        setPrimary(m, monitorInfo->primary);
        addToList(&monitorNames, (void*)(long)monitorInfo->name);
        xcb_randr_monitor_info_next(&iter);
    }
    free(monitors);
    FOR_EACH_REVERSED(Monitor*, m, getAllMonitors()){
        int i;
        for(i = getSize(&monitorNames) - 1; i >= 0; i--)
            if(m->id == (MonitorID)(long)getElement(&monitorNames, i))
                break;
        if(i == -1)
            removeMonitor(m->id);
    }
    clearList(&monitorNames);
#endif
    removeDuplicateMonitors();
    assignUnusedMonitorsToWorkspaces();
    assert(getSize(getAllMonitors()) > 0);
}
int removeMonitor(MonitorID id){
    int index = indexOf(getAllMonitors(), &id, sizeof(id));
    if(index == -1)
        return 0;
    Monitor* m = getElement(getAllMonitors(), index);
    Workspace* w = getWorkspaceFromMonitor(m);
    LOG(LOG_LEVEL_DEBUG, "removing monitor %d\n", id);
    dumpMonitorInfo(m);
    removeFromList(getAllMonitors(), index);
    if(w){
        w->monitor = NULL;
        FOR_EACH_REVERSED(Monitor*, otherMonitor, getAllMonitors()){
            if(!getWorkspaceFromMonitor(otherMonitor) && otherMonitor->base.x == m->base.x && otherMonitor->base.y == m->base.y){
                LOG(LOG_LEVEL_DEBUG, "giving workspace to monitor %d\n", otherMonitor->id);
                assignWorkspace(otherMonitor, w);
                break;
            }
        }
    }
    if(m->name)
        free(m->name);
    free(m);
    return 1;
}

void setPrimary(Monitor* monitor, bool primary){
    if(monitor){
        monitor->primary = primary;
    }
}
bool isPrimary(Monitor* monitor){
    return monitor->primary;
}
bool updateMonitor(MonitorID id, Rect geometry, bool autoAssignWorkspace){
    Monitor* m = find(getAllMonitors(), &id, sizeof(MonitorID));
    int newMonitor = !m;
    if(!m){
        m = calloc(1, sizeof(Monitor));
        m->id = id;
        addToList(getAllMonitors(), m);
        if(autoAssignWorkspace)
            assignWorkspace(m, NULL);
    }
    m->base = geometry;
    resizeMonitorToAvoidAllStructs(m);
    return newMonitor;
}

void resetMonitor(Monitor* m){
    assert(m);
    memcpy(&m->view, &m->base, sizeof(short) * 4);
}
ArrayList* getAllDocks(){
    return &docks;
}
char* getNameOfMonitor(Monitor* m){
    return m->name;
}
ArrayList* getAllMonitors(void){
    return &monitors;
}
