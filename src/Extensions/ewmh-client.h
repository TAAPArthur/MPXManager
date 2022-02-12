#ifndef MPX_EWMH_CLIENT_H_
#define MPX_EWMH_CLIENT_H_
#include "../mywm-structs.h"
#include "../windows.h"
#include "../xutil/xsession.h"
#include "ewmh.h"
#include <xcb/xcb_ewmh.h>

/**
 * Send ClientMessage requesting to change the _NET_ACTIVE_WINDOW
 *
 * @param wid
 */
static inline void sendActivateWindowRequest(WindowID wid) {
    xcb_ewmh_request_change_active_window(ewmh, defaultScreenNumber, wid, source, 0,
        getFocusedWindow() ? getFocusedWindow()->id : root);
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
    xcb_atom_t state, xcb_atom_t state2) {
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
#endif

