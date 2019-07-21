/**
 * @file xsession.h
 * @brief Create/destroy XConnection and set global X vars
 */

#ifndef MPX_XSESSION_H_
#define MPX_XSESSION_H_


#include <X11/Xlib.h>
#include <xcb/xcb_ewmh.h>
#include <string>
#include "mywm-structs.h"

extern int defaultScreenNumber;


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
 * Custom atom store in window's state to indicate that it should not be tiled
 */
extern xcb_atom_t WM_STATE_NO_TILE;

/**
 * Custom atom store in window's state to indicate that this window should
 * have the ROOT_FULLSCREEN mask
 */
extern xcb_atom_t WM_STATE_ROOT_FULLSCREEN;

/**
 * Atom for the WM_SELECTION for the default screen
 */
extern xcb_atom_t WM_SELECTION_ATOM;

/**
 * Atom used to differentiate custom client messages
 * Atoms with the flag are used to remotely interact with the WM
 */
extern xcb_atom_t WM_INTERPROCESS_COM;

/// Atom to store an array of the active layout's for each workspace so the state can be restored
extern xcb_atom_t WM_WORKSPACE_LAYOUT_NAMES;
/// Atom to store an array of the paired monitor for each workspace so the state can be restored
extern xcb_atom_t WM_WORKSPACE_MONITORS;
/// Atom to store an array of the layout offset for each workspace so the state can be restored
extern xcb_atom_t WM_WORKSPACE_LAYOUT_INDEXES;
/// Atom to store an array of each window for every workspace so the state can be restored
/// There is a '0' to separate each workspace's window stack
extern xcb_atom_t WM_WORKSPACE_WINDOWS;
/// Atom to store an array of each window for every master so the state can be restored
/// There is a '0' to separate each master's window stack and each stack is preceded with the master id
extern xcb_atom_t WM_MASTER_WINDOWS;
/// Stores the active master so the state can be restored
extern xcb_atom_t WM_ACTIVE_MASTER;

/**XDisplay instance (only used for events/device commands)*/
extern Display* dpy;
/**XCB display instance*/
extern xcb_connection_t* dis;
/**EWMH instance*/
extern xcb_ewmh_connection_t* ewmh;
/**Root window*/
extern WindowID root;
/**Default screen (assumed to be only 1*/
extern xcb_screen_t* screen;

/**
 * Opens a connection to the X Server designated by the DISPLAY env variable
 *
 * It will wait up to 100ms for a connection to become available. If an X Server on this specific display is not present, the program will exit with status 1
 *
 * @see connectToXserver
 */
void openXDisplay(void);

/**
 * Closes an open XConnection
 *
 * If is not safe to call this method on an already closed connection
 */
void closeConnection(void);

/**
 * Flush the X connection
 *
 * This method is just a wrapper for XFlush and xcb_flush
 */
static inline void flush(void) {
    xcb_flush(dis);
    XFlush(dpy);
}

/**
 * Returns the atom value for the given name.
 *
 * If the name is unmapped, then a new atom is created and assigned to the name
 *
 * @param name
 *
 * @return
 */
xcb_atom_t getAtom(const char* name);
static inline xcb_atom_t getAtom(std::string name) {
    return getAtom(name.c_str());
}
/**
 * Returns a name for the given atom and stores the pointer in value
 *
 * Note that an atom can have multiple names and this method just returns one of them
 *
 * @param atom the atom whose name is wanted
 * @param value pointer to a str
 */
std::string getAtomValue(xcb_atom_t atom);
/**
 *
 * @param keysym
 * @return the key code for the given keysym
 */
int getKeyCode(int keysym);

static inline bool isButton(int buttonOrKey) {return buttonOrKey < 8;}
int getButtonDetailOrKeyCode(int buttonOrKey);
/**
 * If it does not already exist creates a window to be used as proof-of-life.
 * If it already exists, previously return the previously returned window
 *
 * The value will change on every new XConnection
 *
 * @return
 */
xcb_window_t getPrivateWindow(void);
/**
 * @see catchError
 */
int catchErrorSilent(xcb_void_cookie_t cookie);
/**
 * Catches an error and log it
 *
 * @param cookie the result of and xcb_.*_checked function
 *
 * @return the error_code if error was found or 0
 * @see logError
 */
int catchError(xcb_void_cookie_t cookie);
/**
 * Prints info related to the error
 *
 * It may trigger an assert; @see CRASH_ON_ERRORS
 *
 * @param e
 */
void logError(xcb_generic_error_t* e, bool xlibError = 0);
/**
 * Stringifies type
 *
 * @param type a regular event type
 *
 * @return the string representation of type if known
 */
const char* eventTypeToString(int type);
/**
 * Stringifies opcode
 * @param opcode a major code from a xcb_generic_error_t object
 * @return the string representation of type if known
 */
const char* opcodeToString(int opcode);
/**
 * Prints the name of each element of atoms
 *
 * @param atoms
 * @param numberOfAtoms
 */
void dumpAtoms(xcb_atom_t* atoms, int numberOfAtoms);
/**
 * @return 1 iff the last event was synthetic (not sent by the XServer)
 */
int isSyntheticEvent();
/**
 * Returns a monotonically increasing counter indicating the number of times the event loop has been idle. Being idle means event loop has nothing to do at the moment which means it has responded to all prior events
*/
int getIdleCount(void);

/**
 * TODO rename
 * Continually listens and responds to event and applying corresponding Rules.
 * This method will only exit when the x connection is lost
 * @param arg unused
 */
void* runEventLoop(void* arg = NULL);
/**
 * To be called when a generic event is received
 * loads info related to the generic event which can be accessed by getLastEvent()
 */
int loadGenericEvent(xcb_ge_generic_event_t* event);
/**
 *Set the last event received.
 * @param event the last event received
 * @see getLastEvent
 */
void setLastEvent(void* event);
/**
 * Retries the last event received
 * @see getLastEvent
 */
void* getLastEvent(void);
#endif /* XSESSION_H_ */
