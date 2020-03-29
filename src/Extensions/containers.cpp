#include "../monitors.h"
#include "../windows.h"
#include "../wmfunctions.h"
#include "../xsession.h"
#include "../state.h"
#include "../window-masks.h"
#include "../boundfunction.h"
#include "../user-events.h"
#include "../logger.h"
#include "containers.h"

static std::string CONTAINER_NAME = "CONTAINER";

struct Container : WindowInfo, Monitor {
    Container(WindowID id, WindowID parent = 0): WindowInfo(id, parent), Monitor(id, {0, 0, 1, 1}, 0, CONTAINER_NAME, 1) {}
    virtual void setGeometry(const RectWithBorder geo) {
        WindowInfo::setGeometry(geo);
        setBase(geo);
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
    if(!registerWindow(container))
        return 0;
    getAllMonitors().add(container);
    container->addMask(MAPPABLE_MASK);
    if(doNotAssignWorkspace) {
        container->assignWorkspace();
        container->moveToWorkspace(getActiveWorkspaceIndex());
    }
    return win;
}
WindowID createContainer() {
    return createContainer(1);
}

static void saveContainers() {
    DEBUG("Saving custom State");
    MonitorID containers[getNumberOfWorkspaces()];
    for(WorkspaceID i = 0; i < getNumberOfWorkspaces(); i++) {
        Monitor* m = getWorkspace(i)->getMonitor();
        containers[i] = getWindowInfoForContainer(*m) ? m->getID() : 0;
    }
    setWindowProperty(root, MPX_WM_WORKSPACE_MONITORS, XCB_ATOM_CARDINAL, containers, LEN(containers));
}
static void loadContainerMonitors() {
    for(int i = 0; i < getAllMonitors().size(); i++) {
        Monitor* m = getAllMonitors()[i];
        if(m->isFake() && m->getName() == CONTAINER_NAME) {
            createContainer(0);
            getAllMonitors().swap(i, getAllMonitors().size() - 1);
            getAllMonitors().pop();
            getAllMonitors()[i]->assignWorkspace(m->getWorkspace());
            delete m;
        }
    }
}
static void loadContainerWindows() {
    TRACE("Loading fake monitor mappings");
    auto reply = getWindowProperty(root, MPX_WM_FAKE_MONITORS, XCB_ATOM_CARDINAL);
    if(reply)
        for(uint32_t i = 0; i < xcb_get_property_value_length(reply.get()) / sizeof(short) ; i++) {
            short* values = (short*) & (((char*)xcb_get_property_value(reply.get()))[i * sizeof(short)]);
            MonitorID id = values[0];
            WindowInfo* winInfo = getWindowInfoForContainer(id);
            if(winInfo)
                winInfo->moveToWorkspace(i);
        }
}
static void loadContainers() {
    loadContainerMonitors();
    loadContainerWindows();
}
void addResumeContainerRules(bool remove) {
    getEventRules(X_CONNECTION).add(DEFAULT_EVENT(loadContainers, LOW_PRIORITY), remove);
    getBatchEventRules(WINDOW_WORKSPACE_CHANGE).add(DEFAULT_EVENT(saveContainers, LOW_PRIORITY), remove);
}
