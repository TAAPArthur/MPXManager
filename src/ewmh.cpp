#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include "unistd.h"

#include "arraylist.h"
#include "bindings.h"
#include "devices.h"
#include "ewmh.h"
#include "ext.h"
#include "globals.h"
#include "logger.h"
#include "system.h"
#include "time.h"
#include "user-events.h"
#include "window-properties.h"
#include "windows.h"
#include "wmfunctions.h"
#include "xsession.h"

xcb_ewmh_client_source_type_t source = XCB_EWMH_CLIENT_SOURCE_TYPE_OTHER;
static ArrayList<WindowID> mappedOrder;

bool isMPXManagerRunning(void) {
    xcb_get_selection_owner_reply_t* ownerReply = xcb_get_selection_owner_reply(dis, xcb_get_selection_owner(dis,
                WM_SELECTION_ATOM), NULL);
    bool result = 0;
    if(ownerReply && ownerReply->owner) {
        result = getWindowTitle(ownerReply->owner) == WINDOW_MANAGER_NAME;
    }
    free(ownerReply);
    return result;
}
void broadcastEWMHCompilence() {
    DEBUG("Complying with EWMH");
    //functionless window required by EWMH spec
    //we set its class to input only and set override redirect so we (and anyone else ignore it)
    if(!STEAL_WM_SELECTION) {
        xcb_get_selection_owner_reply_t* ownerReply = xcb_get_selection_owner_reply(dis, xcb_get_selection_owner(dis,
                    WM_SELECTION_ATOM), NULL);
        if(ownerReply->owner) {
            LOG(LOG_LEVEL_ERROR, "Selection %d is already owned by window %d", WM_SELECTION_ATOM, ownerReply->owner);
            quit(0);
        }
        free(ownerReply);
    }
    DEBUG("Setting selection owner");
    if(catchError(xcb_set_selection_owner_checked(dis, getPrivateWindow(), WM_SELECTION_ATOM,
                XCB_CURRENT_TIME)) == 0) {
        unsigned int data[5] = {XCB_CURRENT_TIME, WM_SELECTION_ATOM, getPrivateWindow()};
        xcb_ewmh_send_client_message(dis, root, root, WM_SELECTION_ATOM, 5, data);
    }
    xcb_ewmh_set_wm_pid(ewmh, getPrivateWindow(), getpid());
    setWindowClass(getPrivateWindow(), numPassedArguments ? passedArguments[0] : WINDOW_MANAGER_NAME, WINDOW_MANAGER_NAME);
    xcb_ewmh_set_supporting_wm_check(ewmh, root, getPrivateWindow());
    xcb_ewmh_set_supporting_wm_check(ewmh, getPrivateWindow(), getPrivateWindow());
    setWindowTitle(getPrivateWindow(), WINDOW_MANAGER_NAME);
    // Not strictly needed
    updateWorkspaceNames();
    setSupportedActions();
    DEBUG("Complied with EWMH/ICCCM specs");
}

void setSupportedActions() {
    ArrayList<xcb_atom_t> net_atoms = {
        ewmh->_NET_SUPPORTED,
        ewmh->_NET_SUPPORTING_WM_CHECK,
        ewmh->_NET_WM_NAME,
        ewmh->_NET_CLIENT_LIST,
        ewmh->_NET_WM_STRUT, ewmh->_NET_WM_STRUT_PARTIAL,
        ewmh->_NET_WM_STATE,
        ewmh->_NET_WM_WINDOW_TYPE,
        ewmh->_NET_WM_WINDOW_TYPE_NORMAL,
        ewmh->_NET_WM_WINDOW_TYPE_DOCK,
        ewmh->_NET_WM_WINDOW_TYPE_DIALOG,
        ewmh->_NET_WM_WINDOW_TYPE_UTILITY,
        ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR,
        ewmh->_NET_WM_WINDOW_TYPE_SPLASH,
        ewmh->_NET_WM_WINDOW_TYPE_MENU,
        ewmh->_NET_WM_WINDOW_TYPE_DROPDOWN_MENU,
        ewmh->_NET_WM_WINDOW_TYPE_POPUP_MENU,
        ewmh->_NET_WM_WINDOW_TYPE_NOTIFICATION,
        ewmh->_NET_ACTIVE_WINDOW, ewmh->_NET_CLOSE_WINDOW,
        ewmh->_NET_WM_DESKTOP, ewmh->_NET_NUMBER_OF_DESKTOPS,
        ewmh->_NET_CURRENT_DESKTOP, ewmh->_NET_SHOWING_DESKTOP,
        ewmh->_NET_DESKTOP_NAMES, ewmh->_NET_DESKTOP_VIEWPORT,
        ewmh->_NET_WM_USER_TIME_WINDOW, ewmh->_NET_WM_USER_TIME,
        ewmh->_NET_WM_ACTION_ABOVE, ewmh->_NET_WM_ACTION_BELOW,
        ewmh->_NET_WM_ACTION_CHANGE_DESKTOP,
        ewmh->_NET_WM_ACTION_CLOSE, ewmh->_NET_WM_ACTION_STICK,
        ewmh->_NET_WM_ACTION_FULLSCREEN, ewmh->_NET_WM_ACTION_MINIMIZE,
        ewmh->_NET_WM_ACTION_MAXIMIZE_HORZ, ewmh->_NET_WM_ACTION_MAXIMIZE_VERT,
        ewmh->_NET_WM_ACTION_MOVE, ewmh->_NET_WM_ACTION_RESIZE,
        ewmh->_NET_DESKTOP_GEOMETRY, ewmh->_NET_DESKTOP_VIEWPORT, ewmh->_NET_WORKAREA
    };
    getAtomsFromMask(MASKS_TO_SYNC, net_atoms);
    xcb_ewmh_set_supported_checked(ewmh, defaultScreenNumber, net_atoms.size(), net_atoms.data());
}
void setAllowedActions(WindowInfo* winInfo) {
    ArrayList<xcb_atom_t> actions = {
        ewmh->_NET_WM_ACTION_CHANGE_DESKTOP, ewmh->_NET_WM_ACTION_CLOSE,
    };
    getAtomsFromMask(MASKS_TO_SYNC, actions, 1);
    xcb_ewmh_set_wm_allowed_actions_checked(ewmh, winInfo->getID(), actions.size(), actions.data());
}

void updateEWMHClientList() {
    xcb_ewmh_set_client_list(ewmh, defaultScreenNumber, mappedOrder.size(), (WindowID*)mappedOrder.data());
}

void setActiveProperties() {
    WindowInfo* winInfo = getActiveMaster()->getFocusedWindow();
    xcb_ewmh_set_active_window(ewmh, defaultScreenNumber, winInfo ? winInfo->getID() : root);
    xcb_ewmh_set_current_desktop_checked(ewmh, defaultScreenNumber, getActiveWorkspaceIndex());
}
static void updateEWMHWorkspaceProperties() {
    xcb_ewmh_coordinates_t viewPorts[getNumberOfWorkspaces()] = {0};
    xcb_ewmh_geometry_t workAreas[getNumberOfWorkspaces()] = {0};
    int i = 0;
    for(Workspace* w : getAllWorkspaces()) {
        if(w->isVisible())
            w->getMonitor()->getViewport().copyTo(((uint32_t*)&workAreas[i]));
        i++;
    }
    // top left point of each desktop
    xcb_ewmh_set_desktop_viewport(ewmh, defaultScreenNumber, getNumberOfWorkspaces(), viewPorts);
    // viewport of each desktop
    xcb_ewmh_set_workarea(ewmh, defaultScreenNumber, getNumberOfWorkspaces(), workAreas);
    // root screen
    xcb_ewmh_set_desktop_geometry(ewmh, defaultScreenNumber, getRootWidth(), getRootHeight());
}
void updateWorkspaceNames() {
    StringJoiner joiner;
    for(Workspace* w : getAllWorkspaces())
        joiner.add(w->getName());
    xcb_ewmh_set_desktop_names(ewmh, defaultScreenNumber, joiner.getSize(), joiner.getBuffer());
}

static void recordWindow(WindowInfo* winInfo) {
    if(!winInfo->isNotManageable())
        mappedOrder.addUnique(winInfo->getID());
}

static void unrecordWindow(WindowInfo* winInfo) {
    mappedOrder.removeElement(winInfo->getID());
}

void addEWMHRules(AddFlag flag) {
    getBatchEventRules(POST_REGISTER_WINDOW).add(DEFAULT_EVENT(updateEWMHClientList), flag);
    getBatchEventRules(POST_REGISTER_WINDOW).add(DEFAULT_EVENT(updateEWMHWorkspaceProperties), flag);
    getBatchEventRules(SCREEN_CHANGE).add(DEFAULT_EVENT(updateEWMHWorkspaceProperties), flag);
    getBatchEventRules(UNREGISTER_WINDOW).add(DEFAULT_EVENT(updateEWMHClientList), flag);
    getEventRules(CLIENT_MAP_ALLOW).add(DEFAULT_EVENT(autoResumeWorkspace), flag);
    getEventRules(CLIENT_MAP_ALLOW).add(DEFAULT_EVENT(loadSavedAtomState), flag);
    getEventRules(CLIENT_MAP_ALLOW).add(DEFAULT_EVENT(recordWindow), flag);
    getEventRules(POSSIBLE_STATE_CHANGE).add(DEFAULT_EVENT(updateXWindowStateForAllWindows), flag);
    getEventRules(POST_REGISTER_WINDOW).add(DEFAULT_EVENT(setAllowedActions), flag);
    getEventRules(TRUE_IDLE).add(DEFAULT_EVENT(setActiveProperties), flag);
    getEventRules(UNREGISTER_WINDOW).add(DEFAULT_EVENT(unrecordWindow), flag);
    getEventRules(WINDOW_WORKSPACE_CHANGE).add(DEFAULT_EVENT(setSavedWorkspaceIndex), flag);
    getEventRules(XCB_CLIENT_MESSAGE).add(DEFAULT_EVENT(onClientMessage), flag);
    getEventRules(X_CONNECTION).add(DEFAULT_EVENT(broadcastEWMHCompilence), flag);
    getEventRules(X_CONNECTION).add(DEFAULT_EVENT(syncState), flag);
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
    xcb_ewmh_set_wm_desktop(ewmh, winInfo->getID(), winInfo->getWorkspaceIndex());
}
void autoResumeWorkspace(WindowInfo* winInfo) {
    if(winInfo->getWorkspaceIndex() == NO_WORKSPACE && !winInfo->isSpecial()) {
        WorkspaceID w = getSavedWorkspaceIndex(winInfo->getID());
        winInfo->moveToWorkspace(w);
    }
}

void setShowingDesktop(int value) {
    LOG(LOG_LEVEL_INFO, "setting showing desktop %d", value);
    for(WindowInfo* winInfo : getAllWindows())
        if(winInfo->getType() == ewmh->_NET_WM_WINDOW_TYPE_DESKTOP)
            raiseWindow(winInfo->getID(), 0, value);
    xcb_ewmh_set_showing_desktop(ewmh, defaultScreenNumber, value);
}

bool isShowingDesktop(void) {
    unsigned int value = 0;
    xcb_ewmh_get_showing_desktop_reply(ewmh, xcb_ewmh_get_showing_desktop(ewmh, defaultScreenNumber), &value, NULL);
    return value;
}
static const char* getSourceTypeName(xcb_ewmh_client_source_type_t source) {
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
    INFO("Got request from " << getSourceTypeName((xcb_ewmh_client_source_type_t)source));
    return SRC_INDICATION & (1 << (source));
}

void onClientMessage(void) {
    xcb_client_message_event_t* event = (xcb_client_message_event_t*)getLastEvent();
    xcb_client_message_data_t data = event->data;
    xcb_window_t win = event->window;
    Atom message = event->type;
    LOG(LOG_LEVEL_INFO, "Received client message for window: %d message: %s", win, getAtomName(message).c_str());
    if(message == ewmh->_NET_CURRENT_DESKTOP)
        switchToWorkspace(data.data32[0]);
    else if(message == ewmh->_NET_ACTIVE_WINDOW) {
        //data: source,timestamp,current active window
        WindowInfo* winInfo = getWindowInfo(win);
        if(winInfo && allowRequestFromSource(data.data32[0])) {
            Master* m = data.data32[3] ? getMasterByID(data.data32[3]) : getClientMaster(win);
            if(m)
                setActiveMaster(m);
            LOG(LOG_LEVEL_DEBUG, "Activating window %d for master %d due to client request", win, getActiveMaster()->getID());
            activateWindow(winInfo);
        }
    }
    else if(message == ewmh->_NET_SHOWING_DESKTOP) {
        LOG(LOG_LEVEL_DEBUG, "Changing showing desktop to %d\n", data.data32[0]);
        setShowingDesktop(data.data32[0]);
    }
    else if(message == ewmh->_NET_CLOSE_WINDOW) {
        LOG(LOG_LEVEL_DEBUG, "Killing window %d\n", win);
        WindowInfo* winInfo = getWindowInfo(win);
        if(!winInfo)
            killClientOfWindow(win);
        else if(allowRequestFromSource(data.data32[0])) {
            killClientOfWindowInfo(winInfo);
        }
    }
    else if(message == ewmh->_NET_RESTACK_WINDOW) {
        WindowInfo* winInfo = getWindowInfo(win);
        LOG(LOG_LEVEL_DEBUG, "Restacking Window %d sibling %d detail %d\n", win, data.data32[1], data.data32[2]);
        if(winInfo && allowRequestFromSource(data.data32[0]))
            processConfigureRequest(win, NULL, data.data32[1], data.data32[2],
                XCB_CONFIG_WINDOW_STACK_MODE | XCB_CONFIG_WINDOW_SIBLING);
    }
    else if(message == ewmh->_NET_REQUEST_FRAME_EXTENTS) {
        xcb_ewmh_set_frame_extents(ewmh, win, DEFAULT_BORDER_WIDTH, DEFAULT_BORDER_WIDTH, DEFAULT_BORDER_WIDTH,
            DEFAULT_BORDER_WIDTH);
    }
    else if(message == ewmh->_NET_MOVERESIZE_WINDOW) {
        LOG(LOG_LEVEL_DEBUG, "Move/Resize window request %d\n", win);
        int mask = data.data8[1] & 15;
        int source = (data.data8[1] >> 4) & 15;
        short values[4];
        for(int i = 1; i <= 4; i++)
            values[i - 1] = data.data32[i];
        WindowInfo* winInfo = getWindowInfo(win);
        if(winInfo && allowRequestFromSource(source)) {
            processConfigureRequest(win, values, 0, 0, mask);
        }
    }
    else if(message == ewmh->_NET_WM_MOVERESIZE) {
        // x,y, direction, button, src
        WindowInfo* winInfo = getWindowInfo(win);
        if(winInfo && allowRequestFromSource(data.data32[4])) {
            int dir = data.data32[2];
            bool allowMoveX = XCB_EWMH_WM_MOVERESIZE_SIZE_TOP != dir && XCB_EWMH_WM_MOVERESIZE_SIZE_BOTTOM != dir ;
            bool allowMoveY = XCB_EWMH_WM_MOVERESIZE_SIZE_RIGHT != dir && XCB_EWMH_WM_MOVERESIZE_SIZE_LEFT != dir ;
            bool move = dir == XCB_EWMH_WM_MOVERESIZE_MOVE || dir == XCB_EWMH_WM_MOVERESIZE_MOVE_KEYBOARD;
            if(dir == XCB_EWMH_WM_MOVERESIZE_CANCEL) {
                cancelWindowMoveResize();
            }
            else
                startWindowMoveResize(winInfo, move, allowMoveX, allowMoveY);
        }
    }
    //change window's workspace
    else if(message == ewmh->_NET_WM_DESKTOP) {
        WindowInfo* winInfo = getWindowInfo(win);
        WorkspaceID destIndex = data.data32[0];
        if(!(destIndex == NO_WORKSPACE || destIndex < getNumberOfWorkspaces()))
            destIndex = getNumberOfWorkspaces() - 1;
        if(winInfo && allowRequestFromSource(data.data32[1])) {
            LOG(LOG_LEVEL_DEBUG, "Changing window workspace %d %d\n", destIndex, data.data32[0]);
            winInfo->moveToWorkspace(destIndex);
        }
    }
    else if(message == ewmh->_NET_WM_STATE) {
        // action, prop1, prop2, source
        WindowInfo* winInfo = getWindowInfo(win);
        // if prop2 is 0, then there is only 1 property to consider
        int num = data.data32[2] == 0 ? 1 : 2;
        if(winInfo) {
            if(allowRequestFromSource(data.data32[3])) {
                setWindowStateFromAtomInfo(winInfo, (xcb_atom_t*) &data.data32[1], num, data.data32[0]);
                setXWindowStateFromMask(winInfo);
            }
            else
                INFO("Client message denied");
        }
        else {
            WindowInfo fakeWinInfo = {.id = win};
            loadSavedAtomState(&fakeWinInfo);
            setWindowStateFromAtomInfo(&fakeWinInfo, (xcb_atom_t*) &data.data32[1], num, data.data32[0]);
            setXWindowStateFromMask(&fakeWinInfo);
        }
    }
    else if(message == ewmh->WM_PROTOCOLS) {
        if(data.data32[0] == ewmh->_NET_WM_PING) {
            if(win == root) {
                DEBUG("Pong received");
                WindowInfo* winInfo = getWindowInfo(data.data32[2]);
                if(winInfo) {
                    LOG(LOG_LEVEL_DEBUG, "Updated ping timestamp for window %d\n", winInfo->getID());
                    winInfo->setPingTimeStamp(getTime());
                }
            }
            else if(win == getPrivateWindow()) {
                DEBUG("Pong requested");
                xcb_ewmh_send_client_message(ewmh->connection, root, root, ewmh->WM_PROTOCOLS, sizeof(data), data.data32);
                flush();
            }
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
                    getActiveMaster()->setWorkspaceIndex(newLastIndex);
                for(uint32_t i = data.data32[0]; i < getNumberOfWorkspaces(); i++) {
                    for(WindowInfo* winInfo : getWorkspace(i)->getWindowStack())
                        winInfo->moveToWorkspace(newLastIndex);
                }
                removeWorkspaces(-delta);
            }
            assert(data.data32[0] == getNumberOfWorkspaces());
            assignUnusedMonitorsToWorkspaces();
        }
    }
    /*
    //ignored (we are allowed to)
    case ewmh->_NET_DESKTOP_GEOMETRY:
    case ewmh->_NET_DESKTOP_VIEWPORT:
    */
}

/**
 * Will shift the position of the window in response the change in mouse movements
 * @param winInfo
 */
void moveWindowWithMouse(WindowInfo* winInfo);
/**
 * Will change the size of the window in response the change in mouse movements
 * @param winInfo
 */
void resizeWindowWithMouse(WindowInfo* winInfo);


struct RefWindowMouse {
    WindowID win;
    RectWithBorder ref;
    short mousePos[2];
    bool change[2];
    bool move = 0;
    uint32_t lastSeqNumber = 0;
    RectWithBorder calculateNewPosition(const short newMousePos[2], bool* hasChanged) {
        *hasChanged = 0;
        assert(win);
        RectWithBorder result = RectWithBorder(ref);
        for(int i = 0; i < 2; i++) {
            short delta = newMousePos[i] - mousePos[i];
            if(change[i] && delta) {
                *hasChanged = 1;
                if(move)
                    result[i] += delta;
                else if((signed)(delta + result[2 + i]) < 0) {
                    result[i] += delta + result[i + 2];
                    result[i + 2 ] = -delta - result[i + 2];
                }
                else
                    result[i + 2] += delta;
                if(result[i + 2] == 0)
                    result[i + 2] = 1;
            }
        }
        return result;
    }
    const RectWithBorder getRef() {return ref;}
    bool shouldUpdate() {
        if(getLastDetectedEventSequenceNumber() <= lastSeqNumber) {
            return 0;
        }
        lastSeqNumber = getLastDetectedEventSequenceNumber();
        return 1;
    }
};

static Index<RefWindowMouse> key;
static RefWindowMouse* getRef(Master* m, bool createNew = 1) {
    return get(key, m, createNew);
}
static void removeRef(Master* m) {
    remove(key, m);
}

void startWindowMoveResize(WindowInfo* winInfo, bool move, bool changeX, bool changeY, Master* m) {
    auto ref = getRef(m);
    short pos[2] = {0, 0};
    getMousePosition(m, root, pos);
    *ref = (RefWindowMouse) {winInfo->getID(), getRealGeometry(winInfo->getID()), {pos[0], pos[1]}, {changeX, changeY}, move};
}
void commitWindowMoveResize(Master* m) {
    removeRef(m);
}
void cancelWindowMoveResize(Master* m) {
    auto ref = getRef(m, 0);
    if(ref) {
        RectWithBorder r = ref->getRef();
        processConfigureRequest(ref->win, r, 0, 0,
            XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT);
    }
}

void updateWindowMoveResize(Master* m) {
    auto ref = getRef(m, 0);
    if(ref) {
        if(!ref->shouldUpdate())
            return;
        short pos[2];
        if(getMousePosition(m, root, pos)) {
            bool change = 0;
            RectWithBorder r = ref->calculateNewPosition(pos, &change);
            assert(r.width && r.height);
            if(change)
                processConfigureRequest(ref->win, r, 0, 0,
                    XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT);
        }
    }
}

void updateXWindowStateForAllWindows() {
    static std::unordered_map<WindowID, uint32_t> savedWindowMasks;
    for(WindowInfo* winInfo : getAllWindows()) {
        if(winInfo->hasMask(MAPPABLE_MASK))
            if(!savedWindowMasks.count(winInfo->getID()) ||
                winInfo->getMask() & MASKS_TO_SYNC != savedWindowMasks[winInfo->getID()]) {
                setXWindowStateFromMask(winInfo);
            }
    }
    savedWindowMasks.clear();
    for(WindowInfo* winInfo : getAllWindows())
        if(winInfo->hasMask(MAPPABLE_MASK))
            savedWindowMasks[winInfo->getID()] = winInfo->getMask() & MASKS_TO_SYNC;
}
void loadSavedAtomState(WindowInfo* winInfo) {
    xcb_ewmh_get_atoms_reply_t reply;
    if(xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, winInfo->getID()), &reply, NULL)) {
        setWindowStateFromAtomInfo(winInfo, reply.atoms, reply.atoms_len, XCB_EWMH_WM_STATE_ADD);
        xcb_ewmh_get_atoms_reply_wipe(&reply);
    }
}
void setXWindowStateFromMask(WindowInfo* winInfo) {
    INFO("Setting X State for window " << winInfo->getID() << " from masks " << winInfo->getMask());
    xcb_ewmh_get_atoms_reply_t reply;
    bool hasState = xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, winInfo->getID()), &reply, NULL);
    ArrayList<xcb_atom_t> windowState;
    if(hasState) {
        DEBUG("Current state atoms" << getAtomsAsString(reply.atoms, reply.atoms_len));
        for(uint32_t i = 0; i < reply.atoms_len; i++) {
            if((getMaskFromAtom(reply.atoms[i]) & MASKS_TO_SYNC) == 0)
                windowState.add(reply.atoms[i]);
        }
        xcb_ewmh_get_atoms_reply_wipe(&reply);
    }
    getAtomsFromMask(winInfo->getMask() & MASKS_TO_SYNC, windowState);
    xcb_ewmh_set_wm_state(ewmh, winInfo->getID(), windowState.size(), windowState.data());
    DEBUG("New Atoms" << getAtomsAsString(windowState.data(), windowState.size()));
}

void setWindowStateFromAtomInfo(WindowInfo* winInfo, const xcb_atom_t* atoms, uint32_t numberOfAtoms, int action) {
    INFO("Setting window masks for window " << winInfo->getID() << " current masks " << winInfo->getMask() << " from atoms "
        << getAtomsAsString(atoms, numberOfAtoms) << "Action: " << action);
    WindowMask mask = 0;
    for(unsigned int i = 0; i < numberOfAtoms; i++) {
        mask |= getMaskFromAtom(atoms[i]);
    }
    DEBUG("Pre filtered masks: " << mask);
    mask &= MASKS_TO_SYNC;
    DEBUG("Filtered masks: " << mask);
    if(action == XCB_EWMH_WM_STATE_TOGGLE)
        winInfo->toggleMask(mask);
    else if(action == XCB_EWMH_WM_STATE_REMOVE)
        winInfo->removeMask(mask);
    else
        winInfo->addMask(mask);
}
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
    LOG(LOG_LEVEL_INFO, "Current workspace is %d", currentWorkspace);
    switchToWorkspace(currentWorkspace);
    for(Master* m : getAllMasters())
        m->onWindowFocus(getActiveFocus(m->getKeyboardID()));
}
