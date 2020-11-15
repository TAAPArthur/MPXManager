#include "../boundfunction.h"
#include "../layouts.h"
#include "../util/logger.h"
#include "../monitors.h"
#include "../user-events.h"
#include "../window-masks.h"
#include "../xutil/window-properties.h"
#include "../windows.h"
#include "../wmfunctions.h"
#include "../xutil/xsession.h"
#include "containers.h"

static const char* CONTAINER_NAME_PREFIX = "CONTAINER: ";

xcb_atom_t MPX_CONTAINER_WORKSPACE;
static void initContainerAtoms() {
    CREATE_ATOM(MPX_CONTAINER_WORKSPACE);
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


WindowID createContainer(const char* name, Workspace* monitorWorkspace, Workspace* windowWorkspace) {
    if(!name)
        name = "";
    WindowID win;
    Rect base = (Rect) {0, 0, 1, 1};
    do {
        win = createWindow(root, XCB_WINDOW_CLASS_INPUT_OUTPUT, 0, NULL, base);
        if(findElement(getAllMonitors(), &win, sizeof(MonitorID))) {
            destroyWindow(win);
            WARN("Failed to create container because a monitor the generated id was already taken");
            continue;
        }
        break;
    } while(true);
    setWindowTitle(win, name);
    WindowInfo* winInfo = newWindowInfo(win, root);
    addMask(winInfo, INPUT_MASK | MAPPABLE_MASK|BELOW_MASK);
    setUserTime(win, 0);
    lowerWindow(win, 0);
    if(!registerWindowInfo(winInfo, NULL))
        return 0;
    Monitor* m = newMonitor(win, base, 0, name, 1);
    if(monitorWorkspace)
        assignWorkspace(m, monitorWorkspace);
    if(windowWorkspace)
        moveToWorkspace(winInfo, windowWorkspace->id);
    applyEventRules(SCREEN_CHANGE, NULL);
    return win;
}
WindowID createSimpleContainer() {
    return createContainer(CONTAINER_NAME_PREFIX, NULL, NULL);
}
void containWindow(WindowInfo*winInfo){
    WindowID container = createSimpleContainer();
    Monitor*m=getMonitorForContainer(container);
    assignEmptyWorkspace(m);
    swapWindows(container, winInfo->id);
    if(getWorkspaceOfMonitor(m))
        moveToWorkspace(winInfo, getWorkspaceOfMonitor(m)->id);
}

void containWindowAndActivate(WindowInfo*winInfo){
    containWindow(winInfo);
    activateWindow(winInfo);
}

Monitor* containWindows(Workspace* containedWorkspace, WindowFunctionArg arg, const char* name) {
    assert(containedWorkspace);
    Workspace* workspace = getActiveWorkspace();
    if(workspace == containedWorkspace)
        return NULL;
    Monitor* container = getMonitorForContainer(createContainer(name, containedWorkspace, NULL));
    if(!container)
        return NULL;
    for(int i = 0; i < getWorkspaceWindowStack(workspace)->size; i++) {
        WindowInfo* winInfo = getElement(getWorkspaceWindowStack(workspace), i);
        if(arg.func(winInfo, arg)) {
            moveToWorkspace(winInfo, containedWorkspace->id);
            i--;
        }
    }
    return container;
}

void releaseWindows(Monitor* container) {
    const WindowInfo* containerWindow = getWindowInfoForContainer(container->id);
    if(containerWindow) {
        Workspace* workspace = getWorkspaceOfMonitor(container);
        Workspace* workspaceDest = getWorkspaceOfWindow(containerWindow);
        if(workspace && workspaceDest && workspace != workspaceDest)
            while(getWorkspaceWindowStack(workspace)->size)
                moveToWorkspace(getHead(getWorkspaceWindowStack(workspace)), workspaceDest->id);
        INFO("Destroying container: %d", containerWindow->id);
        destroyWindow(containerWindow->id);
    }
}
void releaseAllWindows() {
    FOR_EACH(Monitor*, monitor, getAllMonitors()) {
        if(getWindowInfoForContainer(monitor->id))
            releaseWindows(monitor);
    }
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

static void saveContainers() {
    DEBUG("Saving custom State");
    MonitorID containers[getAllMonitors()->size * 2];
    int i = 0;
    FOR_EACH(Monitor*, m, getAllMonitors()) {
        WindowInfo* winInfo = getWindowInfoForContainer(m->id);
        if(winInfo) {
            containers[i++] = getWorkspaceOfMonitor(m) ? getWorkspaceOfMonitor(m)->id : NO_WORKSPACE;
            containers[i++] = getWorkspaceIndexOfWindow(winInfo);
        }
    }
    setWindowProperty(root, MPX_CONTAINER_WORKSPACE, XCB_ATOM_CARDINAL, containers, i);
}
void loadContainers() {
    xcb_get_property_reply_t* reply = getWindowProperty(root, MPX_CONTAINER_WORKSPACE, XCB_ATOM_CARDINAL);
    if(reply)
        for(uint32_t i = 0; i < xcb_get_property_value_length(reply) / sizeof(MonitorID) - 1 ; i += 2) {
            WorkspaceID containing = ((MonitorID*)xcb_get_property_value(reply))[i];
            WorkspaceID containedIn = ((MonitorID*)xcb_get_property_value(reply))[i + 1];
            Workspace* w = getWorkspace(containing);
            Monitor* m = getMonitor(w);
            if(m && m->fake) {
                INFO("Restoring container: %s", m->name);
                createContainer(m->name, w, getWorkspace(containedIn));
                freeMonitor(m);
            }
        }
    free(reply);
}
void addResumeContainerRules() {
    addEvent(X_CONNECTION, DEFAULT_EVENT(initContainerAtoms, HIGHEST_PRIORITY));
    addEvent(X_CONNECTION, DEFAULT_EVENT(loadContainers, LOW_PRIORITY));
    addEvent(XCB_MAP_NOTIFY, DEFAULT_EVENT(onContainerMapEvent));
    addEvent(XCB_UNMAP_NOTIFY, DEFAULT_EVENT(onContainerUnmapEvent));
    addEvent(UNREGISTER_WINDOW, DEFAULT_EVENT(onContainerUnregegister));
    addEvent(WINDOW_MOVE, DEFAULT_EVENT(updateContainerMonitorOnWindowMove));
    addBatchEvent(WORKSPACE_WINDOW_ADD, DEFAULT_EVENT(saveContainers, LOW_PRIORITY));
}
