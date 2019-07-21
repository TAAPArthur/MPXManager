/**
 * @file session.c
 * @brief Save/load MPXManager state
 */
#include <limits.h>
#include <assert.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>


#include "bindings.h"
#include "devices.h"
#include "layouts.h"
#include "logger.h"
#include "masters.h"
#include "monitors.h"
#include "session.h"
#include "user-events.h"
#include "window-properties.h"
#include "windows.h"
#include "wmfunctions.h"
#include "workspaces.h"
#include "xsession.h"
#include "compatibility.h"

void syncState() {
    WorkspaceID currentWorkspace ;
    if(!xcb_ewmh_get_current_desktop_reply(ewmh,
                                           xcb_ewmh_get_current_desktop(ewmh, defaultScreenNumber),
                                           &currentWorkspace, NULL)) {
        currentWorkspace = getActiveWorkspaceIndex();
    }
    if(currentWorkspace >= getNumberOfWorkspaces())
        currentWorkspace = getNumberOfWorkspaces() - 1;
    xcb_ewmh_set_number_of_desktops(ewmh, defaultScreenNumber, getNumberOfWorkspaces());
    unsigned int value = 0;
    xcb_ewmh_get_showing_desktop_reply(ewmh, xcb_ewmh_get_showing_desktop(ewmh, defaultScreenNumber), &value, NULL);
    setShowingDesktop(value);
    LOG(LOG_LEVEL_INFO, "Current workspace is %d\n", currentWorkspace);
    switchToWorkspace(currentWorkspace);
}

static void loadSavedLayouts() {
    xcb_get_property_cookie_t cookie;
    xcb_get_property_reply_t* reply;
    LOG(LOG_LEVEL_TRACE, "Loading active layouts\n");
    cookie = xcb_get_property(dis, 0, root, WM_WORKSPACE_LAYOUT_NAMES, ewmh->UTF8_STRING, 0, UINT_MAX);
    if((reply = xcb_get_property_reply(dis, cookie, NULL))) {
        int len = xcb_get_property_value_length(reply);
        char* strings = (char*) xcb_get_property_value(reply);
        int index = 0, count = 0;
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
        for(int i = 0; i < xcb_get_property_value_length(reply) / sizeof(int) && i < getNumberOfWorkspaces(); i++)
            getWorkspace(i)->setLayoutOffset(((int*)xcb_get_property_value(reply))[i]);
    free(reply);
}
static void loadSavedMonitorWorkspaceMapping() {
    xcb_get_property_cookie_t cookie;
    xcb_get_property_reply_t* reply;
    LOG(LOG_LEVEL_TRACE, "Loading Workspace monitor mappings\n");
    cookie = xcb_get_property(dis, 0, root, WM_WORKSPACE_MONITORS, XCB_ATOM_CARDINAL, 0, UINT_MAX);
    if((reply = xcb_get_property_reply(dis, cookie, NULL))) {
        for(int i = 0; i < xcb_get_property_value_length(reply) / sizeof(int) && i < getNumberOfWorkspaces(); i++) {
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
        int index = 0;
        WindowID* wid = (WindowID*) xcb_get_property_value(reply);
        for(int i = 0; i < xcb_get_property_value_length(reply) / sizeof(int); i++)
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
        for(int i = 0; i < xcb_get_property_value_length(reply) / sizeof(int); i++)
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

void syncFocus(bool keepLocal = false) {
    for(Master* master : getAllMasters())
        if(keepLocal)
            focusWindow(master->getFocusedWindow(), master);
        else
            master->onWindowFocus(getActiveFocus(master->getID()));
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
    for(int i = 0; i < getNumberOfWorkspaces(); i++) {
        for(WindowInfo* winInfo : getWorkspace(i)->getWindowStack()) {
            windows[numWindows++] = winInfo->getID();
        }
        windows[numWindows++] = 0;
        Monitor* m = getWorkspace(i)->getMonitor();
        monitors[i] = m ? m->getID() : 0;
        layoutOffsets[i] = getWorkspace(i)->getLayoutOffset();
        Layout* layout = getWorkspace(i)->getActiveLayout();
        std::string name = layout ? layout->getName() : "";
        if(bufferSize < len + (name.length()) + 2)
            activeLayoutNames = (char*)realloc(activeLayoutNames, bufferSize *= 2);
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
void setXWindowStateFromMask(WindowInfo* winInfo) {
    xcb_atom_t supportedStates[] = {SUPPORTED_STATES};
    xcb_ewmh_get_atoms_reply_t reply;
    int count = 0;
    int hasState = xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, winInfo->getID()), &reply, NULL);
    xcb_atom_t windowState[LEN(supportedStates) + (hasState ? reply.atoms_len : 0)];
    if(hasState) {
        for(int i = 0; i < reply.atoms_len; i++) {
            char isSupportedState = 0;
            for(int n = 0; n < LEN(supportedStates); n++)
                if(supportedStates[n] == reply.atoms[i]) {
                    isSupportedState = 1;
                    break;
                }
            if(!isSupportedState)
                windowState[count++] = reply.atoms[i];
        }
        xcb_ewmh_get_atoms_reply_wipe(&reply);
    }
    if(winInfo->hasMask(STICKY_MASK))
        windowState[count++] = ewmh->_NET_WM_STATE_STICKY;
    if(winInfo->hasMask(HIDDEN_MASK))
        windowState[count++] = ewmh->_NET_WM_STATE_HIDDEN;
    if(winInfo->hasMask(Y_MAXIMIZED_MASK))
        windowState[count++] = ewmh->_NET_WM_STATE_MAXIMIZED_VERT;
    if(winInfo->hasMask(X_MAXIMIZED_MASK))
        windowState[count++] = ewmh->_NET_WM_STATE_MAXIMIZED_HORZ;
    if(winInfo->hasMask(URGENT_MASK))
        windowState[count++] = ewmh->_NET_WM_STATE_DEMANDS_ATTENTION;
    if(winInfo->hasMask(FULLSCREEN_MASK))
        windowState[count++] = ewmh->_NET_WM_STATE_FULLSCREEN;
    if(winInfo->hasMask(ROOT_FULLSCREEN_MASK))
        windowState[count++] = WM_STATE_ROOT_FULLSCREEN;
    if(winInfo->hasMask(NO_TILE_MASK))
        windowState[count++] = WM_STATE_NO_TILE;
    if(winInfo->hasMask(ABOVE_MASK))
        windowState[count++] = ewmh->_NET_WM_STATE_ABOVE;
    if(winInfo->hasMask(BELOW_MASK))
        windowState[count++] = ewmh->_NET_WM_STATE_BELOW;
    xcb_ewmh_set_wm_state(ewmh, winInfo->getID(), count, windowState);
}
static WindowMasks getMaskFromAtom(xcb_atom_t atom) {
    if(atom == ewmh->_NET_WM_STATE_STICKY)
        return STICKY_MASK;
    else if(atom == ewmh->_NET_WM_STATE_HIDDEN)
        return HIDDEN_MASK;
    else if(atom == ewmh->_NET_WM_STATE_MAXIMIZED_VERT)
        return Y_MAXIMIZED_MASK;
    else if(atom == ewmh->_NET_WM_STATE_MAXIMIZED_HORZ)
        return X_MAXIMIZED_MASK;
    else if(atom == ewmh->_NET_WM_STATE_DEMANDS_ATTENTION)
        return URGENT_MASK;
    else if(atom == ewmh->_NET_WM_STATE_FULLSCREEN)
        return FULLSCREEN_MASK;
    else if(atom == WM_STATE_ROOT_FULLSCREEN)
        return ROOT_FULLSCREEN_MASK;
    else if(atom == WM_STATE_NO_TILE)
        return NO_TILE_MASK;
    else if(atom == ewmh->_NET_WM_STATE_ABOVE)
        return ABOVE_MASK;
    else if(atom == ewmh->_NET_WM_STATE_BELOW)
        return BELOW_MASK;
    else if(atom == WM_TAKE_FOCUS)
        return WM_TAKE_FOCUS_MASK;
    else if(atom == WM_DELETE_WINDOW)
        return WM_DELETE_WINDOW_MASK;
    return NO_MASK;
}

void setWindowStateFromAtomInfo(WindowInfo* winInfo, const xcb_atom_t* atoms, int numberOfAtoms, int action) {
    LOG(LOG_LEVEL_TRACE, "Updating state of %d from %d atoms\n", winInfo->getID(), numberOfAtoms);
    WindowMask mask = 0;
    for(unsigned int i = 0; i < numberOfAtoms; i++) {
        mask |= getMaskFromAtom(atoms[i]);
    }
    if(action == XCB_EWMH_WM_STATE_TOGGLE) {
        if(winInfo->hasMask(mask))
            action = XCB_EWMH_WM_STATE_REMOVE;
        else
            action = XCB_EWMH_WM_STATE_ADD;
    }
    if(action == XCB_EWMH_WM_STATE_REMOVE)
        winInfo->removeMask(mask);
    else
        winInfo->addMask(mask);
}
void loadSavedAtomState(WindowInfo* winInfo) {
    xcb_ewmh_get_atoms_reply_t reply;
    if(xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, winInfo->getID()), &reply, NULL)) {
        setWindowStateFromAtomInfo(winInfo, reply.atoms, reply.atoms_len, XCB_EWMH_WM_STATE_ADD);
        xcb_ewmh_get_atoms_reply_wipe(&reply);
    }
}
static inline void setEnvRect(const char* name, const Rect& rect) {
    const char var[4][32] = {"_%s_X", "_%s_Y", "_%s_WIDTH", "_%s_HEIGHT"};
    char strName[32];
    char strValue[8];
    for(int n = 0; n < 4; n++) {
        sprintf(strName, var[n], name);
        sprintf(strValue, "%d", rect[n]);
        setenv(strName, strValue, 1);
    }
}
void autoAddToWorkspace(WindowInfo* winInfo) {
    winInfo->moveToWorkspace(getSavedWorkspaceIndex(winInfo->getID()));
}
void setClientMasterEnvVar(void) {
    char strValue[8];
    assert(getActiveMaster());
    sprintf(strValue, "%d", getActiveMasterKeyboardID());
    setenv(DEFAULT_KEYBOARD_ENV_VAR_NAME, strValue, 1);
    sprintf(strValue, "%d", getActiveMasterPointerID());
    setenv(DEFAULT_POINTER_ENV_VAR_NAME, strValue, 1);
    if(getFocusedWindow()) {
        sprintf(strValue, "%d", getFocusedWindow()->getID());
        setenv("_WIN_ID", strValue, 1);
    }
    Monitor* m = getActiveWorkspace()->getMonitor();
    if(getFocusedWindow())
        setEnvRect("WIN", getFocusedWindow()->getGeometry());
    if(m) {
        setEnvRect("VIEW", m->getViewport());
        setEnvRect("MON", m->getBase());
    }
    const Rect rootBounds = {0, 0, getRootWidth(), getRootHeight()};
    setEnvRect("ROOT", rootBounds);
    if(LD_PRELOAD_INJECTION)
        setenv("LD_PRELOAD", LD_PRELOAD_PATH.c_str(), 1);
}
void addResumeRules() {
    getEventRules(onXConnection).add(new BoundFunction(loadCustomState));
    getEventRules(onXConnection).add(new BoundFunction(syncState));
    getEventRules(PostRegisterWindow).add(new BoundFunction(loadSavedAtomState));
    getEventRules(PostRegisterWindow).add(new BoundFunction(+[](WindowInfo * winInfo) {if(!winInfo->isDock())autoAddToWorkspace(winInfo);}));
}

