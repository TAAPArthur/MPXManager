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
    auto reply = getWindowProperty(root, MPX_WM_WORKSPACE_LAYOUT_NAMES, ewmh->UTF8_STRING);
    TRACE("Loading active layouts");
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
    TRACE("Loading Workspace layout offsets");
    auto reply = getWindowProperty(root, MPX_WM_WORKSPACE_LAYOUT_INDEXES, XCB_ATOM_CARDINAL);
    if(reply)
        for(uint32_t i = 0; i < xcb_get_property_value_length(reply.get()) / sizeof(int) && i < getNumberOfWorkspaces(); i++)
            getWorkspace(i)->setLayoutOffset(((int*)xcb_get_property_value(reply.get()))[i]);
}
static void loadSavedFakeMonitor() {
    TRACE("Loading fake monitor mappings");
    auto reply = getWindowProperty(root, MPX_WM_FAKE_MONITORS, XCB_ATOM_CARDINAL);
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
    TRACE("Loading Workspace monitor mappings");
    auto reply = getWindowProperty(root, MPX_WM_WORKSPACE_MONITORS, XCB_ATOM_CARDINAL);
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
                INFO("Could not find monitor " << id);
        }
    }
}
static void loadSavedMasterWindows() {
    TRACE("Loading Master window stacks");
    auto reply = getWindowProperty(root, MPX_WM_MASTER_WINDOWS, XCB_ATOM_CARDINAL);
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
    TRACE("Loading active Master");
    auto reply = getWindowProperty(root, MPX_WM_ACTIVE_MASTER, XCB_ATOM_CARDINAL);
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
    for(WindowInfo* winInfo : getAllWindows())
        winInfo->addMask(~EXTERNAL_MASKS & getWindowPropertyValue(winInfo->getID(), MPX_WM_MASKS, XCB_ATOM_CARDINAL));
}

static std::unordered_map<std::string, std::string> monitorWorkspaceMapping;
static std::unordered_map<uint64_t, WorkspaceID> boundsToWorkspaceMapping;

void saveMonitorWorkspaceMapping() {
    for(Monitor* monitor : getAllMonitors())
        if(monitor->getWorkspace()) {
            monitorWorkspaceMapping[monitor->getName()] = monitor->getWorkspace()->getName();
            boundsToWorkspaceMapping[monitor->getBase()] = monitor->getWorkspace()->getID();
        }
    int i = 0;
    short bounds[boundsToWorkspaceMapping.size() * 5];
    for(auto element : boundsToWorkspaceMapping) {
        const Rect rect = (const short*)&element.first;
        bounds[i++] = element.second;
        for(int n = 0; n < 4; n++)
            bounds[i++] = rect[n];
    }
    StringJoiner joiner;
    for(auto element : monitorWorkspaceMapping) {
        joiner.add(element.first);
        joiner.add(element.second);
    }
    setWindowProperty(root, MPX_WM_WORKSPACE_MONITORS, ewmh->UTF8_STRING, joiner.getBuffer(), joiner.getSize());
    setWindowProperty(root, MPX_WM_WORKSPACE_MONITORS_ALT, XCB_ATOM_CARDINAL, bounds, i);
}
void loadMonitorWorkspaceMapping() {
    auto reply = getWindowProperty(root, MPX_WM_WORKSPACE_MONITORS, ewmh->UTF8_STRING);
    TRACE("Loading monitor workspace mappings");
    int numberRestored = 0;
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
            if(monitor && workspace) {
                INFO("Restoring " << monitor->getName() << " " << workspace->getID());
                workspace->setMonitor(monitor);
                numberRestored++;
            }
            monitorWorkspaceMapping[monitorName] = workspaceName;
        }
    }
    if(numberRestored != getAllMonitors().size()) {
        DEBUG("Couldn't fully restore monitors based on name; trying remaining with position");
        reply = getWindowProperty(root, MPX_WM_WORKSPACE_MONITORS_ALT, XCB_ATOM_CARDINAL);
        if(reply) {
            uint32_t len = xcb_get_property_value_length(reply.get()) / sizeof(short);
            auto data = (short*) xcb_get_property_value(reply.get());
            for(int i = 0; i + 4 < len; i += 5) {
                WorkspaceID id = data[i];
                Rect bounds = &data[i + 1];
                boundsToWorkspaceMapping[bounds] = id;
                Workspace* workspace = getWorkspace(id);
                if(workspace && !workspace->isVisible())
                    for(Monitor* m : getAllMonitors())
                        if(!m->getWorkspace() && m->getBase() == bounds) {
                            INFO("Restoring " << m->getName() << " " << workspace->getID() << " based on position");
                            workspace->setMonitor(m);
                            break;
                        }
            }
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
    setWindowProperty(root, MPX_WM_WORKSPACE_LAYOUT_NAMES, ewmh->UTF8_STRING, joiner.getBuffer(), joiner.getSize());
    setWindowProperty(root, MPX_WM_WORKSPACE_LAYOUT_INDEXES, XCB_ATOM_CARDINAL, layoutOffsets, LEN(layoutOffsets));
    setWindowProperty(root, MPX_WM_WORKSPACE_MONITORS, XCB_ATOM_CARDINAL, monitors, LEN(monitors));
    setWindowProperty(root, MPX_WM_FAKE_MONITORS, XCB_ATOM_CARDINAL, fakeMonitors, numFakeMonitors);
    setWindowProperty(root, MPX_WM_MASTER_WINDOWS, XCB_ATOM_CARDINAL, masterWindows, numMasterWindows);
    setWindowProperty(root, MPX_WM_ACTIVE_MASTER, XCB_ATOM_CARDINAL, getActiveMaster()->getID());
    for(WindowInfo* winInfo : getAllWindows()) {
        auto mask = ~EXTERNAL_MASKS & (uint32_t)winInfo->getMask();
        if(mask)
            setWindowProperty(winInfo->getID(), MPX_WM_MASKS, XCB_ATOM_CARDINAL, mask);
    }
    flush();
}
void addResumeCustomStateRules(AddFlag flag) {
    getEventRules(X_CONNECTION).add(DEFAULT_EVENT(loadCustomState), flag);
    getEventRules(X_CONNECTION).add(DEFAULT_EVENT(loadMonitorWorkspaceMapping), flag);
    getEventRules(MONITOR_DETECTED).add(DEFAULT_EVENT(loadMonitorWorkspaceMapping), flag);
    getEventRules(SCREEN_CHANGE).add(DEFAULT_EVENT(loadMonitorWorkspaceMapping), flag);
    getBatchEventRules(MONITOR_WORKSPACE_CHANGE).add(DEFAULT_EVENT(saveMonitorWorkspaceMapping), flag);
    getBatchEventRules(DEVICE_EVENT).add(DEFAULT_EVENT(saveCustomState), flag);
    getBatchEventRules(TILE_WORKSPACE).add(DEFAULT_EVENT(saveCustomState), flag);
}

