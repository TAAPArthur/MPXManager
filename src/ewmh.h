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


void addEWMHRules();

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

static inline void sendActivateWindowRequest(WindowInfo* winInfo) {
    xcb_ewmh_request_change_active_window(ewmh, defaultScreenNumber, winInfo->getID(), source, 0,
                                          getFocusedWindow() ? getFocusedWindow()->getID() : root);
}
static inline void sendChangeWindowStateRequest(WindowInfo* winInfo, xcb_ewmh_wm_state_action_t action,
        xcb_atom_t state, xcb_atom_t state2 = 0) {
    xcb_ewmh_request_change_wm_state(ewmh, defaultScreenNumber, winInfo->getID(), action,
                                     state, state2, source);
}
static inline void sendChangeWindowWorkspaceRequest(WindowInfo* winInfo, WorkspaceID index) {
    xcb_ewmh_request_change_wm_desktop(ewmh, defaultScreenNumber, winInfo->getID(), index, source);
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
static inline void sendCloseWindowRequest(WindowInfo* winInfo) {
    xcb_ewmh_request_close_window(ewmh, defaultScreenNumber, winInfo->getID(), 0, source);
}
static inline void sendWMMoveResizeRequest(WindowInfo* winInfo, short x, short y,
        xcb_ewmh_moveresize_direction_t direction,
        uint8_t button
                                          ) {
    xcb_ewmh_request_wm_moveresize(ewmh, defaultScreenNumber, winInfo->getID(), x, y, direction, (xcb_button_index_t)button,
                                   source);
}
static inline void sendMoveResizeRequest(WindowInfo* winInfo, const short* values, int mask) {
    assert(XCB_EWMH_MOVERESIZE_WINDOW_X == 1 << 8);
    xcb_ewmh_request_moveresize_window(ewmh, defaultScreenNumber, winInfo->getID(),
                                       XCB_GRAVITY_NORTH_WEST, source, (xcb_ewmh_moveresize_window_opt_flags_t)(mask << 8), values[0], values[1], values[2],
                                       values[3]);
}
static inline void sendRestackRequest(WindowInfo* winInfo, xcb_window_t sibling_window, xcb_stack_mode_t detail) {
    xcb_ewmh_request_restack_window(ewmh, defaultScreenNumber, winInfo->getID(), sibling_window, detail);
}

void commitWindowMoveResize(Master* m = getActiveMaster()) ;
void startWindowMoveResize(WindowInfo* winInfo, bool move, bool allowMoveX = 1, bool allowMoveY = 1,
                           Master* m = getActiveMaster());
void cancelWindowMoveResize(Master* m = getActiveMaster());
void updateWindowMoveResize(Master* m) ;
#endif /* EWMH_H_ */
