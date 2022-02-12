/**
 * @file
 * Methods to load client supplied window info into our WindowInfo
 */
#ifndef MPX_WINDOW_PROPERTIES
#define MPX_WINDOW_PROPERTIES
#include <stdbool.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>

#include "../masters.h"
#include "../mywm-structs.h"
#include "../util/rect.h"

/**
 * Sets the win to be a transient for transientTo
 *
 * @param win
 * @param transientTo
 */
void setWindowTransientFor(WindowID win, WindowID transientTo);
/**
 * Sets the ICCC class and instance (resource) for the window
 *
 * @param win
 * @param className
 * @param instanceName
 */
void setWindowClass(WindowID win, const char* className, const char* instanceName);
/**
 * Sets the window title
 *
 * @param win
 * @param title
 */
void setWindowTitle(WindowID win, const char* title);
/**
 * Sets the window type in order of preference
 *
 * @param win
 * @param atoms
 * @param num
 */
void setWindowTypes(WindowID win, xcb_atom_t* atoms, int num);
/**
 * Sets a single window type
 *
 * @param win
 * @param atom
 */
static inline void setWindowType(WindowID win, xcb_atom_t atom) {setWindowTypes(win, &atom, 1);}

/**
 * Loads class and instance name for the given window
 * @param win
 * @param className
 * @param instanceName
 *
 * @return 1 iff the class info was successfully loaded
 */
bool getClassInfo(WindowID win, char* className, char* instanceName);

/**
 * Returns the name of the window in a str that needs to be freeded
 *
 * @param win
 *
 * @return
 */
bool getWindowTitle(WindowID win, char* title);

xcb_atom_t getWindowType(WindowID win);
/**
 * Sets WM_WINDOW_ROLE to role on wid
 *
 * @param wid
 * @param role
 */
void setWindowRole(WindowID wid, const char* role);

/**
 * Sets the border color for the given window
 * @param win the window in question
 * @param color the new window border color
 */
void setBorderColor(WindowID win, unsigned int color);

/**
 * Sets the border color of the window to match the active master
 * @param win
 */
void setBorder(WindowID win);
/**
 * Sets the border color of the window to match the master that last focused this window
 * (not counting the current active master) or to the default color
 * @param win
 */
void resetBorder(WindowID win);

/**
 * Assigns names to the first n workspaces
 * @param names
 * @param numberOfNames
 */
void setWorkspaceNames(char* names[], int numberOfNames);

/**
 * Focuses the given window
 * @param win the window to focus
 * @param master the master who gets the focus
 * @return 1 iff no error was detected
 */
int focusWindowAsMaster(WindowID win, Master* master);
static inline int focusWindow(WindowID win) {return focusWindowAsMaster(win, getActiveMaster());}
/**
 * Focus the given window
 * This method is different from focusWindow in that it allows different protocols for
 * focusing the window based on window masks
 */
int focusWindowInfoAsMaster(WindowInfo* winInfo, Master* master);
static inline int focusWindowInfo(WindowInfo* winInfo) {return focusWindowInfoAsMaster(winInfo, getActiveMaster());}

/**
 * Returns the cached window size hints
 *
 * These are cached on calls to load properties
 *
 * @param winInfo
 *
 * @return
 */
xcb_size_hints_t* getWindowSizeHints(WindowInfo* winInfo);
/**
 * Loads normal hints
 * Loads grouptID, input and window state for a given window
 * @param winInfo
 * @see loadWindowProperties
 */
void loadWindowHints(WindowInfo* winInfo);

/**
 * @param id
 * @return the live window geometry
 */
Rect getRealGeometry(WindowID id) ;

uint16_t getWindowBorder(WindowID id);

/**
 * @param win
 *
 * @return the _NET_USER_TIME property of win or 0 if unset
 */
uint32_t getUserTime(WindowID win);
/**
 * Sets the _NET_USER_TIME property on win to time
 *
 * @param win
 * @param time
 */
void setUserTime(WindowID win, uint32_t time);


/**
 * Send a kill signal to the client with the window
 * @param win
 * @return 0 on success or the error
 */
int killClientOfWindow(WindowID win);

/**
 * Sends a WM_DELETE_WINDOW message or sends a kill requests
 * @param winInfo
 * @see killWindow();
 */
void killClientOfWindowInfo(WindowInfo* winInfo);

#endif
