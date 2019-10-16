/**
 * @file session.c
 * @brief Save/load MPXManager state
 */
#include <limits.h>
#include <assert.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>


#include "session.h"
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
#include "../compatibility.h"


static void loadSavedLayouts() {
    xcb_get_property_cookie_t cookie;
    xcb_get_property_reply_t* reply;
    LOG(LOG_LEVEL_TRACE, "Loading active layouts\n");
    cookie = xcb_get_property(dis, 0, root, WM_WORKSPACE_LAYOUT_NAMES, ewmh->UTF8_STRING, 0, UINT_MAX);
    if((reply = xcb_get_property_reply(dis, cookie, NULL))) {
        uint32_t len = xcb_get_property_value_length(reply);
        char* strings = (char*) xcb_get_property_value(reply);
        uint32_t index = 0, count = 0;
        while(index < len) {
            Layout* layout = findLayoutByName(std::string(&strings[index]));
            getWorkspace(count++)->setActiveLayout(layout);
            index += strlen(&strings[index]) + 1;
            if(count == getNumberOfWorkspaces())
                break;
        }
    }
    free(reply);
}
static void loadSavedLayoutOffsets() {
    xcb_get_property_cookie_t cookie;
    xcb_get_property_reply_t* reply;
    LOG(LOG_LEVEL_TRACE, "Loading Workspace layout offsets\n");
    cookie = xcb_get_property(dis, 0, root, WM_WORKSPACE_LAYOUT_INDEXES, XCB_ATOM_CARDINAL, 0, UINT_MAX);
    if((reply = xcb_get_property_reply(dis, cookie, NULL)))
        for(uint32_t i = 0; i < xcb_get_property_value_length(reply) / sizeof(int) && i < getNumberOfWorkspaces(); i++)
            getWorkspace(i)->setLayoutOffset(((int*)xcb_get_property_value(reply))[i]);
    free(reply);
}
static void loadSavedMonitorWorkspaceMapping() {
    xcb_get_property_cookie_t cookie;
    xcb_get_property_reply_t* reply;
    LOG(LOG_LEVEL_TRACE, "Loading Workspace monitor mappings\n");
    cookie = xcb_get_property(dis, 0, root, WM_WORKSPACE_MONITORS, XCB_ATOM_CARDINAL, 0, UINT_MAX);
    if((reply = xcb_get_property_reply(dis, cookie, NULL))) {
        for(uint32_t i = 0; i < xcb_get_property_value_length(reply) / sizeof(int) && i < getNumberOfWorkspaces(); i++) {
            Monitor* m = getAllMonitors().find(((MonitorID*)xcb_get_property_value(reply))[i]);
            if(m) {
                Workspace* w = m->getWorkspace();
                if(w)
                    swapMonitors(i, w->getID());
            }
        }
        free(reply);
    }
}
static void loadSavedWorkspaceWindows() {
    xcb_get_property_cookie_t cookie;
    xcb_get_property_reply_t* reply;
    LOG(LOG_LEVEL_TRACE, "Loading Workspace window stacks\n");
    cookie = xcb_get_property(dis, 0, root, WM_WORKSPACE_WINDOWS, XCB_ATOM_CARDINAL, 0, UINT_MAX);
    if((reply = xcb_get_property_reply(dis, cookie, NULL))) {
        WorkspaceID index = 0;
        WindowID* wid = (WindowID*) xcb_get_property_value(reply);
        for(uint32_t i = 0; i < xcb_get_property_value_length(reply) / sizeof(int); i++)
            if(wid[i] == 0) {
                index++;
                if(index == getNumberOfWorkspaces())
                    break;
            }
            else if(getWindowInfo(wid[i])) {
                if(getWindowInfo(wid[i]))
                    getWindowInfo(wid[i])->moveToWorkspace(index);
                assert(getWorkspace(index)->getWindowStack().back()->getID() == wid[i]);
            }
        free(reply);
    }
}
static void loadSavedMasterWindows() {
    xcb_get_property_cookie_t cookie;
    xcb_get_property_reply_t* reply;
    LOG(LOG_LEVEL_TRACE, "Loading Master window stacks\n");
    cookie = xcb_get_property(dis, 0, root, WM_MASTER_WINDOWS, XCB_ATOM_CARDINAL, 0, UINT_MAX);
    if((reply = xcb_get_property_reply(dis, cookie, NULL))) {
        Master* master = NULL;
        WindowID* wid = (WindowID*)xcb_get_property_value(reply);
        for(uint32_t i = 0; i < xcb_get_property_value_length(reply) / sizeof(int); i++)
            if(wid[i] == 0) {
                master = getMasterById(wid[++i]);
            }
            else if(master)
                master->onWindowFocus(wid[i]);
        free(reply);
    }
}
static void loadSavedActiveMaster() {
    xcb_get_property_cookie_t cookie;
    xcb_get_property_reply_t* reply;
    LOG(LOG_LEVEL_TRACE, "Loading active Master\n");
    cookie = xcb_get_property(dis, 0, root, WM_ACTIVE_MASTER, XCB_ATOM_CARDINAL, 0, sizeof(MasterID));
    if((reply = xcb_get_property_reply(dis, cookie, NULL)) && xcb_get_property_value_length(reply)) {
        setActiveMasterByDeviceId(*(MasterID*)xcb_get_property_value(reply));
        free(reply);
    }
}

void loadCustomState(void) {
    loadSavedLayouts();
    loadSavedLayoutOffsets();
    loadSavedMonitorWorkspaceMapping();
    loadSavedWorkspaceWindows();
    loadSavedMasterWindows();
    loadSavedActiveMaster();
}
void saveCustomState(void) {
    int layoutOffsets[getNumberOfWorkspaces()];
    int monitors[getNumberOfWorkspaces()];
    WindowID windows[getAllWindows().size() + 2 * getNumberOfWorkspaces()];
    WindowID masterWindows[(getAllWindows().size() + 2) * getAllMasters().size()];
    int numWindows = 0;
    int numMasterWindows = 0;
    int len = 0;
    int bufferSize = 64;
    char* activeLayoutNames = (char*) malloc(bufferSize);
    for(int i = getAllMasters().size() - 1; i >= 0; i--) {
        Master* master = getAllMasters()[i];
        masterWindows[numMasterWindows++] = 0;
        masterWindows[numMasterWindows++] = master->getID();
        for(auto p = master->getWindowStack().rbegin(); p != master->getWindowStack().rend(); ++p)
            masterWindows[numMasterWindows++] = (*p)->getID();
    }
    for(WorkspaceID i = 0; i < getNumberOfWorkspaces(); i++) {
        for(WindowInfo* winInfo : getWorkspace(i)->getWindowStack()) {
            windows[numWindows++] = winInfo->getID();
        }
        windows[numWindows++] = 0;
        Monitor* m = getWorkspace(i)->getMonitor();
        monitors[i] = m ? m->getID() : 0;
        layoutOffsets[i] = getWorkspace(i)->getLayoutOffset();
        Layout* layout = getWorkspace(i)->getActiveLayout();
        std::string name = layout ? layout->getName() : "";
        const int requiredSize = len + (name.length()) + 2;
        if(bufferSize < requiredSize) {
            bufferSize = requiredSize * 2;
            activeLayoutNames = (char*)realloc(activeLayoutNames, bufferSize);
        }
        strcpy(&activeLayoutNames[len], name.c_str());
        len += (name.length()) + 1;
    }
    xcb_atom_t REPLACE = XCB_PROP_MODE_REPLACE;
    xcb_change_property(dis, REPLACE, root, WM_WORKSPACE_LAYOUT_NAMES, ewmh->UTF8_STRING, 8, len, activeLayoutNames);
    xcb_change_property(dis, REPLACE, root, WM_WORKSPACE_LAYOUT_INDEXES, XCB_ATOM_CARDINAL, 32,  LEN(layoutOffsets),
        layoutOffsets);
    xcb_change_property(dis, REPLACE, root, WM_WORKSPACE_WINDOWS, XCB_ATOM_CARDINAL, 32, numWindows, windows);
    xcb_change_property(dis, REPLACE, root, WM_WORKSPACE_MONITORS, XCB_ATOM_CARDINAL, 32, LEN(monitors), monitors);
    xcb_change_property(dis, REPLACE, root, WM_MASTER_WINDOWS, XCB_ATOM_CARDINAL, 32, numMasterWindows, masterWindows);
    xcb_change_property(dis, REPLACE, root, WM_ACTIVE_MASTER, XCB_ATOM_CARDINAL, 32, 1, getActiveMaster());
    free(activeLayoutNames);
    flush();
}
void addResumeCustomStateRules() {
    getEventRules(onXConnection).add(DEFAULT_EVENT(loadCustomState));
    getBatchEventRules(TileWorkspace).add(DEFAULT_EVENT(saveCustomState));
}

