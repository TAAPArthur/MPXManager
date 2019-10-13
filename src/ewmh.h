/*
 * @file ewmh.h
 * Provides macros to set various EWMH properties
 */

#ifndef MPX_EWMH_H_
#define MPX_EWMH_H_

#include "mywm-structs.h"
#include <xcb/xcb_ewmh.h>
#include "xsession.h"
#include "masters.h"
#include "windows.h"

#define ALLOWED_ACTIONS\
    ewmh->_NET_WM_ACTION_ABOVE, \
    ewmh->_NET_WM_ACTION_BELOW,\
    ewmh->_NET_WM_ACTION_CHANGE_DESKTOP,\
    ewmh->_NET_WM_ACTION_CLOSE,\
    ewmh->_NET_WM_ACTION_FULLSCREEN,\
    ewmh->_NET_WM_ACTION_MAXIMIZE_HORZ,\
    ewmh->_NET_WM_ACTION_MAXIMIZE_VERT,\
    ewmh->_NET_WM_ACTION_MINIMIZE,\
    ewmh->_NET_WM_ACTION_STICK,\
    ewmh->_NET_WM_ACTION_MOVE,\
    ewmh->_NET_WM_ACTION_RESIZE,\
    ewmh->_NET_WM_ALLOWED_ACTIONS

#define SET_ALLOWED_ACTIONS(ewmh){\
    xcb_atom_t actions[] = {ALLOWED_ACTIONS};\
    xcb_ewmh_set_wm_allowed_actions_checked(ewmh, win, LEN(actions), list);\
    }

#define SUPPORTED_STATES\
       ewmh->_NET_WM_STATE_ABOVE,\
       ewmh->_NET_WM_STATE_BELOW,\
       ewmh->_NET_WM_STATE_DEMANDS_ATTENTION,\
       ewmh->_NET_WM_STATE_FULLSCREEN,\
       ewmh->_NET_WM_STATE_HIDDEN,\
       ewmh->_NET_WM_STATE_MAXIMIZED_HORZ,\
       ewmh->_NET_WM_STATE_MAXIMIZED_VERT,\
       ewmh->_NET_WM_STATE_STICKY,\
       WM_STATE_ROOT_FULLSCREEN,\
       WM_STATE_NO_TILE,\

#define SUPPORTED_TYPES \
       ewmh->_NET_WM_WINDOW_TYPE_NORMAL,\
       ewmh->_NET_WM_WINDOW_TYPE_DOCK,\
       ewmh->_NET_WM_WINDOW_TYPE_DIALOG,\
       ewmh->_NET_WM_WINDOW_TYPE_UTILITY,\
       ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR,\
       ewmh->_NET_WM_WINDOW_TYPE_SPLASH,\
       ewmh->_NET_WM_WINDOW_TYPE_MENU,\
       ewmh->_NET_WM_WINDOW_TYPE_DROPDOWN_MENU,\
       ewmh->_NET_WM_WINDOW_TYPE_POPUP_MENU,\
       ewmh->_NET_WM_WINDOW_TYPE_NOTIFICATION,
/**
 * Sets all the ewmh we support.
 *
 * We don't have true support for desktop geometry like atoms; they don't align with out multi monitor view
 * _NET_DESKTOP_GEOMETRY,_NET_DESKTOP_VIEWPORT,_NET_WORKAREA
 * We don't currently support _NET_WM_MOVERESIZE, _NET_CLIENT_LIST_STACKING\
 */
#define SET_SUPPORTED_OPERATIONS(ewmh){\
    xcb_atom_t net_atoms[] = { \
           ewmh->_NET_SUPPORTED,\
           ewmh->_NET_SUPPORTING_WM_CHECK,\
           ewmh->_NET_WM_NAME,\
           \
           ewmh->_NET_CLIENT_LIST,\
           \
           ewmh->_NET_WM_STRUT, ewmh->_NET_WM_STRUT_PARTIAL,\
           ewmh->_NET_WM_STATE,\
               SUPPORTED_STATES\
           ewmh->_NET_WM_WINDOW_TYPE,\
               SUPPORTED_TYPES\
           ewmh->_NET_ACTIVE_WINDOW,ewmh->_NET_CLOSE_WINDOW,\
           ewmh->_NET_WM_DESKTOP,\
               ewmh->_NET_NUMBER_OF_DESKTOPS,\
               ewmh->_NET_CURRENT_DESKTOP,\
                ewmh->_NET_SHOWING_DESKTOP,\
               ewmh->_NET_DESKTOP_NAMES,\
               ewmh->_NET_DESKTOP_VIEWPORT,\
           ALLOWED_ACTIONS,\
           ewmh->_NET_DESKTOP_GEOMETRY,ewmh->_NET_DESKTOP_VIEWPORT,ewmh->_NET_WORKAREA\
           };\
           xcb_ewmh_set_supported_checked(ewmh, defaultScreenNumber,\
            sizeof(net_atoms) / sizeof(xcb_atom_t) , net_atoms);\
        }


/**
 *
 *
 * @return 1 iff there is a running instance of MPXManager and it is acting like a window manger (RUN_AS_WM==1)
 */
bool isMPXManagerRunning(void);

/**
 * Do everything needed to comply when EWMH.
 * Creates a functionless window with set all the Atoms we support, clear any
 * unsupported values from the root window
 */
void broadcastEWMHCompilence();

/**
 * Updates the root window with a list of windows we are managing
 */
void updateEWMHClientList();
/**
 * Sets the EWMH property to be the given window
 * @param win
 */
void setActiveWindowProperty(WindowID win);
void setActiveWorkspaceProperty(WorkspaceID index);


void addEWMHRules(AddFlag flag = ADD_UNIQUE);

/**
 * Reads EWMH desktop property to find the workspace the window should be long.
 * If the field is not present the active workspace is used.
 * If the field is greater than the number of workspaces, the last workspace is used
 * @param win
 * @return the workspace the window should be placed in by default
 */
WorkspaceID getSavedWorkspaceIndex(WindowID win);
void setSavedWorkspaceIndex(WindowInfo* winInfo);

void autoResumeWorkspace(WindowInfo* winInfo);
void updateWorkspaceNames();

/**
 * Check to see if we are showing the desktop.
 * @return 1 if we are currently showing the desktop
 */
bool isShowingDesktop(void);
/**
 * show or hide desktop depending on values
 * @param value 1 or 0
 */
void setShowingDesktop(int value);

/**
 * Responds to client messages according the ICCCM/EWMH spec
 */
void onClientMessage(void);

extern xcb_ewmh_client_source_type_t source;

static inline void sendActivateWindowRequest(WindowID wid) {
    xcb_ewmh_request_change_active_window(ewmh, defaultScreenNumber, wid, source, 0,
                                          getFocusedWindow() ? getFocusedWindow()->getID() : root);
}
static inline void sendChangeWindowStateRequest(WindowID wid, xcb_ewmh_wm_state_action_t action,
        xcb_atom_t state, xcb_atom_t state2 = 0) {
    xcb_ewmh_request_change_wm_state(ewmh, defaultScreenNumber, wid, action,
                                     state, state2, source);
}
static inline void sendChangeWindowWorkspaceRequest(WindowID wid, WorkspaceID index) {
    xcb_ewmh_request_change_wm_desktop(ewmh, defaultScreenNumber, wid, index, source);
}
static inline void sendChangeWorkspaceRequest(WorkspaceID id) {
    xcb_ewmh_request_change_current_desktop(ewmh, defaultScreenNumber, id, 0);
}
static inline void sendChangeNumWorkspaceRequestAbs(int num) {
    xcb_ewmh_request_change_number_of_desktops(ewmh, defaultScreenNumber, num);
}
static inline void sendChangeNumWorkspaceRequest(int num) {
    sendChangeNumWorkspaceRequest(getNumberOfWorkspaces() + num);
}
static inline void sendCloseWindowRequest(WindowID wid) {
    xcb_ewmh_request_close_window(ewmh, defaultScreenNumber, wid, 0, source);
}
static inline void sendWMMoveResizeRequest(WindowID wid, short x, short y,
        xcb_ewmh_moveresize_direction_t direction,
        uint8_t button
                                          ) {
    xcb_ewmh_request_wm_moveresize(ewmh, defaultScreenNumber, wid, x, y, direction, (xcb_button_index_t)button,
                                   source);
}
static inline void sendMoveResizeRequest(WindowID wid, const short* values, int mask) {
    assert(XCB_EWMH_MOVERESIZE_WINDOW_X == 1 << 8);
    xcb_ewmh_request_moveresize_window(ewmh, defaultScreenNumber, wid,
                                       XCB_GRAVITY_NORTH_WEST, source, (xcb_ewmh_moveresize_window_opt_flags_t)(mask << 8), values[0], values[1], values[2],
                                       values[3]);
}
static inline void sendRestackRequest(WindowID wid, xcb_window_t sibling_window, xcb_stack_mode_t detail) {
    xcb_ewmh_request_restack_window(ewmh, defaultScreenNumber, wid, sibling_window, detail);
}

void commitWindowMoveResize(Master* m = getActiveMaster()) ;
void startWindowMoveResize(WindowInfo* winInfo, bool move, bool allowMoveX = 1, bool allowMoveY = 1,
                           Master* m = getActiveMaster());
void cancelWindowMoveResize(Master* m = getActiveMaster());
void updateWindowMoveResize(Master* m) ;
/**
 * Sets the WM_STATE from the window masks
 */
void setXWindowStateFromMask(WindowInfo* winInfo);
/**
 * Reads the WM_STATE fields from the given window and sets the window mask to be consistent with the state
 * If the WM_STATE cannot be read, then nothing is done
 * @see setWindowStateFromAtomInfo
 */
void loadSavedAtomState(WindowInfo* winInfo);

/**
 * Sets the window state based on a list of atoms
 * @param winInfo the window whose mask will be modified
 * @param atoms the list of atoms to add,remove or toggle depending on ACTION
 * @param numberOfAtoms the number of atoms
 * @param action whether to add,remove or toggle atoms. For toggle, if all of the corresponding masks for the list of atoms is set, then they all will be removed else they all will be added
 * @see XCB_EWMH_WM_STATE_ADD, XCB_EWMH_WM_STATE_REMOVE, XCB_EWMH_WM_STATE_TOGGLE
 */
void setWindowStateFromAtomInfo(WindowInfo* winInfo, const xcb_atom_t* atoms, uint32_t numberOfAtoms, int action);
/**
 * Sync EWMH global properties with our internal state
 */
void syncState();
#endif /* EWMH_H_ */
