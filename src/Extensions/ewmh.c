#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

#include "../util/arraylist.h"
#include "../util/debug.h"
#include "../bindings.h"
#include "../devices.h"
#include "ewmh.h"
#include "../globals.h"
#include "../util/logger.h"
#include "../system.h"
#include "../util/time.h"
#include "../user-events.h"
#include "../xutil/window-properties.h"
#include "../xutil/string-array.h"
#include "../windows.h"
#include "../wmfunctions.h"
#include "../functions.h"
#include "../xutil/xsession.h"

xcb_ewmh_client_source_type_t source = XCB_EWMH_CLIENT_SOURCE_TYPE_OTHER;

bool isMPXManagerRunningAsWM(void) {
    xcb_get_selection_owner_reply_t* ownerReply = xcb_get_selection_owner_reply(dis, xcb_get_selection_owner(dis,
                WM_SELECTION_ATOM), NULL);
    bool result = 0;
    if(ownerReply && ownerReply->owner) {
        char buffer[MAX_NAME_LEN];
        if(getWindowTitle(ownerReply->owner, buffer))
            result = strcmp(buffer, WINDOW_MANAGER_NAME) == 0;
    }
    free(ownerReply);
    return result;
}
void broadcastEWMHCompilence() {
    DEBUG("Complying with EWMH");
    //functionless window required by EWMH spec
    //we set its class to input only and set override redirect so we (and anyone else ignore it)
    ownSelection(WM_SELECTION_ATOM);
    setWindowClass(getPrivateWindow(), numPassedArguments ? passedArguments[0] : WINDOW_MANAGER_NAME, WINDOW_MANAGER_NAME);
    setWindowPropertyInt(getPrivateWindow(), MPX_RESTART_COUNTER, XCB_ATOM_CARDINAL, RESTART_COUNTER);
    xcb_ewmh_set_supporting_wm_check(ewmh, root, getPrivateWindow());
    xcb_ewmh_set_supporting_wm_check(ewmh, getPrivateWindow(), getPrivateWindow());
    setWindowTitle(getPrivateWindow(), WINDOW_MANAGER_NAME);
    // Not strictly needed
    updateWorkspaceNames();
    xcb_ewmh_set_number_of_desktops(ewmh, defaultScreenNumber, getNumberOfWorkspaces());
    setSupportedActions();
    DEBUG("Complied with EWMH/ICCCM specs");
}


void setActiveProperties() {
    static WindowInfo* lastActiveWindow;
    static WorkspaceID lastActiveWorkspaceIndex;
    WindowInfo* winInfo = getFocusedWindow();
    if(lastActiveWindow != winInfo) {
        xcb_ewmh_set_active_window(ewmh, defaultScreenNumber, winInfo ? winInfo->id : root);
        lastActiveWindow = winInfo;
    }
    if(lastActiveWorkspaceIndex != getActiveWorkspaceIndex()) {
        xcb_ewmh_set_current_desktop_checked(ewmh, defaultScreenNumber, getActiveWorkspaceIndex());
        lastActiveWorkspaceIndex = getActiveWorkspaceIndex();
    }
}
static void updateEWMHWorkspaceProperties() {
    xcb_ewmh_coordinates_t viewPorts[getNumberOfWorkspaces()];
    xcb_ewmh_geometry_t workAreas[getNumberOfWorkspaces()];
    FOR_EACH(Workspace*, w, getAllWorkspaces()) {
        if(isWorkspaceVisible(w))
            copyTo(&getMonitor(w)->view, 0, ((uint32_t*)&workAreas[w->id]));
    }
    // top left point of each desktop
    xcb_ewmh_set_desktop_viewport(ewmh, defaultScreenNumber, getNumberOfWorkspaces(), viewPorts);
    // viewport of each desktop
    xcb_ewmh_set_workarea(ewmh, defaultScreenNumber, getNumberOfWorkspaces(), workAreas);
    // root screen
    xcb_ewmh_set_desktop_geometry(ewmh, defaultScreenNumber, getRootWidth(), getRootHeight());
}
void updateWorkspaceNames() {
    StringJoiner joiner = {0};
    FOR_EACH(Workspace*, w, getAllWorkspaces()) {
        addString(&joiner, w->name);
    }
    xcb_ewmh_set_desktop_names(ewmh, defaultScreenNumber, joiner.bufferSize, getBuffer(&joiner));
    freeBuffer(&joiner);
}

WorkspaceID getSavedWorkspaceIndex(WindowID win) {
    WorkspaceID workspaceIndex = getActiveWorkspaceIndex();
    if((xcb_ewmh_get_wm_desktop_reply(ewmh,
                xcb_ewmh_get_wm_desktop(ewmh, win), &workspaceIndex, NULL))) {
        if(workspaceIndex != NO_WORKSPACE && workspaceIndex >= getNumberOfWorkspaces()) {
            workspaceIndex = getNumberOfWorkspaces() - 1;
        }
    }
    else workspaceIndex = getActiveWorkspaceIndex();
    return workspaceIndex;
}
void setSavedWorkspaceIndex(WindowInfo* winInfo) {
    xcb_ewmh_set_wm_desktop(ewmh, winInfo->id, getWorkspaceIndexOfWindow(winInfo));
}
void autoResumeWorkspace(WindowInfo* winInfo) {
    if(!getWorkspaceOfWindow(winInfo)) {
        moveToWorkspace(winInfo, getSavedWorkspaceIndex(winInfo->id));
    }
}
static inline const char* getSourceTypeName(xcb_ewmh_client_source_type_t source) {
    switch(source) {
        default:
        case XCB_EWMH_CLIENT_SOURCE_TYPE_NONE:
            return "Old";
        case XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL:
            return "Application";
        case XCB_EWMH_CLIENT_SOURCE_TYPE_OTHER:
            return "Other";
    }
}
/**
 * Checks to see if the window has SRC* masks set that will allow. If not client requests with such a source will be ignored
 * @param source
 * @return
 */
static bool allowRequestFromSource(int source) {
    INFO("Got request from %s", getSourceTypeName((xcb_ewmh_client_source_type_t)source));
    return SRC_INDICATION & (1 << (source));
}

void onClientMessage(xcb_client_message_event_t* event) {
    xcb_client_message_data_t data = event->data;
    xcb_window_t win = event->window;
    Atom message = event->type;
    INFO("Received client message for window: %d message: %s", win, getAtomName(message, NULL));
    WindowInfo* winInfo = getWindowInfo(win);
    if(message == ewmh->_NET_CURRENT_DESKTOP)
        switchToWorkspace(data.data32[0]);
    else if(message == ewmh->_NET_ACTIVE_WINDOW) {
        //data: source,timestamp,current active window
        if(winInfo && allowRequestFromSource(data.data32[0])) {
            Master* m = data.data32[3] ? getMasterByID(data.data32[3]) : getClientMaster(win);
            if(m)
                setActiveMaster(m);
            DEBUG("Activating window %d for master %d due to client request", win, getActiveMaster()->id);
            activateWindow(winInfo);
        }
    }
    else if(message == ewmh->_NET_SHOWING_DESKTOP) {
        DEBUG("Changing showing desktop to %d\n", data.data32[0]);
        setShowingDesktop(data.data32[0]);
    }
    else if(message == ewmh->_NET_CLOSE_WINDOW) {
        DEBUG("Killing window %d\n", win);
        if(!winInfo)
            killClientOfWindow(win);
        else if(allowRequestFromSource(data.data32[0])) {
            killClientOfWindowInfo(winInfo);
        }
    }
    else if(message == ewmh->_NET_RESTACK_WINDOW) {
        DEBUG("Restacking Window %d sibling %d detail %d\n", win, data.data32[1], data.data32[2]);
        if(winInfo && allowRequestFromSource(data.data32[0]))
            processConfigureRequest(win, NULL, data.data32[1], data.data32[2],
                XCB_CONFIG_WINDOW_STACK_MODE | XCB_CONFIG_WINDOW_SIBLING);
    }
    else if(message == ewmh->_NET_REQUEST_FRAME_EXTENTS) {
        xcb_ewmh_set_frame_extents(ewmh, win, DEFAULT_BORDER_WIDTH, DEFAULT_BORDER_WIDTH, DEFAULT_BORDER_WIDTH,
            DEFAULT_BORDER_WIDTH);
    }
    else if(message == ewmh->_NET_MOVERESIZE_WINDOW) {
        DEBUG("Move/Resize window request %d\n", win);
        int mask = data.data8[1] & 15;
        int source = (data.data8[1] >> 4) & 15;
        short values[4];
        for(int i = 1; i <= 4; i++)
            values[i - 1] = data.data32[i];
        if(winInfo && allowRequestFromSource(source)) {
            processConfigureRequest(win, values, 0, 0, mask);
        }
    }
    else if(message == ewmh->_NET_WM_MOVERESIZE) {
        // x,y, direction, button, src
        if(winInfo && allowRequestFromSource(data.data32[4])) {
            int dir = data.data32[2];
            bool disallowMoveX = XCB_EWMH_WM_MOVERESIZE_SIZE_TOP == dir || XCB_EWMH_WM_MOVERESIZE_SIZE_BOTTOM == dir ;
            bool disallowMoveY = XCB_EWMH_WM_MOVERESIZE_SIZE_RIGHT == dir || XCB_EWMH_WM_MOVERESIZE_SIZE_LEFT == dir ;
            bool move = dir == XCB_EWMH_WM_MOVERESIZE_MOVE || dir == XCB_EWMH_WM_MOVERESIZE_MOVE_KEYBOARD;
            Master* m = getClientMaster(win);
            setActiveMaster(m);
            if(dir == XCB_EWMH_WM_MOVERESIZE_CANCEL) {
                extern Binding* startWindowMoveBinding;
                if(m->bindings.size)
                    if(peek(&m->bindings) == startWindowMoveBinding || peek(&m->bindings) == startWindowMoveBinding + 1)
                        pop(&m->bindings);
                cancelWindowMoveResize();
            }
            else {
                int btn = data.data32[3] ? data.data32[3] : 1;
                INFO("Starting WM move/resize with button %d (%d)", btn, data.data32[3]);
                startWindowMoveResize(winInfo, move, (disallowMoveY << 1) | disallowMoveX);
                extern Binding* startWindowMoveBinding;
                enterChain(startWindowMoveBinding + !move, &m->bindings);
            }
        }
    }
    //change window's workspace
    else if(message == ewmh->_NET_WM_DESKTOP) {
        WorkspaceID destIndex = data.data32[0];
        if(!(destIndex == NO_WORKSPACE || destIndex < getNumberOfWorkspaces()))
            destIndex = getNumberOfWorkspaces() - 1;
        if(winInfo && allowRequestFromSource(data.data32[1])) {
            DEBUG("Changing window workspace %d %d\n", destIndex, data.data32[0]);
            moveToWorkspace(winInfo, destIndex);
        }
    }
    else if(message == ewmh->_NET_WM_STATE) {
        // action, prop1, prop2, source
        // if prop2 is 0, then there is only 1 property to consider
        int num = data.data32[2] == 0 ? 1 : 2;
        if(winInfo) {
            if(allowRequestFromSource(data.data32[3])) {
                WindowMask mask = winInfo->mask;
                bool added = setWindowStateFromAtomInfo(winInfo, (xcb_atom_t*) &data.data32[1], num, data.data32[0]);
                if(mask != winInfo->mask)
                    setXWindowStateFromMask(winInfo, &data.data32[1], added ? num : 0);
            }
            else
                INFO("Client message denied");
        }
    }
    else if(message == WM_CHANGE_STATE) {
        if(winInfo && allowRequestFromSource(XCB_EWMH_CLIENT_SOURCE_TYPE_NONE))
            if(data.data32[0] == XCB_ICCCM_WM_STATE_ICONIC) {
                if(getMasksToSync(winInfo) & HIDDEN_MASK) {
                    INFO("Received request to iconify window; marking window as HIDDEN");
                    addMask(winInfo, HIDDEN_MASK);
                    setXWindowStateFromMask(winInfo, NULL, 0);
                }
                else
                    INFO("Received request to iconify window; ignored");
            }
    }
    else if(message == ewmh->_NET_NUMBER_OF_DESKTOPS) {
        if(data.data32[0] > 0) {
            int delta = data.data32[0] - getNumberOfWorkspaces();
            if(delta > 0)
                addWorkspaces(delta);
            else {
                WorkspaceID newLastIndex = data.data32[0] - 1;
                if(getActiveWorkspaceIndex() > newLastIndex)
                    setActiveWorkspaceIndex(newLastIndex);
                removeWorkspaces(-delta);
            }
            assert(data.data32[0] == getNumberOfWorkspaces());
            assignUnusedMonitorsToWorkspaces();
        }
    }
    //ignored (we are allowed to) ewmh->_NET_DESKTOP_GEOMETRY, ewmh->_NET_DESKTOP_VIEWPORT
}

void setXWindowStateFromMask(WindowInfo* winInfo, xcb_atom_t* atoms, int len) {
    INFO("Setting X State for window %d from masks  %d", winInfo->id, winInfo->mask);
    xcb_ewmh_get_atoms_reply_t reply;
    bool hasState = xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, winInfo->id), &reply, NULL);
    xcb_atom_t windowState[sizeof(WindowMask) * 8 + (hasState ? reply.atoms_len : 0)];
    int n = 0;
    if(hasState) {
        LOG_RUN(LOG_LEVEL_DEBUG, dumpAtoms(reply.atoms, reply.atoms_len));
        for(uint32_t i = 0; i < reply.atoms_len; i++) {
            if((getMaskFromAtom(reply.atoms[i]) & getMasksToSync(winInfo)) == 0)
                windowState[n++] = reply.atoms[i];
        }
        xcb_ewmh_get_atoms_reply_wipe(&reply);
    }
    if(ALLOW_SETTING_UNSYNCED_MASKS) {
        for(uint32_t i = 0; i < len; i++)
            if((getMaskFromAtom(atoms[i]) & getMasksToSync(winInfo)) == 0)
                windowState[n++] = atoms[i];
    }
    n += getAtomsFromMask(winInfo->mask & getMasksToSync(winInfo), 0, windowState + n);
    xcb_ewmh_set_wm_state(ewmh, winInfo->id, n, windowState);
    dumpAtoms(windowState, n);
}

void updateXWindowStateForAllWindows() {
    FOR_EACH(WindowInfo*, winInfo, getAllWindows()) {
        if(hasMask(winInfo, MAPPABLE_MASK)) {
            if((winInfo->mask ^ winInfo->savedMask) & getMasksToSync(winInfo))
                setXWindowStateFromMask(winInfo, NULL, 0);
        }
    }
}

bool setWindowStateFromAtomInfo(WindowInfo* winInfo, const xcb_atom_t* atoms, uint32_t numberOfAtoms, int action) {
    assert(numberOfAtoms);
    INFO("Setting window masks for window %d %s current masks %d from %d atoms; Action %d", winInfo->id, winInfo->title,
        winInfo->mask,  numberOfAtoms, action);
    dumpAtoms(atoms, numberOfAtoms);
    WindowMask mask = 0;
    for(unsigned int i = 0; i < numberOfAtoms; i++) {
        mask |= getMaskFromAtom(atoms[i]);
    }
    DEBUG("Pre filtered masks: %d", mask);
    mask &= getMasksToSync(winInfo);
    DEBUG("Filtered masks: %d", mask);
    if(action == XCB_EWMH_WM_STATE_TOGGLE)
        toggleMask(winInfo, mask);
    else if(action == XCB_EWMH_WM_STATE_REMOVE)
        removeMask(winInfo, mask);
    else
        addMask(winInfo, mask);
    return hasMask(winInfo, mask);
}
void loadSavedAtomState(WindowInfo* winInfo) {
    xcb_ewmh_get_atoms_reply_t reply;
    if(xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, winInfo->id), &reply, NULL)) {
        if(reply.atoms_len)
            setWindowStateFromAtomInfo(winInfo, reply.atoms, reply.atoms_len, XCB_EWMH_WM_STATE_ADD);
        xcb_ewmh_get_atoms_reply_wipe(&reply);
    }
}
static void setWMState(WindowInfo* winInfo) {
    setWindowPropertyInt(winInfo->id, WM_STATE, XCB_ATOM_CARDINAL, XCB_ICCCM_WM_STATE_NORMAL);
}
static void clearWMState(WindowInfo* winInfo) {
    clearWindowProperty(winInfo->id, WM_STATE);
}

static void rememberMappedWindows(WindowInfo* winInfo) {
    if(getWindowPropertyValueInt(winInfo->id, WM_STATE, XCB_ATOM_CARDINAL))
        addMask(winInfo, MAPPABLE_MASK);
}

void updateDockProperties(xcb_property_notify_event_t* event) {
    WindowInfo* winInfo = getWindowInfo(event->window);
    // only reload properties if a window is mapped
    if(winInfo && hasMask(winInfo, MAPPED_MASK)) {
        if(event->atom == ewmh->_NET_WM_STRUT || event->atom == ewmh->_NET_WM_STRUT_PARTIAL) {
            if(winInfo->dock)
                loadDockProperties(winInfo);
        }
    }
}

void addEWMHRules() {
    addBatchEvent(POST_REGISTER_WINDOW, DEFAULT_EVENT(updateEWMHWorkspaceProperties));
    addBatchEvent(SCREEN_CHANGE, DEFAULT_EVENT(updateEWMHWorkspaceProperties));
    addBatchEvent(TILE_WORKSPACE, DEFAULT_EVENT(updateXWindowStateForAllWindows));
    addEvent(CLIENT_MAP_ALLOW, DEFAULT_EVENT(autoResumeWorkspace));
    addEvent(CLIENT_MAP_ALLOW, DEFAULT_EVENT(loadSavedAtomState));
    addEvent(CLIENT_MAP_ALLOW, DEFAULT_EVENT(setWMState));
    addEvent(CLIENT_MAP_DISALLOW, DEFAULT_EVENT(clearWMState));
    addEvent(POST_REGISTER_WINDOW, DEFAULT_EVENT(rememberMappedWindows));
    addEvent(TRUE_IDLE, DEFAULT_EVENT(setActiveProperties));
    addEvent(WORKSPACE_WINDOW_ADD, DEFAULT_EVENT(setSavedWorkspaceIndex));
    addEvent(XCB_CLIENT_MESSAGE, DEFAULT_EVENT(onClientMessage));
    addEvent(XCB_PROPERTY_NOTIFY, DEFAULT_EVENT(updateDockProperties));
    addEvent(X_CONNECTION, DEFAULT_EVENT(broadcastEWMHCompilence, HIGHER_PRIORITY));
}
