/**
 * @file
 * Provides macros to set various EWMH properties
 */

#ifndef MPX_EWMH_H_
#define MPX_EWMH_H_

#include "mywm-structs.h"
#include <xcb/xcb_ewmh.h>
#include "xsession.h"
#include "masters.h"
#include "windows.h"

/// List of supported actions
#define SUPPORTED_ACTIONS\
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

/// list of supported states
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
/// List of all supported types
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
 * Note that by default we don't support large desktops
 *
 * We don't currently support _NET_CLIENT_LIST_STACKING because it is a pain to properly implement
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
           SUPPORTED_ACTIONS,\
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
 * Mark the active window and active workspace
 */
void setActiveProperties();


/**
 * Adds a series of rules to be compliant with EWMH/ICCCM spec
 *
 * @param flag
 */
void addEWMHRules(AddFlag flag = ADD_UNIQUE);

/**
 * Reads EWMH desktop property to find the workspace the window should be long.
 * If the field is not present the active workspace is used.
 * If the field is greater than the number of workspaces, the last workspace is used
 * @param win
 * @return the workspace the window should be placed in by default
 */
WorkspaceID getSavedWorkspaceIndex(WindowID win);
/**
 * Sets the EWMH workspace property for this window to its workspace
 *
 * @param winInfo
 */
void setSavedWorkspaceIndex(WindowInfo* winInfo);

/**
 * Moves a window to its saved workspace index
 *
 * @see getSavedWorkspaceIndex
 *
 * @param winInfo
 */
void autoResumeWorkspace(WindowInfo* winInfo);
/**
 * Set all the workspace names to the root window
 */
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

/// the default source when sending a WM request
extern xcb_ewmh_client_source_type_t source;

/**
 * Send ClientMessage requesting to change the _NET_ACTIVE_WINDOW
 *
 * @param wid
 */
static inline void sendActivateWindowRequest(WindowID wid) {
    xcb_ewmh_request_change_active_window(ewmh, defaultScreenNumber, wid, source, 0,
        getFocusedWindow() ? getFocusedWindow()->getID() : root);
}
/**
 * Wrapper for xcb_ewmh_request_change_wm_state
 *
 * @param wid
 * @param action
 * @param state
 * @param state2
 */
static inline void sendChangeWindowStateRequest(WindowID wid, xcb_ewmh_wm_state_action_t action,
    xcb_atom_t state, xcb_atom_t state2 = 0) {
    xcb_ewmh_request_change_wm_state(ewmh, defaultScreenNumber, wid, action,
        state, state2, source);
}
/**
 * wrapper for xcb_ewmh_request_change_wm_desktop
 *
 * @param wid
 * @param index
 */
static inline void sendChangeWindowWorkspaceRequest(WindowID wid, WorkspaceID index) {
    xcb_ewmh_request_change_wm_desktop(ewmh, defaultScreenNumber, wid, index, source);
}
/**
 * Wrapper for xcb_ewmh_request_change_current_desktop
 *
 * @param id
 */
static inline void sendChangeWorkspaceRequest(WorkspaceID id) {
    xcb_ewmh_request_change_current_desktop(ewmh, defaultScreenNumber, id, 0);
}
/**
 *
 *
 * @param num
 */
static inline void sendChangeNumWorkspaceRequestAbs(int num) {
    xcb_ewmh_request_change_number_of_desktops(ewmh, defaultScreenNumber, num);
}
/**
 * wrapper for sendChangeNumWorkspaceRequest
 * @param num
 */
static inline void sendChangeNumWorkspaceRequest(int num) {
    sendChangeNumWorkspaceRequest(getNumberOfWorkspaces() + num);
}
/**
 * wrapper for xcb_ewmh_request_close_window
 * @param wid
 */
static inline void sendCloseWindowRequest(WindowID wid) {
    xcb_ewmh_request_close_window(ewmh, defaultScreenNumber, wid, 0, source);
}
/**
 * Wrapper for sendWMMoveResizeRequest
 * @param wid
 * @param x
 * @param y
 * @param direction
 * @param button
 */
static inline void sendWMMoveResizeRequest(WindowID wid, short x, short y,
    xcb_ewmh_moveresize_direction_t direction,
    uint8_t button
) {
    xcb_ewmh_request_wm_moveresize(ewmh, defaultScreenNumber, wid, x, y, direction, (xcb_button_index_t)button,
        source);
}
/**
 *
 *
 * @param wid
 * @param values
 * @param mask
 */
static inline void sendMoveResizeRequest(WindowID wid, const short* values, int mask) {
    assert(XCB_EWMH_MOVERESIZE_WINDOW_X == 1 << 8);
    xcb_ewmh_request_moveresize_window(ewmh, defaultScreenNumber, wid,
        XCB_GRAVITY_NORTH_WEST, source, (xcb_ewmh_moveresize_window_opt_flags_t)(mask << 8), values[0], values[1], values[2],
        values[3]);
}
/**
 * Wrapper for xcb_ewmh_request_restack_window
 *
 * @param wid
 * @param sibling_window
 * @param detail
 */
static inline void sendRestackRequest(WindowID wid, xcb_window_t sibling_window, xcb_stack_mode_t detail) {
    xcb_ewmh_request_restack_window(ewmh, defaultScreenNumber, wid, sibling_window, detail);
}

/**
 * Starts a window move-resize request
 *
 * The current window position and mouse position is stored. ON subsequent calls
 * to updateWindowMoveResize the window position will be change by the delta in the mouse position.
 *
 * If this method is called twice, the previous stored value is forgotten.
 * This method does not itself affect the window position
 *
 * @param winInfo
 * @param move whether to move (change position) or resize (change width/height)
 * @param allowMoveX allow x position/width to be changed
 * @param allowMoveY allow y position/height to be changed
 * @param m
 */
void startWindowMoveResize(WindowInfo* winInfo, bool move, bool allowMoveX = 1, bool allowMoveY = 1,
    Master* m = getActiveMaster());
/**
 * Update a window-move resize with the new mouse position.
 * If a request has not been started (@see startWindowMoveResize) this method is a no-op
 * The current master position is calculated, and the window is move/resized according to the displacement of the current position and the stored position.
 *
 * If the mouse delta is 0, this is a no-op
 * If the resize would cause dimension to be exactly 0, that dimension would have size 1
 * If the resize make a dimension negative, the window is moved and resized in a standard manner
 * @param m
 */
void updateWindowMoveResize(Master* m = getActiveMaster()) ;
/**
 * Commits the window-move resize
 * Subsequent calls to updateWindowMoveResize or cancelWindowMoveResize won't have any effect
 *
 * @param m
 */
void commitWindowMoveResize(Master* m = getActiveMaster()) ;
/**
 * Cancels the window-move resize operation
 *
 * The window is reverted to its original position/size it had when startWindowMoveResize was called
 *
 * Subsequent calls to updateWindowMoveResize or commitWindowMoveResize won't have any effect
 *
 *
 * @param m
 */
void cancelWindowMoveResize(Master* m = getActiveMaster());
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
