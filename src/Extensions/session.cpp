#include <limits.h>
#include <assert.h>
#include <string.h>

#include "../bindings.h"
#include "../devices.h"
#include "../layouts.h"
#include "../logger.h"
#include "../masters.h"
#include "../monitors.h"
#include "../user-events.h"
#include "../window-properties.h"
#include "../windows.h"
#include "../wmfunctions.h"
#include "../workspaces.h"
#include "../xsession.h"
#include "session.h"


static void loadSavedLayouts() {
    auto reply = getWindowProperty(root, WM_WORKSPACE_LAYOUT_NAMES, ewmh->UTF8_STRING);
    LOG(LOG_LEVEL_TRACE, "Loading active layouts\n");
    if(reply) {
        uint32_t len = xcb_get_property_value_length(reply.get());
        char* strings = (char*) xcb_get_property_value(reply.get());
        uint32_t index = 0, count = 0;
        while(index < len) {
            Layout* layout = findLayoutByName(std::string(&strings[index]));
            getWorkspace(count++)->setActiveLayout(layout);
            index += strlen(&strings[index]) + 1;
            if(count == getNumberOfWorkspaces())
                break;
        }
    }
}
static void loadSavedLayoutOffsets() {
    LOG(LOG_LEVEL_TRACE, "Loading Workspace layout offsets\n");
    auto reply = getWindowProperty(root, WM_WORKSPACE_LAYOUT_INDEXES, XCB_ATOM_CARDINAL);
    if(reply)
        for(uint32_t i = 0; i < xcb_get_property_value_length(reply.get()) / sizeof(int) && i < getNumberOfWorkspaces(); i++)
            getWorkspace(i)->setLayoutOffset(((int*)xcb_get_property_value(reply.get()))[i]);
}
static void loadSavedFakeMonitor() {
    LOG(LOG_LEVEL_TRACE, "Loading fake monitor mappings\n");
    auto reply = getWindowProperty(root, WM_FAKE_MONITORS, XCB_ATOM_CARDINAL);
    if(reply)
        for(uint32_t i = 0; i < xcb_get_property_value_length(reply.get()) / (sizeof(short) * 5) ; i++) {
            short* values = (short*) & (((char*)xcb_get_property_value(reply.get()))[i * sizeof(short) * 5]);
            MonitorID id = values[0];
            values++;
            Monitor* m = getAllMonitors().find(id);
            if(m)
                m->setBase(values);
            else
                getAllMonitors().add(new Monitor(id, values, 0, "", 1));
        }
}
static void loadSavedMonitorWorkspaceMapping() {
    LOG(LOG_LEVEL_TRACE, "Loading Workspace monitor mappings\n");
    auto reply = getWindowProperty(root, WM_WORKSPACE_MONITORS, XCB_ATOM_CARDINAL);
    if(reply) {
        for(uint32_t i = 0; i < xcb_get_property_value_length(reply.get()) / sizeof(int) && i < getNumberOfWorkspaces(); i++) {
            MonitorID id = ((MonitorID*)xcb_get_property_value(reply.get()))[i];
            if(!id)
                continue;
            Monitor* m = getAllMonitors().find(id);
            if(m) {
                Workspace* w = m->getWorkspace();
                if(w)
                    swapMonitors(i, w->getID());
                else
                    getWorkspace(i)->setMonitor(m);
            }
            else
                logger.info() << "Could not find monitor " << id << std::endl;
        }
    }
}
static void loadSavedMasterWindows() {
    LOG(LOG_LEVEL_TRACE, "Loading Master window stacks\n");
    auto reply = getWindowProperty(root, WM_MASTER_WINDOWS, XCB_ATOM_CARDINAL);
    if(reply) {
        Master* master = NULL;
        WindowID* wid = (WindowID*)xcb_get_property_value(reply.get());
        for(uint32_t i = 0; i < xcb_get_property_value_length(reply.get()) / sizeof(int); i++)
            if(wid[i] == 0) {
                master = getMasterByID(wid[++i]);
            }
            else if(master)
                master->onWindowFocus(wid[i]);
    }
}
static void loadSavedActiveMaster() {
    LOG(LOG_LEVEL_TRACE, "Loading active Master\n");
    auto reply = getWindowProperty(root, WM_ACTIVE_MASTER, XCB_ATOM_CARDINAL);
    if(reply)
        setActiveMasterByDeviceID(*(MasterID*)xcb_get_property_value(reply.get()));
}

void loadCustomState(void) {
    loadSavedLayouts();
    loadSavedLayoutOffsets();
    loadSavedFakeMonitor();
    loadSavedMonitorWorkspaceMapping();
    loadSavedMasterWindows();
    loadSavedActiveMaster();
    for(Master* master : getAllMasters()) {
        if(master->getFocusedWindow())
            setBorderColor(master->getFocusedWindow()->getID(), master->getFocusColor());
    }
}

static std::unordered_map<std::string, std::string> monitorWorkspaceMapping;

void saveMonitorWorkspaceMapping() {
    for(Monitor* monitor : getAllMonitors())
        if(monitor->getWorkspace())
            monitorWorkspaceMapping[monitor->getName()] = monitor->getWorkspace()->getName();
    StringJoiner joiner;
    for(auto element : monitorWorkspaceMapping) {
        joiner.add(element.first);
        joiner.add(element.second);
    }
    setWindowProperty(root, WM_WORKSPACE_MONITORS, ewmh->UTF8_STRING, joiner.getBuffer(), joiner.getSize());
}
void loadMonitorWorkspaceMapping() {
    auto reply = getWindowProperty(root, WM_WORKSPACE_MONITORS, ewmh->UTF8_STRING);
    LOG(LOG_LEVEL_TRACE, "Loading active layouts\n");
    if(reply) {
        uint32_t len = xcb_get_property_value_length(reply.get());
        char* strings = (char*) xcb_get_property_value(reply.get());
        uint32_t index = 0;
        while(index + 1 < len) {
            std::string monitorName = std::string(&strings[index]);
            index += strlen(&strings[index]) + 1;
            std::string workspaceName = std::string(&strings[index]);
            index += strlen(&strings[index]) + 1;
            Monitor* monitor = getMonitorByName(monitorName);
            Workspace* workspace = getWorkspaceByName(workspaceName);
            if(monitor && workspace)
                workspace->setMonitor(monitor);
        }
    }
}

void saveCustomState(void) {
    int layoutOffsets[getNumberOfWorkspaces()];
    int monitors[getNumberOfWorkspaces()];
    short fakeMonitors[getAllMonitors().size() * 5];
    WindowID masterWindows[(getAllWindows().size() + 2) * getAllMasters().size()];
    int numFakeMonitors = 0;
    int numMasterWindows = 0;
    for(int i = getAllMasters().size() - 1; i >= 0; i--) {
        Master* master = getAllMasters()[i];
        masterWindows[numMasterWindows++] = 0;
        masterWindows[numMasterWindows++] = master->getID();
        for(auto p = master->getWindowStack().rbegin(); p != master->getWindowStack().rend(); ++p)
            masterWindows[numMasterWindows++] = (*p)->getID();
    }
    for(Monitor* monitor : getAllMonitors())
        if(monitor->isFake()) {
            fakeMonitors[numFakeMonitors++] = monitor->getID();
            for(int n = 0; n < 4; n++)
                fakeMonitors[numFakeMonitors++] = monitor->getBase()[n];
        }
    StringJoiner joiner;
    for(WorkspaceID i = 0; i < getNumberOfWorkspaces(); i++) {
        Monitor* m = getWorkspace(i)->getMonitor();
        monitors[i] = m ? m->getID() : 0;
        layoutOffsets[i] = getWorkspace(i)->getLayoutOffset();
        Layout* layout = getWorkspace(i)->getActiveLayout();
        joiner.add(layout ? layout->getName() : "");
    }
    setWindowProperty(root, WM_WORKSPACE_LAYOUT_NAMES, ewmh->UTF8_STRING, joiner.getBuffer(), joiner.getSize());
    setWindowProperty(root, WM_WORKSPACE_LAYOUT_INDEXES, XCB_ATOM_CARDINAL, layoutOffsets, LEN(layoutOffsets));
    setWindowProperty(root, WM_WORKSPACE_MONITORS, XCB_ATOM_CARDINAL, monitors, LEN(monitors));
    setWindowProperty(root, WM_FAKE_MONITORS, XCB_ATOM_CARDINAL, fakeMonitors, numFakeMonitors);
    setWindowProperty(root, WM_MASTER_WINDOWS, XCB_ATOM_CARDINAL, masterWindows, numMasterWindows);
    setWindowProperty(root, WM_ACTIVE_MASTER, XCB_ATOM_CARDINAL, getActiveMaster()->getID());
    flush();
}
void addResumeCustomStateRules(AddFlag flag) {
    getEventRules(X_CONNECTION).add(DEFAULT_EVENT(loadCustomState), flag);
    getEventRules(MONITOR_DETECTED).add(DEFAULT_EVENT(loadMonitorWorkspaceMapping), flag);
    getBatchEventRules(MONITOR_WORKSPACE_CHANGE).add(DEFAULT_EVENT(saveMonitorWorkspaceMapping), flag);
    getBatchEventRules(DEVICE_EVENT).add(DEFAULT_EVENT(saveCustomState), flag);
    getBatchEventRules(TILE_WORKSPACE).add(DEFAULT_EVENT(saveCustomState), flag);
}

