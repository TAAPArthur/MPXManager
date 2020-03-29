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

WindowID createContainer(bool doNotAssignWorkspace) {
    WindowID win = createWindow(root, XCB_WINDOW_CLASS_INPUT_OUTPUT, XCB_CW_BACK_PIXEL, &screen->black_pixel);
    Container* container = new Container(win, root);
    if(!registerWindow(container))
        return 0;
    getAllMonitors().add(container);
    container->addMask(MAPPABLE_MASK);
    container->moveToWorkspace(getActiveWorkspaceIndex());
    if(doNotAssignWorkspace)
        container->assignWorkspace();
    return win;
}
WindowID createContainer() {
    return createContainer(1);
}
static void loadContainers() {
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
void addResumeContainerRules(bool remove) {
    getEventRules(X_CONNECTION).add(DEFAULT_EVENT(loadContainers, LOW_PRIORITY), remove);
}
