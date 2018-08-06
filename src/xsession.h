/**
 * @file xsession.h
 * @brief Create/destroy XConnection
 */

#ifndef XSESSION_H_
#define XSESSION_H_


/**
 * Closes an open XConnection
 */
void closeConnection();
/**
 * exits the application
 */
void quit();

/**
 * Establishes a connection with the X server.
 * Creates and Xlib and xcb display and set global vars like ewmh, root and screen
 *
 * This method creates at least 1 master and monitor such that other mywm functions
 * can be safely used
 */
void connectToXserver();

/**
 * Query for all monitors
 */
void detectMonitors();

#endif /* XSESSION_H_ */
