#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

#include "arraylist.h"
#include "ext.h"
#include "bindings.h"
#include "devices.h"
#include "ewmh.h"
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
    LOG(LOG_LEVEL_DEBUG, "Complying with EWMH\n");
    //functionless window required by EWMH spec
    //we set its class to input only and set override redirect so we (and anyone else  ignore it)
    if(!STEAL_WM_SELECTION) {
        xcb_get_selection_owner_reply_t* ownerReply = xcb_get_selection_owner_reply(dis, xcb_get_selection_owner(dis,
                WM_SELECTION_ATOM), NULL);
        if(ownerReply->owner) {
            LOG(LOG_LEVEL_ERROR, "Selection %d is already owned by window %d\n", WM_SELECTION_ATOM, ownerReply->owner);
            quit(0);
        }
        free(ownerReply);
    }
    LOG(LOG_LEVEL_DEBUG, "Setting selection owner\n");
    if(catchError(xcb_set_selection_owner_checked(dis, getPrivateWindow(), WM_SELECTION_ATOM,
                  XCB_CURRENT_TIME)) == 0) {
        unsigned int data[5] = {XCB_CURRENT_TIME, WM_SELECTION_ATOM, getPrivateWindow()};
        xcb_ewmh_send_client_message(dis, root, root, WM_SELECTION_ATOM, 5, data);
    }
    SET_SUPPORTED_OPERATIONS(ewmh);
    xcb_ewmh_set_wm_pid(ewmh, root, getpid());
    xcb_ewmh_set_supporting_wm_check(ewmh, root, getPrivateWindow());
    setWindowTitle(getPrivateWindow(), WINDOW_MANAGER_NAME);
    LOG(LOG_LEVEL_DEBUG, "Complied with EWMH/ICCCM specs\n");
}

void updateEWMHClientList() {
    xcb_ewmh_set_client_list(ewmh, defaultScreenNumber, mappedOrder.size(), (WindowID*)mappedOrder.data());
}
void setActiveWindowProperty(WindowID win) {
    xcb_ewmh_set_active_window(ewmh, defaultScreenNumber, win);
}
void setActiveWorkspaceProperty(WorkspaceID index) {
    xcb_ewmh_set_current_desktop_checked(ewmh, DefaultScreen(dpy), index);
}
void setActiveProperties() {
    WindowInfo* winInfo = getActiveMaster()->getFocusedWindow();
    setActiveWindowProperty(winInfo ? winInfo->getID() : root);
    setActiveWorkspaceProperty(getActiveWorkspaceIndex());
}
static void updateEWMHWorkspaceProperties() {
    xcb_ewmh_coordinates_t viewPorts[getNumberOfWorkspaces()] = {0};
    xcb_ewmh_geometry_t workAreas[getNumberOfWorkspaces()] = {0};
    int i = 0;
    for(Workspace* w : getAllWorkspaces()) {
        if(w->isVisible())
            w->getMonitor()->getViewport().copyTo(((int*)&workAreas[i]));
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
    const char* names[getNumberOfWorkspaces()];
    int i = 0;
    for(Workspace* w : getAllWorkspaces())
        names[i++] = w->getName().c_str();
    if(ewmh && RUN_AS_WM)
        xcb_ewmh_set_desktop_names(ewmh, defaultScreenNumber, getNumberOfWorkspaces(), (char*)names);
}
void addEWMHRules(AddFlag flag) {
    getEventRules(XCB_CLIENT_MESSAGE).add(DEFAULT_EVENT(onClientMessage), flag);
    getEventRules(ClientMapAllow).add({+[](WindowInfo * winInfo) {mappedOrder.add(winInfo->getID());}, "_recordWindow"},
    flag);
    getEventRules(UnregisteringWindow).add({+[](WindowInfo * winInfo) {mappedOrder.removeElement(winInfo->getID());}, "_unrecordWindow"},
    flag);
    getEventRules(PostRegisterWindow).add(DEFAULT_EVENT(updateEWMHClientList), flag);
    getEventRules(PostRegisterWindow).add(DEFAULT_EVENT(autoResumeWorkspace), flag);
    getEventRules(UnregisteringWindow).add(DEFAULT_EVENT(updateEWMHClientList), flag);
    getEventRules(onXConnection).add(DEFAULT_EVENT(broadcastEWMHCompilence), flag);
    getEventRules(WindowWorkspaceMove).add(DEFAULT_EVENT(setSavedWorkspaceIndex), flag);
    getEventRules(TrueIdle).add(DEFAULT_EVENT(setActiveProperties), flag);
    getBatchEventRules(onScreenChange).add(DEFAULT_EVENT(updateEWMHWorkspaceProperties), flag);
    getBatchEventRules(PostRegisterWindow).add(DEFAULT_EVENT(updateEWMHWorkspaceProperties), flag);
    getEventRules(onXConnection).add(DEFAULT_EVENT(syncState));
    getEventRules(PostRegisterWindow).add(DEFAULT_EVENT(loadSavedAtomState));
}
WorkspaceID getSavedWorkspaceIndex(WindowID win) {
    WorkspaceID workspaceIndex = 0;
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
    if(winInfo->getWorkspaceIndex() == NO_WORKSPACE && !winInfo->isDock()) {
        WorkspaceID w = getSavedWorkspaceIndex(winInfo->getID());
        LOG(LOG_LEVEL_DEBUG, "Moving %d to workspace %d\n", winInfo->getID(), w);
        winInfo->moveToWorkspace(w);
    }
}

void setShowingDesktop(int value) {
    LOG(LOG_LEVEL_INFO, "setting showing desktop %d\n", value);
    for(WindowInfo* winInfo : getAllWindows())
        if(winInfo->isInteractable() && winInfo->getType() == ewmh->_NET_WM_WINDOW_TYPE_DESKTOP)
            raiseWindowInfo(winInfo, value);
    xcb_ewmh_set_showing_desktop(ewmh, defaultScreenNumber, value);
}

bool isShowingDesktop(void) {
    unsigned int value = 0;
    xcb_ewmh_get_showing_desktop_reply(ewmh, xcb_ewmh_get_showing_desktop(ewmh, defaultScreenNumber), &value, NULL);
    return value;
}

void onClientMessage(void) {
    xcb_client_message_event_t* event = (xcb_client_message_event_t*)getLastEvent();
    xcb_client_message_data_t data = event->data;
    xcb_window_t win = event->window;
    Atom message = event->type;
    LOG(LOG_LEVEL_DEBUG, "Received client message/request for window: %d\n", win);
    if(message == ewmh->_NET_CURRENT_DESKTOP)
        switchToWorkspace(data.data32[0]);
    else if(message == ewmh->_NET_ACTIVE_WINDOW) {
        //data: source,timestamp,current active window
        WindowInfo* winInfo = getWindowInfo(win);
        if(winInfo->allowRequestFromSource(data.data32[0])) {
            Master* m = data.data32[3] ? getMasterById(data.data32[3]) : getClientMaster(win);
            if(m)
                setActiveMaster(m);
            LOG(LOG_LEVEL_DEBUG, "Activating window %d for master %d due to client request\n", win, getActiveMaster()->getID());
            activateWindow(winInfo);
        }
    }
    else if(message == ewmh->_NET_SHOWING_DESKTOP) {
        LOG(LOG_LEVEL_DEBUG, "Changing showing desktop to %d\n\n", data.data32[0]);
        setShowingDesktop(data.data32[0]);
    }
    else if(message == ewmh->_NET_CLOSE_WINDOW) {
        LOG(LOG_LEVEL_DEBUG, "Killing window %d\n\n", win);
        WindowInfo* winInfo = getWindowInfo(win);
        if(!winInfo)
            killClientOfWindow(win);
        else if(winInfo->allowRequestFromSource(data.data32[0])) {
            killClientOfWindowInfo(winInfo);
        }
    }
    else if(message == ewmh->_NET_RESTACK_WINDOW) {
        WindowInfo* winInfo = getWindowInfo(win);
        LOG(LOG_LEVEL_TRACE, "Restacking Window %d sibling %d detail %d\n\n", win, data.data32[1], data.data32[2]);
        if(winInfo->allowRequestFromSource(data.data32[0]))
            processConfigureRequest(win, NULL, data.data32[1], data.data32[2],
                                    XCB_CONFIG_WINDOW_STACK_MODE | XCB_CONFIG_WINDOW_SIBLING);
    }
    else if(message == ewmh->_NET_REQUEST_FRAME_EXTENTS) {
        xcb_ewmh_set_frame_extents(ewmh, win, DEFAULT_BORDER_WIDTH, DEFAULT_BORDER_WIDTH, DEFAULT_BORDER_WIDTH,
                                   DEFAULT_BORDER_WIDTH);
    }
    else if(message == ewmh->_NET_MOVERESIZE_WINDOW) {
        LOG(LOG_LEVEL_DEBUG, "Move/Resize window request %d\n\n", win);
        int mask = data.data8[1] & 15;
        int source = (data.data8[1] >> 4) & 15;
        short values[4];
        for(int i = 1; i <= 4; i++)
            values[i - 1] = data.data32[i];
        WindowInfo* winInfo = getWindowInfo(win);
        if(winInfo->allowRequestFromSource(source)) {
            processConfigureRequest(win, values, 0, 0, mask);
        }
    }
    else if(message == ewmh->_NET_WM_MOVERESIZE) {
        // x,y, direction, button, src
        WindowInfo* winInfo = getWindowInfo(win);
        if(winInfo && winInfo->allowRequestFromSource(data.data32[4])) {
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
        if(winInfo && winInfo->allowRequestFromSource(data.data32[1])) {
            LOG(LOG_LEVEL_DEBUG, "Changing window workspace %d %d\n\n", destIndex, data.data32[0]);
            winInfo->moveToWorkspace(destIndex);
        }
    }
    else if(message == ewmh->_NET_WM_STATE) {
        LOG(LOG_LEVEL_DEBUG, "Settings client window manager state %d\n", data.data32[3]);
        WindowInfo* winInfo = getWindowInfo(win);
        int num = data.data32[2] == 0 ? 1 : 2;
        if(winInfo) {
            if(winInfo->allowRequestFromSource(data.data32[3]))
                setWindowStateFromAtomInfo(winInfo, (xcb_atom_t*) &data.data32[1], num, data.data32[0]);
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
                LOG(LOG_LEVEL_DEBUG, "Pong received\n");
                WindowInfo* winInfo = getWindowInfo(data.data32[2]);
                if(winInfo) {
                    LOG(LOG_LEVEL_DEBUG, "Updated ping timestamp for window %d\n\n", winInfo->getID());
                    winInfo->setPingTimeStamp(getTime());
                }
            }
            else if(win == getPrivateWindow()) {
                LOG(LOG_LEVEL_DEBUG, "Pong requested\n");
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
    bool move;
    RectWithBorder calculateNewPosition(const short newMousePos[2]) {
        assert(win);
        RectWithBorder result = RectWithBorder(ref);
        for(int i = 0; i < 2; i++) {
            short delta = newMousePos[i] - mousePos[i];
            if(change[i]) {
                if(move)
                    result[i] += delta;
                else if((signed)(delta + result[2 + i]) < 0) {
                    result[i] += delta + result[i + 2];
                    result[i + 2 ] = -delta - result[i + 2];
                }
                else
                    result[i + 2] += delta;
            }
        }
        LOG_RUN(LOG_LEVEL_TRACE, std::cout << "WindowMoveResizeResult: " << result << "\n");
        return result;
    }
    const RectWithBorder getRef() {return ref;}
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
        short pos[2];
        if(getMousePosition(m, root, pos)) {
            RectWithBorder r = ref->calculateNewPosition(pos);
            if(r.width && r.height)
                processConfigureRequest(ref->win, r, 0, 0,
                                        XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT);
        }
    }
}

void loadSavedAtomState(WindowInfo* winInfo) {
    xcb_ewmh_get_atoms_reply_t reply;
    if(xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, winInfo->getID()), &reply, NULL)) {
        setWindowStateFromAtomInfo(winInfo, reply.atoms, reply.atoms_len, XCB_EWMH_WM_STATE_ADD);
        xcb_ewmh_get_atoms_reply_wipe(&reply);
    }
}
void setXWindowStateFromMask(WindowInfo* winInfo) {
    xcb_atom_t supportedStates[] = {SUPPORTED_STATES};
    xcb_ewmh_get_atoms_reply_t reply;
    int count = 0;
    bool hasState = xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, winInfo->getID()), &reply, NULL);
    xcb_atom_t windowState[LEN(supportedStates) + (hasState ? reply.atoms_len : 0)];
    if(hasState) {
        for(uint32_t i = 0; i < reply.atoms_len; i++) {
            char isSupportedState = 0;
            for(uint32_t n = 0; n < LEN(supportedStates); n++)
                if(supportedStates[n] == reply.atoms[i]) {
                    isSupportedState = 1;
                    break;
                }
            if(!isSupportedState)
                windowState[count++] = reply.atoms[i];
        }
        xcb_ewmh_get_atoms_reply_wipe(&reply);
    }
    count += getAtomsFromMask(winInfo->getMask(), windowState + count);
    xcb_ewmh_set_wm_state(ewmh, winInfo->getID(), count, windowState);
}

void setWindowStateFromAtomInfo(WindowInfo* winInfo, const xcb_atom_t* atoms, uint32_t numberOfAtoms, int action) {
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
