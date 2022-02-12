#include "../boundfunction.h"
#include "../layouts.h"
#include "../monitors.h"
#include "../user-events.h"
#include "../util/logger.h"
#include "../window-masks.h"
#include "../windows.h"
#include "../wmfunctions.h"
#include "../xutil/window-properties.h"
#include "../xutil/xsession.h"
#include "containers.h"

static xcb_atom_t MPX_CONTAINER = 0;
static void initContainerAtoms() {
    CREATE_ATOM(MPX_CONTAINER);
}


WindowInfo* getWindowInfoForContainer(MonitorID mon) {
    return getWindowInfo(mon);
}

Monitor* getMonitorForContainer(WindowID win) {
    return findElement(getAllMonitors(), &win, sizeof(MonitorID));
}

void updateContainerMonitorOnWindowMove(WindowInfo* winInfo) {
    Monitor* m = getMonitorForContainer(winInfo->id);
    if(m) {
        setBase(m, winInfo->geometry);
        applyEventRules(SCREEN_CHANGE, NULL);
    }
}

void onContainerMapEvent(xcb_map_notify_event_t* event) {
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(!winInfo)
        return;
    Monitor* m = getMonitorForContainer(winInfo->id);
    if(m) {
        m->inactive = 0;
        applyEventRules(SCREEN_CHANGE, NULL);
    }
}

void onContainerUnmapEvent(xcb_unmap_notify_event_t* event) {
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(!winInfo)
        return;
    Monitor* m = getMonitorForContainer(winInfo->id);
    if(m) {
        m->inactive = 1;
        applyEventRules(SCREEN_CHANGE, NULL);
    }
}

void onContainerUnregegister(WindowInfo* winInfo) {
    Monitor* m = getMonitorForContainer(winInfo->id);
    if(m) {
        freeMonitor(m);
        applyEventRules(SCREEN_CHANGE, NULL);
    }
}

bool createContainerInWorkspace(WindowInfo*winInfo, Workspace* monitorWorkspace) {
    char buffer[16];
    sprintf(buffer, "%d", winInfo->id);
    bool allocatedMonitor = 0;
    Monitor* m = getMonitorForContainer(winInfo->id);
    if(!m) {
        m = newMonitor(winInfo->id, winInfo->geometry, buffer, 1);
        allocatedMonitor = 1;
    }
    if(monitorWorkspace)
        assignWorkspace(m, monitorWorkspace);

    if(MPX_CONTAINER)
        setWindowPropertyInt(root, MPX_CONTAINER, XCB_ATOM_CARDINAL, m->id);
    applyEventRules(SCREEN_CHANGE, NULL);
    return allocatedMonitor;

}

void toggleContainer(WindowInfo* winInfo) {
    Monitor* m = getMonitorForContainer(winInfo->id);
    if(m) {
        INFO("Activating container");
        activateWorkspace(getWorkspaceOfMonitor(m)->id);
    }
    else {
        INFO("Activating Window container");
        Workspace* w = getWorkspaceOfWindow(winInfo);
        WindowInfo* container = w ? getMonitor(w) ? getWindowInfoForContainer(getMonitor(w)->id) : NULL : NULL;
        if(container)
            activateWorkspace(getWorkspaceOfWindow(container)->id);
    }
}

void addContainerRules() {
    addEvent(X_CONNECTION, DEFAULT_EVENT(initContainerAtoms, HIGHEST_PRIORITY));
    addEvent(XCB_MAP_NOTIFY, DEFAULT_EVENT(onContainerMapEvent));
    addEvent(XCB_UNMAP_NOTIFY, DEFAULT_EVENT(onContainerUnmapEvent));
    addEvent(UNREGISTER_WINDOW, DEFAULT_EVENT(onContainerUnregegister));
    addEvent(WINDOW_MOVE, DEFAULT_EVENT(updateContainerMonitorOnWindowMove));
}
