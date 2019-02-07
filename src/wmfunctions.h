/**
 * @file wmfunctions.h
 * @brief Connect our internal structs to X
 *
 */

#ifndef MYWM_FUNCTIONS_H_
#define MYWM_FUNCTIONS_H_

#include <xcb/xcb.h>

#include "mywm-util.h"


/**
 * Window Manager name used to comply with EWMH
 */
#define WM_NAME "MPX Manger"


/**
 * @param winInfo
 * @return 1 iff the window is PARTIALLY_VISIBLE or VISIBLE
 */
int isWindowVisible(WindowInfo* winInfo);

/**
 *
 * @param winInfo the window whose map state to query
 * @return the window map state either UNMAPPED(0) or MAPPED(1)
 */
int isMappable(WindowInfo* winInfo);

/**
 * Returns true if the user can interact (ie focus,type etc)  with the window
 * For this to be true the window would have to be mapped and not hidden
 */
int isInteractable(WindowInfo* winInfo);

/**
 * Determines if a window should be tiled given its mapState and masks
 * @param winInfo
 * @return true if the window should be tiled
 */
int isTileable(WindowInfo* winInfo);

/**
 * @param winInfo
 * @return 1 iff external resize requests should be granted
 */
int isExternallyResizable(WindowInfo* winInfo);
int isExternallyRaisable(WindowInfo* winInfo);
int isExternallyBorderConfigurable(WindowInfo* winInfo);
/**
 * @param winInfo
 * @return 1 iff external move requests should be granted
 */
int isExternallyMoveable(WindowInfo* winInfo);

/**
 *A window is actiable if it is MAPPABLE and not HIDDEN
 * @param winInfo
 * @return true if the window can receive focus
 */
int isActivatable(WindowInfo* winInfo);

/**
 * Establishes a connection with the X server.
 * Creates and Xlib and xcb display and set global vars like ewmh, root and screen
 *
 * This method creates at least 1 master and monitor such that other mywm functions
 * can be safely used
 */
void connectToXserver();

/**
 * Sync EWMH global properties with our internal state
 */
void syncState();

/**
 * Queries the XServer for all top level windows and process all of them
 * that do not have override redirect
 */
void scan(xcb_window_t baseWindow);

/**
 * Sets the EWMH property to be the given window
 * @param win
 */
void setActiveWindowProperty(int win);


/**
 * Reads EWMH desktop property to find the workspace the window should be long.
 * If the field is not present the active workspace is used.
 * If the field is greater than the number of workspaces, the last workspace is used
 * @param win
 * @return the workspace the window should be placed in by default
 */
int getSavedWorkspaceIndex(xcb_window_t win);


/**
 * Sets the mapped state to UNMAPPED(0) or MAPPED(1)
 * @param id the window whose state is being modified
 * @param state the new state
 * @return  true if the state actuall changed
 */
void updateMapState(int id,int state);

/**
 * Updates our view of the focused window
 * @param winInfo
 * @see onWindowFocus()
 */
void updateFocusState(WindowInfo*winInfo);


/**
 * Sets the border color for the given window
 * @param win   the window in question
 * @param color the new window border color
 * @return 1 iff no error was deteced
 */
int setBorderColor(xcb_window_t win,unsigned int color);

/**
 * Sets the border color of the window to match the active master
 * @param winInfo 
 * @return 1 iff no error was deteced
 */
int setBorder(WindowInfo* winInfo);
/**
 * Sets the border color of the window to match the master that last focused this window or to the default color 
 * @param winInfo 
 * @return 1 iff no error was deteced
 */
int resetBorder(WindowInfo*winInfo);

/**
 * Switch to window's worksspace, raise and focus the window
 * @param winInfo
 * @return the id of the activated window or 0 if no window was activated
 */
int activateWindow(WindowInfo* winInfo);
/**
 * Focuses the given window
 * @param win   the window to focus
 * @return 1 iff no error was detected
 */
int focusWindow(int win);
/**
 * Focus the given window
 * This method is diffrent from focusWindow in that it allows diffrent protocals for 
 * focusing the window based on window masks
 */
int focusWindowInfo(WindowInfo*winInfo);
/**
 * Raises the given window
 * @param win   the window to raise
 * @return 1 iff no error was detected
 */
int raiseWindow(xcb_window_t win);
/**
 * Raise the given window within its onw layer
 */
int raiseWindowInfo(WindowInfo* winInfo);

void raiseLowerWindowInfo(WindowInfo*winInfo,int above);

/**
 * Maps the window specified by id
 * @param id
 * @return if the attempt was successful
 */
int attemptToMapWindow(int id);
/**
 * Removes a window from our records and updates EWMH client list if the window was present
 * @return 1 iff the window was already in our records
 */
int deleteWindow(xcb_window_t winToRemove);



/**
 * Checks to see if the window has SRC* masks set that will allow. If not client requests with such a source will be ignored
 * @param winInfo
 * @param source
 * @return
 */
int allowRequestFromSource(WindowInfo* winInfo,int source);
/**
 * Applies the gravity to pos
 * @param win if gravity is 0 look up the gravity from the ICCCM normal hints on this window
 * @param pos array of values x,y,w,h,border
 * @param gravity the gravity to use
 */
void applyGravity(int win,short pos[5],int gravity);

/**
 * Assigns names to the first n workspaces
 * @param names
 * @param numberOfNames
 */
void setWorkspaceNames(char*names[],int numberOfNames);

/**
 * Returns the first workspace index whose names matches name
 * @param name
 * @return
 */
int getIndexFromName(char*name);

/**
* Returns the name of the workspace at the specifed index
* @param index 
*
* @return the name of the workspace
*/
char*getWorkspaceName(int index);
/**
 * Make the active workspace the one designated by workspaceIndex
 * For the workspace to become active,it must become visible. For this to be true it must 
 * swap monitor with an already visible workspace
 * Upon switching, the focus for all masters is updated to be the last focused window out of 
 * all visible windows
 */
void switchToWorkspace(int workspaceIndex);

/**
 * Moves a window to the workspace given by destIndex at the NORMAL_LAYER
 */
void moveWindowToWorkspace(WindowInfo* winInfo,int destIndex);

/**
 * Filters the configure request to allowed actions then configures the window
 * @param win
 * @param values
 * @param sibling 
 * @param stackMode
 * @param configMask
 * @see xcb_configure_window
 */
void processConfigureRequest(int win,short values[5],xcb_window_t sibling,int stackMode,int configMask);

/**
 * Do everything needed to comply when EWMH.
 * Creates a functionless window with set all the Atoms we support, clear any
 * unsporrted values from the root window
 */
void broadcastEWMHCompilence();

/**
 * Moves a window to the specifed layer in the same workspace
 * @param win
 * @param layer
 */
void moveWindowToLayer(WindowInfo* win,int layer);

/**
 * Send a kill signal to the client with the window
 * @param win
 */
void killWindow(xcb_window_t win);

/**
 * Sends a WM_DELETE_WINDOW message or sends a kill requests
 * @param winInfo
 * @see killWindow();
 */
void killWindowInfo(WindowInfo* winInfo);
/**
 * Check to see if we are showing the desktop.
 * When showing the desktop all windows in the workspace are hidden except for windows in the Desktop Layer
 * @param index
 * @return 1 if we are currently showing the desktop
 */
int isShowingDesktop(int index);
/**
 * show or hide desktop depending on values
 * @param index workpsace index
 * @param value 1 or 0
 */
void setShowingDesktop(int index,int value);
/**
 * Toggle showing of the desktop
 * @see isShowingDesktop()
 */
void toggleShowDesktop();

void updateEWMHClientList(void);

#endif
