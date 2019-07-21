/**
 * @file wmfunctions.h
 * @brief Provides methods to help sync our internal structs to X11
 */

#ifndef MYWM_FUNCTIONS_H_
#define MYWM_FUNCTIONS_H_

#include <xcb/xcb.h>

#include "windows.h"
#include "masters.h"
//
/// size of config parameters
#define CONFIG_LEN 6
enum {CONFIG_X = 0, CONFIG_Y = 1, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_BORDER, CONFIG_STACK};

bool postRegisterWindow(WindowInfo* winInfo, bool newlyCreated);
static inline ArrayList<WindowInfo*>& getActiveWindowStack() {return getActiveMaster()->getWorkspace()->getWindowStack();}
/**
 * Determines if and how a given window should be managed
 *
 *
 * @param winInfo
 * @return 1 iff the window wasn't ignored
 */
bool registerWindow(WindowID win, WindowID parent, xcb_get_window_attributes_reply_t* attr = NULL);

/**
 * Queries the XServer for all top level windows and process all of them
 * that do not have override redirect
 */
void scan(xcb_window_t baseWindow);
/**
 * Removes a window from our records and updates EWMH client list if the window was present
 * @param winInfo
 * @return 1 iff the window was already in our records
 */
bool unregisterWindow(WindowInfo* winInfo);

/**
 * Send a kill signal to the client with the window
 * @param win
 */
int killClientOfWindow(WindowID win);
int destroyWindow(WindowID win);

/**
 * Sends a WM_DELETE_WINDOW message or sends a kill requests
 * @param winInfo
 * @see killWindow();
 */
void killClientOfWindowInfo(WindowInfo* winInfo);

/**
 * Maps the window specified by id if it is allowed to be mapped (ie if it is in a visible workspace and is mappable)
 * @param id
 * @return 0 if the attempt was successful
 */
int attemptToMapWindow(WindowID id);

void updateWindowWorkspaceState(WindowInfo* winInfo, bool updateFocus = 0);
/**
 * If the workspace is visible, map all MAPPABLE windows
 * Else unmap all windows
 *
 * @param index
 */
void syncMappedState(WorkspaceID index);

/**
 * Make the active workspace the one designated by workspaceIndex and make it visible.
 * If the workspace is not already visible, it will swap with a visible workspace.
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
 * Switch to the workspace of winInfo (is it has one) and raise and focus winInfo
 * @param winInfo
 * @return the id of the activated window or 0 if no window was activated
 */
WindowID activateWindow(WindowInfo* winInfo);



/**
 * @see xcb_configure_window
 */
void configureWindow(WindowID win, uint32_t mask, int values[7]);

void setWindowPosition(WindowID win, const RectWithBorder geo);
/**
 * Swaps the workspaces and positions in said workspaces between the two windows
 * @param winInfo1
 * @param winInfo2
 */
void swapWindows(WindowInfo* winInfo1, WindowInfo* winInfo2);
/**
 * Filters the configure request to allowed actions then configures the window
 * @param win
 * @param values
 * @param sibling
 * @param stackMode
 * @param configMask
 * @see xcb_configure_window
 */
void processConfigureRequest(WindowID win, const short values[5], WindowID sibling, int stackMode, int configMask);


/**
 * Sets the border width to 0
 *
 * @param winInfo
 */
void removeBorder(WindowInfo* winInfo);
/**
 * Raises or lowers the winInfo depending on above.
 * @param winInfo
 * @param above whether to raise or lower the window
 */
bool raiseWindowInfo(WindowInfo* winInfo, bool above = 1);
/**
 * @param winInfo
 * @see raiseWindowInfo
 */
bool lowerWindowInfo(WindowInfo* winInfo);
/**
 *
 *
 * @param index
 * @param mask
 *
 * @return true if there exists at least one window  in workspace with the given mask
 */
bool doesWorkspaceHaveWindowWithMask(WorkspaceID index, WindowMask mask);
#endif
