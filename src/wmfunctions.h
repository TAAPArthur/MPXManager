/**
 * @file wmfunctions.h
 * @brief Provides methods to help sync our internal structs to X11
 */

#ifndef MYWM_FUNCTIONS_H_
#define MYWM_FUNCTIONS_H_

#include <xcb/xcb.h>

#include "windows.h"
#include "masters.h"


/**
 * Called after a window has been successfully registered
 *
 * if newlyCreated is true, then a createEvent should have been detected for this window.
 * The window's creation time will be set to the current time
 * Else hints are loaded for this window. If this window is currently in the MappedState, then we set the appropriate masks, loadProperties and run CLIENT_MAP_ALLOW
 *
 * If the function hasn't short circuited, then POST_REGISTER_WINDOW rules are triggered
 *
 * @param winInfo
 * @param newlyCreated
 *
 * @return 1 if this function wasn't short circuited
 */
bool postRegisterWindow(WindowInfo* winInfo, bool newlyCreated);
/**
 * Performs checks on the window and adds it to the our managed list if it passes.
 *
 * First a new WindowInfo is created from win and parent.
 * If attr is set, the window is considered not newly created.
 * Else attr is loaded
 * Values from attr are added to the WindowInfo object
 * If attr fails to be loaded, the method aborts
 *
 * PreregisterWindow rules are applied
 *
 * If all these pass, the window is added to our list and postRegisterWindow is called
 *
 *
 * @param win
 * @param parent
 * @param attr
 *
 * @return the result of POST_REGISTER_WINDOW or 0 if it was never called
 */
bool registerWindow(WindowID win, WindowID parent, xcb_get_window_attributes_reply_t* attr = NULL);
/**
 * Used to register a window when more information is know that just the window id and parent
 *
 * @param winInfo
 * @param attr
 *
 * @return
 */
bool registerWindow(WindowInfo* winInfo, xcb_get_window_attributes_reply_t* attr = NULL);

/**
 * Queries the XServer for all direct children of baseWindow
 * @param baseWindow
 */
void scan(xcb_window_t baseWindow);
/**
 * Removes a window from our records and updates EWMH client list if the window was present
 * @param winInfo
 * @param destroyed a hint as to wheter the window has been destroyed by X. This should only be true if a destroy event was received
 * @return 1 iff the window was already in our records
 */
bool unregisterWindow(WindowInfo* winInfo, bool destroyed = 0) ;

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

/**
 * Updates the map state of the window to be in sync with its workspace.
 * For Sticky windows which should always be mapped, this involves moving it to a different workspace
 * @param winInfo
 */
void updateWindowWorkspaceState(WindowInfo* winInfo);
/**
 * For all masters focused on winInfoToIgnore, the focus will be shifted to the
 * first focusable window in the window stack not including winInfoToIgnore.
 * If there is no such window, focus will be transferred to defaultWinInfo instead.
 *
 * @param winInfo
 */
void updateFocusForAllMasters(WindowInfo* winInfo);

/**
 * Make the active workspace the one designated by workspaceIndex and make it visible.
 * If the workspace is not already visible, it will swap with a visible workspace.
 * Upon switching, the focus for all masters is updated to be the last focused window out of
 * all visible windows
 */
void switchToWorkspace(int workspaceIndex);

/**
 * Switch to the workspace of winInfo (is it has one) and raise and focus winInfo
 * @param winInfo
 * @return the id of the activated window or 0 if no window was activated
 */
WindowID activateWindow(WindowInfo* winInfo);



/**
 * @see xcb_configure_window
 */
void configureWindow(WindowID win, uint32_t mask, uint32_t values[7]);

/**
 * Sets the window position to be geo.
 *
 * This is a wrapper for processConfigureRequest
 *
 * @param win
 * @param geo
 * @param onlyPosition if true, then only x,y position will be modified
 */
void setWindowPosition(WindowID win, const RectWithBorder geo, bool onlyPosition = 0);
/**
 * Swaps the workspaces and positions in said workspaces between the two windows
 * @param winInfo1
 * @param winInfo2
 */
void swapWindows(WindowInfo* winInfo1, WindowInfo* winInfo2);
/**
 * Filters the configure request to allowed actions then configures the window
 * The request is filtered by the mask on win if it is registered
 * @see EXTERNAL_CONFIGURABLE_MASK
 * @param win
 * @param values
 * @param sibling
 * @param stackMode
 * @param configMask
 * @see xcb_configure_window
 */
int processConfigureRequest(WindowID win, const short values[5], WindowID sibling, int stackMode, int configMask);


/**
 * Sets the border width to 0
 *
 * @param win
 */
void removeBorder(WindowID win);


/**
 * @param upper iff true returns the boundary between the normal and top layer instead of the normal and bottom layer
 *
 * @return the window seperating the BELOW/ABOVE windows from the normal windows
 */
WindowID getWindowDivider(bool upper);
/**
 * Raises or lowers the winInfo depending on above.
 * @param win
 * @param sibling the window to move above/below. If 0, then the window will be moved to top/bottom of the stack
 * @param above whether to raise or lower the window
 */
void raiseWindow(WindowID win, WindowID sibling = 0, bool above = 1);
/**
 * Raises or lowers the winInfo depending on above.
 * @param winInfo
 * @param sibling the window to move above/below. If 0, then the window will be moved to top/bottom of the stack
 * @param above whether to raise or lower the window
 */
void raiseWindow(WindowInfo* winInfo, WindowID sibling = 0, bool above = 1);

/**
 * @param win
 * @param sibling the window to move above/below. If 0, then the window will be moved to top/bottom of the stack
 * @see raiseWindowInfo
 */
static inline void lowerWindow(WindowID win, WindowID sibling = 0) {
    raiseWindow(win, sibling, 0);
}
static inline void lowerWindow(WindowInfo* winInfo, WindowID sibling = 0) {
    raiseWindow(winInfo, sibling, 0);
}
#endif
