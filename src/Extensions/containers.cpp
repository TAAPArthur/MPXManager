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


struct Container : WindowInfo, Monitor{
    Container(WindowID id, WindowID parent = 0): WindowInfo(id, parent), Monitor(id, {0,0,1,1}) {}
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

WindowID createContainer() {
    WindowID win = createWindow(root, XCB_WINDOW_CLASS_INPUT_OUTPUT, XCB_CW_BACK_PIXEL, &screen->black_pixel);
    Container* container = new Container(win, root);
    if(!registerWindow(container))
        return 0;
    getAllMonitors().add(container);
    container->addMask(MAPPABLE_MASK);
    container->moveToWorkspace(getActiveWorkspaceIndex());
    container->assignWorkspace();
    return win;
}
