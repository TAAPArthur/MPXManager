/**
 * @file xsession.h
 * @brief Create/destroy XConnection
 */

#ifndef XSESSION_H_
#define XSESSION_H_

#include <xcb/xcb.h>

extern xcb_atom_t WM_TAKE_FOCUS;
extern xcb_atom_t WM_DELETE_WINDOW;

extern xcb_atom_t WM_STATE_NO_TILE;
extern xcb_atom_t WM_STATE_ROOT_FULLSCREEN;

/**
 * Opens a connection to the Xserver
 *
 * This method creates an Xlib display and converts into an
 * xcb_connection_t instance and set the event queue to support xcb.
 * It also sets the close down mode to XCB_CLOSE_DOWN_DESTROY_ALL
 *
 * This method probably should not be called directly.
 * @see connectToXserver
 */
void openXDisplay(void);

/**
 * Closes an open XConnection
 */
void closeConnection(void);
/**
 * exits the application
 */
void quit(void);

/**
 * Flush the X connection
 */
void flush();

#endif /* XSESSION_H_ */
