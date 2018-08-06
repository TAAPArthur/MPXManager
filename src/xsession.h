/**
 * @file xsession.h
 * @brief Create/destroy XConnection
 */

#ifndef XSESSION_H_
#define XSESSION_H_

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
void openXDisplay();

/**
 * Closes an open XConnection
 */
void closeConnection();
/**
 * exits the application
 */
void quit();



#endif /* XSESSION_H_ */
