/**
 * @file
 * Provides macros to set various EWMH properties
 */

#ifndef MPX_EWMH_H_
#define MPX_EWMH_H_

#include <string.h>
#include <xcb/xcb_ewmh.h>

#include "../boundfunction.h"
#include "../masters.h"
#include "../mywm-structs.h"
#include "../windows.h"

/**
 *
 *
 * @return 1 iff there is a running instance of MPXManager and it is acting like a window manger (RUN_AS_WM==1)
 */
bool isMPXManagerRunning(void);

/**
 * Do everything needed to comply when EWMH.
 * Creates a functionless window with set all the Atoms we support, clear any
 * unsupported values from the root window
 */
void broadcastEWMHCompilence();
/**
 * Sets all the ewmh we support.
 *
 * Note that by default we don't support large desktops
 *
 * We don't currently support _NET_CLIENT_LIST_STACKING because it is a pain to properly implement
 */
void setSupportedActions();

/**
 * Updates the root window with a list of windows we are managing
 */
void updateEWMHClientList();
/**
 * Mark the active window and active workspace
 */
void setActiveProperties();


/**
 * Adds a series of rules to be compliant with EWMH/ICCCM spec
 *
 * @param remove
 */
void addEWMHRules();

/**
 * Reads EWMH desktop property to find the workspace the window should be long.
 * If the field is not present the active workspace is used.
 * If the field is greater than the number of workspaces, the last workspace is used
 * @param win
 * @return the workspace the window should be placed in by default
 */
WorkspaceID getSavedWorkspaceIndex(WindowID win);
/**
 * Sets the EWMH workspace property for this window to its workspace
 *
 * @param winInfo
 */
void setSavedWorkspaceIndex(WindowInfo* winInfo);

/**
 * Moves a window to its saved workspace index
 *
 * @see getSavedWorkspaceIndex
 *
 * @param winInfo
 */
void autoResumeWorkspace(WindowInfo* winInfo);
/**
 * Set all the workspace names to the root window
 */
void updateWorkspaceNames();

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

/**
 * Responds to client messages according the ICCCM/EWMH spec
 */
void onClientMessage(xcb_client_message_event_t* event);

/// the default source when sending a WM request
extern xcb_ewmh_client_source_type_t source;

/**
 * Sets the WM_STATE from the window masks
 */
void setXWindowStateFromMask(WindowInfo* winInfo, xcb_atom_t* atoms, int len);
/**
 * Reads the WM_STATE fields from the given window and sets the window mask to be consistent with the state
 * If the WM_STATE cannot be read, then nothing is done
 * @see setWindowStateFromAtomInfo
 */
void loadSavedAtomState(WindowInfo* winInfo);

/**
 * Updates WM_STATE for all MAPPABLE windows where MASKS_TO_SYNC has changed
 */
void updateXWindowStateForAllWindows();

/**
 * Sets the window state based on a list of atoms
 * @param winInfo the window whose mask will be modified
 * @param atoms the list of atoms to add,remove or toggle depending on ACTION
 * @param numberOfAtoms the number of atoms
 * @param action whether to add,remove or toggle atoms. For toggle, if all of the corresponding masks for the list of atoms is set, then they all will be removed else they all will be added
 * @see XCB_EWMH_WM_STATE_ADD, XCB_EWMH_WM_STATE_REMOVE, XCB_EWMH_WM_STATE_TOGGLE
 */
bool setWindowStateFromAtomInfo(WindowInfo* winInfo, const xcb_atom_t* atoms, uint32_t numberOfAtoms, int action);
/**
 * Sets the WM_ACTIONS supported for this given window (which is the same for all of them)
 *
 * @param winInfo
 */
void setAllowedActions(WindowInfo* winInfo);
/**
 * Sync whether or not we are only showing the desktop
 */
void syncShowingDesktop();
#endif /* EWMH_H_ */
