/**
 * @file xsession.h
 * @brief Create/destroy XConnection
 */

#ifndef XSESSION_H_
#define XSESSION_H_

#include <xcb/xcb.h>

///global graphics context
extern xcb_gcontext_t graphics_context;

/**
 * WM_TAKE_FOCUS atom
 * Used to send client messages to direct focus of an application
*/
extern xcb_atom_t WM_TAKE_FOCUS;

/**
 * WM_DELETE_WINDOW atom
 * Used to send client messages to delete the window
*/
extern xcb_atom_t WM_DELETE_WINDOW;

/**
 *Custom atom store in window's state to indicate that it should not be tield
 */
extern xcb_atom_t WM_STATE_NO_TILE;

/**
 *Custom atom store in window's state to indicate that this window should 
 * have the ROOT_FULLSCREEN mask
 */
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
void flush(void);

#endif /* XSESSION_H_ */
