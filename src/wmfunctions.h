/**
 * @file wmfunctions.h
 * @brief Provides methods to help sync our internal structs to X11
 */

#ifndef MYWM_FUNCTIONS_H_
#define MYWM_FUNCTIONS_H_

#include <xcb/xcb.h>

#include "mywm-structs.h"


/**
 * Window Manager name used to comply with EWMH
 */
#define WM_NAME "MPX Manger"


/**
 *
 *
 * @return 1 iff there is a running instance of MPXManager and it is acting like a window manger (RUN_AS_WM==1)
 */
bool isMPXManagerRunning(void);

/**
 * Establishes a connection with the X server.
 * Creates and Xlib and xcb display and set global vars like ewmh, root and screen
 *
 * This method creates at least 1 master and monitor which is a prerequisite to use most other functions
 * can be safely used
 */
void connectToXserver();



/**
 * Sets the EWMH property to be the given window
 * @param win
 */
void setActiveWindowProperty(WindowID win);


/**
 * Reads EWMH desktop property to find the workspace the window should be long.
 * If the field is not present the active workspace is used.
 * If the field is greater than the number of workspaces, the last workspace is used
 * @param win
 * @return the workspace the window should be placed in by default
 */
int getSavedWorkspaceIndex(WindowID win);


/**
 * Sets the mapped state to UNMAPPED(0) or MAPPED(1)
 * @param id the window whose state is being modified
 * @param state the new state
 * @return  true if the state actually changed
 */
void updateMapState(int id, int state);

/**
 * Updates our view of the focused window
 * @param winInfo
 * @see onWindowFocus()
 */
void updateFocusState(WindowInfo* winInfo);


/**
 * Sets the border color for the given window
 * @param win   the window in question
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
 * Sets the border width to 0
 *
 * @param winInfo
 */
void removeBorder(WindowInfo* winInfo);

/**
 * Switch to window's workspace, raise and focus the window
 * @param winInfo
 * @return the id of the activated window or 0 if no window was activated
 */
int activateWindow(WindowInfo* winInfo);
/**
 * Focuses the given window
 * @param win   the window to focus
 * @return 1 iff no error was detected
 */
int focusWindow(WindowID win);
/**
 * Focus the given window
 * This method is different from focusWindow in that it allows different protocols for
 * focusing the window based on window masks
 */
int focusWindowInfo(WindowInfo* winInfo);
/**
 * Raise the given window and any window that is is transient for it
 * @param winInfo
 * @return true on success
 */
int raiseWindowInfo(WindowInfo* winInfo);
/**
 * Lower the given window
 * @param winInfo
 * @return true on success
 */
int lowerWindowInfo(WindowInfo* winInfo);

/**
 * Raises or lowers the winInfo depending on raise.
 * If the transientFor property is set, then the window is raised lowered relative to is tansientFor window
 * @param winInfo
 * @param raise
 */
int raiseLowerWindowInfo(WindowInfo* winInfo, int raise);

/**
 * Maps the window specified by id
 * @param id
 * @return if the attempt was successful
 */
int attemptToMapWindow(int id);
/**
 * Removes a window from our records and updates EWMH client list if the window was present
 * @param winInfo
 * @return 1 iff the window was already in our records
 */
int unregisterWindow(WindowInfo* winInfo);




/**
 * Assigns names to the first n workspaces
 * @param names
 * @param numberOfNames
 */
void setWorkspaceNames(char* names[], int numberOfNames);

/**
 * Returns the first workspace index whose names matches name
 * @param name
 * @return
 */
int getIndexFromName(char* name);

/**
* Returns the name of the workspace at the specified index
* @param index
*
* @return the name of the workspace
*/
char* getWorkspaceName(int index);
/**
 * Make the active workspace the one designated by workspaceIndex
 * For the workspace to become active,it must become visible. For this to be true it must
 * swap monitor with an already visible workspace
 * Upon switching, the focus for all masters is updated to be the last focused window out of
 * all visible windows
 */
void switchToWorkspace(int workspaceIndex);

/**
 * Switch to workspace and activate the last focused window of the active master
 * @param workspaceIndex
 */
void activateWorkspace(int workspaceIndex);

/**
 * Swap the active workspace and the workspace with index workspaceIndex
 * @param workspaceIndex
 * @see swapWorkspaces()
 */
void swapWithWorkspace(int workspaceIndex);
/**
 * Workspace index1 and index2 swap monitors with no checks preformed
 * This method does not modify the active workspace index
 * @param index1
 * @param index2
 */
void swapWorkspaces(int index1, int index2);

/**
 * Moves a window to the workspace given by destIndex
 */
void moveWindowToWorkspace(WindowInfo* winInfo, int destIndex);
/**
 * Moves a window to the workspace given by destIndex
 * The method differs from moveWindowToWorkspace in that negative values have no special meaning. This method was made to send windows to "hidden" workspaces
 * @param winInfo
 * @param index a number in the range (-NUMBER_OF_HIDDEN_WINDOWS,0)
 */
void banishWindow(WindowInfo* winInfo, int index);

/**
 * Filters the configure request to allowed actions then configures the window
 * @param win
 * @param values
 * @param sibling
 * @param stackMode
 * @param configMask
 * @see xcb_configure_window
 */
void processConfigureRequest(WindowID win, short values[5], WindowID sibling, int stackMode, int configMask);

/**
 * Do everything needed to comply when EWMH.
 * Creates a functionless window with set all the Atoms we support, clear any
 * unsupported values from the root window
 */
void broadcastEWMHCompilence();

/**
 * Moves a window to the specified layer in the same workspace
 * @param win
 * @param layer
 */
void moveWindowToLayer(WindowInfo* win, int layer);

/**
 * Send a kill signal to the client with the window
 * @param win
 */
void killWindow(WindowID win);

/**
 * Sends a WM_DELETE_WINDOW message or sends a kill requests
 * @param winInfo
 * @see killWindow();
 */
void killWindowInfo(WindowInfo* winInfo);



/**
 * Adds the FLOATING_MASK to a given window
 */
void floatWindow(WindowInfo* win);
/**
 * Removes any all all masks that would cause the window to not be tiled
 */
void sinkWindow(WindowInfo* win);

/**
 * if Workspace:mapped != isVisibleWorkspace, then we either attempt to map, unmap all windows in the workspace according to isVisibleWorkspace
 *
 * @param index
 */
void syncMonitorMapState(int index);

/**
 * Swaps the workspaces and positions in said workspaces between the two windows
 * @param winInfo1
 * @param winInfo2
 */
void swapWindows(WindowInfo* winInfo1, WindowInfo* winInfo2);
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

#endif
