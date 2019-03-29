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
 * @return  true if the state actuall changed
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
 * @return 1 iff no error was deteced
 */
int setBorderColor(WindowID win, unsigned int color);

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
int resetBorder(WindowInfo* winInfo);

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
int focusWindow(WindowID win);
/**
 * Focus the given window
 * This method is diffrent from focusWindow in that it allows diffrent protocals for
 * focusing the window based on window masks
 */
int focusWindowInfo(WindowInfo* winInfo);
/**
 * Raises the given window
 * @param win   the window to raise
 * @return 1 iff no error was detected
 */
int raiseWindow(WindowID win);
/**
 * Raise the given window within its onw layer
 */
int raiseWindowInfo(WindowInfo* winInfo);

/**
 * Raises or lowers the winInfo depening on raise.
 * If the transientFor property is set, then the window is raised lowered releative to is tansientFor window
 * @param winInfo
 * @param raise
 */
void raiseLowerWindowInfo(WindowInfo* winInfo, int raise);

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
int deleteWindow(WindowID winToRemove);




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
* Returns the name of the workspace at the specifed index
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
 * Switch to workspace and activate the last focused window of the actie master
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
 * Moves a window to the workspace given by destIndex at the NORMAL_LAYER
 */
void moveWindowToWorkspace(WindowInfo* winInfo, int destIndex);

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
 * unsporrted values from the root window
 */
void broadcastEWMHCompilence();

/**
 * Moves a window to the specifed layer in the same workspace
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
 * Updates the root window with a list of windows we are managing
 */
void updateEWMHClientList(void);


/**
 * Adds the FLOATING_MASK to a given window
 */
void floatWindow(WindowInfo* win);
/**
 * Removes any all all masks that would cause the window to not be tiled
 */
void sinkWindow(WindowInfo* win);

/**
 * if Workspace:mapped != isVisibleWorkspace, then we either attempt to map,unmap all windows in the workspace according to isVisibleWorkspace
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
 *
 *
 * @param index
 * @param mask
 *
 * @return true if there exists at least one window  in workspace with the given mask
 */
int doesWorkspaceHaveWindowsWithMask(int index, WindowMask mask);
#endif
