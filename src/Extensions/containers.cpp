#include "../boundfunction.h"
#include "../logger.h"
#include "../monitors.h"
#include "../user-events.h"
#include "../window-masks.h"
#include "../window-properties.h"
#include "../windows.h"
#include "../wmfunctions.h"
#include "../xsession.h"
#include "containers.h"

static std::string CONTAINER_NAME = "CONTAINER";

xcb_atom_t MPX_CONTAINER_WORKSPACE;
static void initContainerAtoms() {
    CREATE_ATOM(MPX_CONTAINER_WORKSPACE);
}

struct Container : WindowInfo, Monitor {
    Container(WindowID id, WindowID parent = 0): WindowInfo(id, parent), Monitor(id, {0, 0, 1, 1}, 0, CONTAINER_NAME, 1) {}
    virtual void setGeometry(const RectWithBorder geo) {
        WindowInfo::setGeometry(geo);
        setBase(geo);
        markState();
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


WindowID createContainer(bool doNotAssignWorkspace) {
    WindowID win = createWindow(root, XCB_WINDOW_CLASS_INPUT_OUTPUT, XCB_CW_BACK_PIXEL, &screen->black_pixel);
    Container* container = new Container(win, root);
    setWindowTitle(win, CONTAINER_NAME);
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
    WindowID win = createContainer(1);
    auto index = getAllMonitors().indexOf(m);
    getAllMonitors().swap(index, getAllMonitors().size() - 1);
    getAllMonitors().pop();
    getAllMonitors()[index]->assignWorkspace(m->getWorkspace());
    delete m;
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
void addResumeContainerRules(bool remove) {
    getEventRules(X_CONNECTION).add(DEFAULT_EVENT(initContainerAtoms, HIGH_PRIORITY), remove);
    getEventRules(X_CONNECTION).add(DEFAULT_EVENT(loadContainers, LOW_PRIORITY), remove);
    getBatchEventRules(WINDOW_WORKSPACE_CHANGE).add(DEFAULT_EVENT(saveContainers, LOW_PRIORITY), remove);
}
