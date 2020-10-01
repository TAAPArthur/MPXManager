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
 * @return private window of wm if there is a running instance of MPXManager and it is acting like a window manger (RUN_AS_WM==1)
 */
WindowID isMPXManagerRunning(void);

void ownSelection(xcb_atom_t selectionAtom);

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
bool registerWindow(WindowID win, WindowID parent, xcb_get_window_attributes_reply_t* attr);
/**
 * Used to register a window when more information is know that just the window id and parent
 *
 * @param winInfo
 * @param attr
 *
 * @return
 */
bool registerWindowInfo(WindowInfo* winInfo, xcb_get_window_attributes_reply_t* attr);

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
bool unregisterWindow(WindowInfo* winInfo, bool destroyed) ;

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
 * @return 1 or 0 if no window was activated
 */
bool activateWindow(WindowInfo* winInfo);

void activateWindowUnchecked(WindowInfo* winInfo);

/**
 * Combination of switchToWorkspace and activateWindow
 *
 * Switches to the specified workspace and activates the first window in the active master's windowStack that is in said workspace. If no such window is exists, no window is explictly activated;
 *
 * @param workspaceIndex
 *
 */
void activateWorkspace(WorkspaceID workspaceIndex);



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
 */
void setWindowPosition(WindowID win, const Rect geo);
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
void raiseLowerWindow(WindowID win, WindowID sibling, bool above);
/**
 * Raises or lowers the winInfo depending on above.
 * @param winInfo
 * @param sibling the window to move above/below. If 0, then the window will be moved to top/bottom of the stack
 * @param above whether to raise or lower the window
 */
void raiseLowerWindowInfo(WindowInfo* winInfo, WindowID sibling, bool above);

/**
 * @param win
 * @param sibling the window to move above/below. If 0, then the window will be moved to top/bottom of the stack
 * @see raiseWindowInfo
 */
static inline void raiseWindow(WindowID win, WindowID sibling) {
    raiseLowerWindow(win, sibling, 1);
}
static inline void raiseWindowInfo(WindowInfo* winInfo, WindowID sibling) {
    raiseLowerWindowInfo(winInfo, sibling, 1);
}

/**
 * @param win
 * @param sibling the window to move above/below. If 0, then the window will be moved to top/bottom of the stack
 * @see raiseWindowInfo
 */
static inline void lowerWindow(WindowID win, WindowID sibling) {
    raiseLowerWindow(win, sibling, 0);
}
static inline void lowerWindowInfo(WindowInfo* winInfo, WindowID sibling) {
    raiseLowerWindowInfo(winInfo, sibling, 0);
}
#endif
