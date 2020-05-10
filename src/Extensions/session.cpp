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

/// Stores the active master so the state can be restored
xcb_atom_t MPX_WM_ACTIVE_MASTER;
/// Atom to store fake monitors
xcb_atom_t MPX_WM_FAKE_MONITORS;
/// Used to save raw window masks
xcb_atom_t MPX_WM_MASKS;
/// Str representation of MPX_WM_MASKS used solely for debugging
xcb_atom_t MPX_WM_MASKS_STR;
/// Atom to store an array of each window for every master so the state can be restored
/// There is a '0' to separate each master's window stack and each stack is preceded with the master id
xcb_atom_t MPX_WM_MASTER_WINDOWS;
/// pair of master index and master's workspace index
xcb_atom_t MPX_WM_MASTER_WORKSPACES;
/// Atom to store an array of the layout offset for each workspace so the state can be restored
xcb_atom_t MPX_WM_WORKSPACE_LAYOUT_INDEXES;
/// Atom to store an array of the active layout's for each workspace so the state can be restored
xcb_atom_t MPX_WM_WORKSPACE_LAYOUT_NAMES;
/// Atom to store a mapping or monitor name to workspace name so a monitor can resume its workspace when it is disconnected and reconnected
xcb_atom_t MPX_WM_WORKSPACE_MONITORS;

/// Used to preserves the workspace stack order
xcb_atom_t MPX_WM_WORKSPACE_ORDER;
static void initSessionAtoms() {
    CREATE_ATOM(MPX_WM_ACTIVE_MASTER);
    CREATE_ATOM(MPX_WM_FAKE_MONITORS);
    CREATE_ATOM(MPX_WM_MASKS);
    CREATE_ATOM(MPX_WM_MASKS_STR);
    CREATE_ATOM(MPX_WM_MASTER_WINDOWS);
    CREATE_ATOM(MPX_WM_MASTER_WORKSPACES);
    CREATE_ATOM(MPX_WM_WORKSPACE_LAYOUT_INDEXES);
    CREATE_ATOM(MPX_WM_WORKSPACE_LAYOUT_NAMES);
    CREATE_ATOM(MPX_WM_WORKSPACE_MONITORS);
    CREATE_ATOM(MPX_WM_WORKSPACE_ORDER);
}

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
    INFO("Restored workspace layouts" << getAllWorkspaces());
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
        for(uint32_t i = 0; i < xcb_get_property_value_length(reply.get()) / (sizeof(MonitorID) * 5) ; i++) {
            MonitorID* values = (MonitorID*) & (((char*)xcb_get_property_value(reply.get()))[i * sizeof(MonitorID) * 5]);
            MonitorID id = values[0];
            values++;
            Monitor* m = getAllMonitors().find(id);
            if(m)
                m->setBase(values);
            else
                getAllMonitors().add(new Monitor(id, values, 0, "", 1));
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
static void loadSavedMasterWorkspaces() {
    TRACE("Loading Master Workspace indexes");
    auto reply = getWindowProperty(root, MPX_WM_MASTER_WORKSPACES, XCB_ATOM_CARDINAL);
    if(reply) {
        WorkspaceID* id = (WorkspaceID*)xcb_get_property_value(reply.get());
        for(uint32_t i = 0; i + 1 < xcb_get_property_value_length(reply.get()) / sizeof(int); i += 2) {
            if(getMasterByID(id[i]))
                getMasterByID(id[i])->setWorkspaceIndex(id[i + 1]);
        }
    }
}
static void loadSavedWorkspaceWindows() {
    TRACE("Loading Master window stacks");
    auto reply = getWindowProperty(root, MPX_WM_WORKSPACE_ORDER, XCB_ATOM_CARDINAL);
    if(reply) {
        WindowID* wid = (WindowID*)xcb_get_property_value(reply.get());
        WorkspaceID workspaceID = 0;
        for(uint32_t i = 0; i < xcb_get_property_value_length(reply.get()) / sizeof(int); i++)
            if(wid[i] == 0) {
                if(++workspaceID == getNumberOfWorkspaces())
                    break;
            }
            else {
                WindowInfo* winInfo = getWindowInfo(wid[i]);
                if(winInfo && winInfo->getWorkspaceIndex() == workspaceID) {
                    Workspace* w = getWorkspace(workspaceID);
                    w->getWindowStack().shiftToEnd(w->getWindowStack().indexOf(winInfo));
                }
            }
    }
}
static void loadSavedActiveMaster() {
    TRACE("Loading active Master");
    auto reply = getWindowProperty(root, MPX_WM_ACTIVE_MASTER, XCB_ATOM_CARDINAL);
    if(reply) {
        setActiveMasterByDeviceID(*(MasterID*)xcb_get_property_value(reply.get()));
        INFO("Restored the active master to " >> *getActiveMaster());
    }
}
void loadSavedMonitorWorkspaceMapping() {
    auto reply = getWindowProperty(root, MPX_WM_WORKSPACE_MONITORS, XCB_ATOM_CARDINAL);
    if(reply) {
        uint32_t len = xcb_get_property_value_length(reply.get()) / sizeof(short);
        auto data = (short*) xcb_get_property_value(reply.get());
        for(int i = 0; i + 4 < len; i += 5) {
            WorkspaceID id = data[i];
            Rect bounds = &data[i + 1];
            Workspace* workspace = getWorkspace(id);
            INFO("ID:" << id << " " << bounds << " " << workspace->isVisible());
            if(workspace && !workspace->isVisible())
                for(Monitor* m : getAllMonitors())
                    if(!m->getWorkspace() && m->getBase() == bounds) {
                        INFO("Restoring Monitor '" << m->getID() << "' to Workspace: " << workspace->getID());
                        workspace->setMonitor(m);
                        break;
                    }
        }
    }
    INFO(getAllMonitors());
}

void loadWindowMasks() {
    for(WindowInfo* winInfo : getAllWindows()) {
        WindowMask mask = ~EXTERNAL_MASKS & getWindowPropertyValue(winInfo->getID(), MPX_WM_MASKS, XCB_ATOM_CARDINAL);
        if(mask) {
            INFO("Restoring mask of window:" >> *winInfo);
            if(!winInfo->hasMask(mask)) {
                winInfo->addMask(mask);
                INFO("Added masks:" << mask);
            }
        }
    }
}

void loadSavedNonWindowState(void) {
    loadSavedLayouts();
    loadSavedLayoutOffsets();
    loadSavedFakeMonitor();
    loadSavedMonitorWorkspaceMapping();
    loadSavedMasterWorkspaces();
    loadSavedActiveMaster();
}

void loadSavedWindowState(void) {
    loadSavedMasterWindows();
    loadSavedWorkspaceWindows();
    loadWindowMasks();
}


static std::unordered_map<uint64_t, WorkspaceID> boundsToWorkspaceMapping;

void saveMonitorWorkspaceMapping() {
    for(Monitor* monitor : getAllMonitors())
        if(monitor->getWorkspace()) {
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
    setWindowProperty(root, MPX_WM_WORKSPACE_MONITORS, XCB_ATOM_CARDINAL, bounds, i);
}
void saveCustomState(void) {
    DEBUG("Saving custom State");
    int layoutOffsets[getNumberOfWorkspaces()];
    MonitorID fakeMonitors[getAllMonitors().size() * 5];
    WindowID masterWindows[(getAllWindows().size() + 2) * getAllMasters().size()];
    WorkspaceID masterWorkspaces[getAllMasters().size() * 2];
    WindowID workspaceWindows[getAllWindows().size() + getNumberOfWorkspaces() ];
    int numFakeMonitors = 0;
    int numMasterWindows = 0;
    int numWorkspaceWindows = 0;
    for(int i = getAllMasters().size() - 1; i >= 0; i--) {
        Master* master = getAllMasters()[i];
        masterWindows[numMasterWindows++] = 0;
        masterWindows[numMasterWindows++] = master->getID();
        masterWorkspaces[2 * i] = master->getID();
        masterWorkspaces[2 * i + 1] = master->getWorkspaceIndex();
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
        layoutOffsets[i] = getWorkspace(i)->getLayoutOffset();
        Layout* layout = getWorkspace(i)->getActiveLayout();
        joiner.add(layout ? layout->getName() : "");
        for(WindowInfo* winInfo : getWorkspace(i)->getWindowStack())
            workspaceWindows[numWorkspaceWindows++] = winInfo->getID();
        workspaceWindows[numWorkspaceWindows++] = 0;
    }
    setWindowProperty(root, MPX_WM_ACTIVE_MASTER, XCB_ATOM_CARDINAL, getActiveMaster()->getID());
    setWindowProperty(root, MPX_WM_FAKE_MONITORS, XCB_ATOM_CARDINAL, fakeMonitors, numFakeMonitors);
    setWindowProperty(root, MPX_WM_MASTER_WINDOWS, XCB_ATOM_CARDINAL, masterWindows, numMasterWindows);
    setWindowProperty(root, MPX_WM_MASTER_WORKSPACES, XCB_ATOM_CARDINAL, masterWorkspaces, LEN(masterWorkspaces));
    setWindowProperty(root, MPX_WM_WORKSPACE_LAYOUT_INDEXES, XCB_ATOM_CARDINAL, layoutOffsets, LEN(layoutOffsets));
    setWindowProperty(root, MPX_WM_WORKSPACE_LAYOUT_NAMES, ewmh->UTF8_STRING, joiner.getBuffer(), joiner.getSize());
    setWindowProperty(root, MPX_WM_WORKSPACE_ORDER, XCB_ATOM_CARDINAL, workspaceWindows, numWorkspaceWindows);
    for(WindowInfo* winInfo : getAllWindows()) {
        if(!winInfo->isNotManageable()) {
            WindowMask mask = ~EXTERNAL_MASKS & (uint32_t)winInfo->getMask();
            TRACE("Saving window masks for window: " << *winInfo);
            setWindowProperty(winInfo->getID(), MPX_WM_MASKS, XCB_ATOM_CARDINAL, (int)mask);
            setWindowProperty(winInfo->getID(), MPX_WM_MASKS_STR, ewmh->UTF8_STRING, (std::string)mask);
        }
    }
}
void addResumeCustomStateRules(bool remove) {
    getEventRules(X_CONNECTION).add(DEFAULT_EVENT(initSessionAtoms, HIGHEST_PRIORITY), remove);
    getEventRules(X_CONNECTION).add(DEFAULT_EVENT(loadSavedNonWindowState, HIGHER_PRIORITY), remove);
    getEventRules(X_CONNECTION).add(DEFAULT_EVENT(loadSavedWindowState), remove);
    getBatchEventRules(SCREEN_CHANGE).add(DEFAULT_EVENT(loadSavedMonitorWorkspaceMapping), remove);
    getBatchEventRules(MONITOR_WORKSPACE_CHANGE).add(DEFAULT_EVENT(saveMonitorWorkspaceMapping), remove);
    getBatchEventRules(DEVICE_EVENT).add(DEFAULT_EVENT(saveCustomState), remove);
    getBatchEventRules(TILE_WORKSPACE).add(DEFAULT_EVENT(saveCustomState), remove);
}

