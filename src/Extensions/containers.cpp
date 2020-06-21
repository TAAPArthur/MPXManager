#include "../boundfunction.h"
#include "../layouts.h"
#include "../logger.h"
#include "../monitors.h"
#include "../user-events.h"
#include "../window-masks.h"
#include "../window-properties.h"
#include "../windows.h"
#include "../wmfunctions.h"
#include "../xsession.h"
#include "containers.h"

static std::string CONTAINER_NAME_PREFIX = "CONTAINER: ";

xcb_atom_t MPX_CONTAINER_WORKSPACE;
static void initContainerAtoms() {
    CREATE_ATOM(MPX_CONTAINER_WORKSPACE);
}

struct Container : WindowInfo, Monitor {
    Container(WindowID id, WindowID parent = 0, std::string name = ""):
        WindowInfo(id, parent),
        Monitor(id, {0, 0, 1, 1}, 0, name, 1, id) {}
    virtual void setGeometry(const RectWithBorder geo) {
        WindowInfo::setGeometry(geo);
        setBase(geo);
    }
    virtual void addMask(WindowMask mask) override {
        bool change = !hasMask(MAPPED_MASK) && (mask & MAPPED_MASK);
        WindowInfo::addMask(mask);
        if(change)
            applyEventRules(MONITOR_WORKSPACE_CHANGE, {.workspace = Monitor::getWorkspace(), .monitor = this});
    }
    /**
     * Removes the states give by mask from the window
     * @param mask
     */
    virtual void removeMask(WindowMask mask) override {
        bool change = hasMask(MAPPED_MASK) && (mask & MAPPED_MASK);
        WindowInfo::removeMask(mask);
        if(change) {
            applyEventRules(MONITOR_WORKSPACE_CHANGE, {.workspace = Monitor::getWorkspace(), .monitor = this});
        }
    }
    bool isActive() const override {
        return hasMask(MAPPED_MASK) && Monitor::isActive();
    }

    ~Container() {
        getAllMonitors().removeElement(this);
        getAllWindows().removeElement(this);
    }
};


WindowInfo* getWindowInfoForContainer(MonitorID mon) {
    return getWindowInfo(mon);
}
Monitor* getMonitorForContainer(WindowID win) {
    return getAllMonitors().find(win);
}


WindowID createContainer(bool doNotAssignWorkspace, const char* name = NULL) {
    if(name && getMonitorByName(name)) {
        INFO("Container already exists");
        return 0;
    }
    if(!name)
        name = "";
    WindowID win;
    do {
        win = createWindow(root, XCB_WINDOW_CLASS_INPUT_OUTPUT);
        if(getAllMonitors().find(win)) {
            destroyWindow(win);
            WARN("Failed to create container because a monitor the generated id was already taken");
            continue;
        }
        break;
    } while(true);
    Container* container = new Container(win, root, name);
    setWindowTitle(win, CONTAINER_NAME_PREFIX + name);
    container->addMask(INPUT_MASK | MAPPABLE_MASK);
    if(!registerWindow(container))
        return 0;
    getAllMonitors().add(container);
    if(!doNotAssignWorkspace) {
        container->assignWorkspace();
        if(!((WindowInfo*)container)->getWorkspace()) {
            container->moveToWorkspace(getActiveWorkspaceIndex());
        }
    }
    return win;
}
WindowID createContainer() {
    return createContainer(0);
}

Monitor* containWindows(Workspace* containedWorkspace, const BoundFunction& func, const char* name) {
    assert(containedWorkspace);
    Workspace* workspace = getActiveWorkspace();
    if(workspace == containedWorkspace)
        return NULL;
    Monitor* container = getMonitorForContainer(createContainer(1, name));
    if(!container)
        container = getMonitorByName(name);
    if(!container)
        return NULL;
    if(!container->getWorkspace())
        container->assignWorkspace(containedWorkspace);
    for(int i = 0; i < workspace->getWindowStack().size(); i++) {
        auto winInfo = workspace->getWindowStack()[i];
        if(func({.winInfo = winInfo})) {
            winInfo->moveToWorkspace(*containedWorkspace);
            i--;
        }
    }
    return container;
}
void releaseWindows(Monitor* container) {
    const WindowInfo* containerWindow = getWindowInfoForContainer(container->getID());
    if(containerWindow) {
        if(container->getWorkspace() != containerWindow->getWorkspace())
            while(container->getWorkspace()->getWindowStack().size())
                container->getWorkspace()->getWindowStack()[0]->moveToWorkspace(containerWindow->getWorkspace()->getID());
        if(getActiveWorkspace() == container->getWorkspace())
            getActiveMaster()->setWorkspaceIndex(containerWindow->getWorkspaceIndex());
        INFO("Destroying container: " << *containerWindow);
        destroyWindow(*containerWindow);
    }
}
void releaseAllWindows() {
    for(Monitor* monitor : getAllMonitors()) {
        if(getWindowInfoForContainer(*monitor))
            releaseWindows(monitor);
    }
}
void toggleContainer(WindowInfo* winInfo) {
    Monitor* container = getMonitorForContainer(winInfo->getID());
    Workspace* workspace = NULL;
    if(container) {
        DEBUG("toggleContainer: given container " << *container);
        workspace = container->getWorkspace();
    }
    else if(winInfo->getMonitor()) {
        WindowInfo* containerWindow = getWindowInfoForContainer(*winInfo->getMonitor());
        if(containerWindow) {
            DEBUG("toggleContainer: given window in container " << *winInfo);
            workspace = containerWindow->getWorkspace();
        }
    }
    if(workspace) {
        activateWorkspace(*workspace);
    }
}

static void saveContainers() {
    DEBUG("Saving custom State");
    MonitorID containers[getAllMonitors().size() * 2];
    int i = 0;
    for(Monitor* m : getAllMonitors()) {
        WindowInfo* winInfo = getWindowInfoForContainer(*m);
        if(winInfo) {
            containers[i++] = m->getID();
            containers[i++] = winInfo->getWorkspaceIndex();
        }
    }
    setWindowProperty(root, MPX_CONTAINER_WORKSPACE, XCB_ATOM_CARDINAL, containers, i);
}
static WindowInfo* loadContainerMonitors(Monitor* m) {
    INFO("Restoring container: " << *m);
    Workspace* workspace = m->getWorkspace();
    auto name =  m->getName();
    delete getAllMonitors().removeElement(m);
    WindowID win = createContainer(1, name.c_str());
    INFO("Created container: " << win);
    if(workspace)
        getMonitorForContainer(win)->assignWorkspace(workspace);
    return getWindowInfo(win);
}
void loadContainers() {
    auto reply = getWindowProperty(root, MPX_CONTAINER_WORKSPACE, XCB_ATOM_CARDINAL);
    if(reply)
        for(uint32_t i = 0; i < xcb_get_property_value_length(reply.get()) / sizeof(MonitorID) - 1 ; i += 2) {
            MonitorID id = ((MonitorID*)xcb_get_property_value(reply.get()))[i];
            WorkspaceID index = ((MonitorID*)xcb_get_property_value(reply.get()))[i + 1];
            Monitor* m = getAllMonitors().find(id);
            if(m && m->isFake())
                loadContainerMonitors(m)->moveToWorkspace(index);
        }
}
void retileOnContainerMove(WindowInfo* winInfo) {
    Monitor* containerMonitor = getMonitorForContainer(*winInfo);
    if(containerMonitor && containerMonitor->getWorkspace()) {
        tileWorkspace(containerMonitor->getWorkspace());
    }
}
void addResumeContainerRules(bool remove) {
    getEventRules(X_CONNECTION).add(DEFAULT_EVENT(initContainerAtoms, HIGHEST_PRIORITY), remove);
    getEventRules(X_CONNECTION).add(DEFAULT_EVENT(loadContainers, LOW_PRIORITY), remove);
    getEventRules(WINDOW_MOVE).add(DEFAULT_EVENT(retileOnContainerMove), remove);
    getBatchEventRules(WINDOW_WORKSPACE_CHANGE).add(DEFAULT_EVENT(saveContainers, LOW_PRIORITY), remove);
}
