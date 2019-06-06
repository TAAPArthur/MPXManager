/**
 * @file windows.h
 * @brief Methods related to WindowInfo
 *
 */
#ifndef WINDOWS_H_
#define WINDOWS_H_

#include <xcb/xcb.h>

#include "globals.h"
#include "mywm-structs.h"


/**
 * @param winInfo
 * @returns the full mask of the given window
 */
WindowMask getMask(WindowInfo* winInfo);
/**
 *
 * @param winInfo
 * @param mask
 * @return the intersection of mask and the window mask
 */
WindowMask hasPartOfMask(WindowInfo* winInfo, WindowMask mask);

/**
 *
 * @param win
 * @param mask
 * @return 1 iff the window containers the complete mask
 */
int hasMask(WindowInfo* win, WindowMask mask);
/**
 * Returns the subset of mask that refers to user set masks
 */
WindowMask getUserMask(WindowInfo* winInfo);

/**
 * Resets the user controlled state to the defaults
 * @param winInfo
 */
void resetUserMask(WindowInfo* winInfo);

/**
 * Adds the states give by mask to the window
 * @param winInfo
 * @param mask
 */
void addMask(WindowInfo* winInfo, WindowMask mask);
/**
 * Removes the states give by mask from the window
 * @param winInfo
 * @param mask
 */
void removeMask(WindowInfo* winInfo, WindowMask mask);
/**
 * Adds or removes the mask depending if the window already contains
 * the complete mask
 * @param winInfo
 * @param mask
 */
void toggleMask(WindowInfo* winInfo, WindowMask mask);

/**
 * Loads class and instance name for the given window
 * @param info
 */
void loadClassInfo(WindowInfo* info);
/**
 * Loads title for a given window
 * @param winInfo
 */
void loadTitleInfo(WindowInfo* winInfo);

/**
 * Loads type name and atom value for a given window
 * @param winInfo
 */
void loadWindowType(WindowInfo* winInfo);

/**
 * Loads grouptId, input and window state for a given window
 * @param winInfo
 */
void loadWindowHints(WindowInfo* winInfo);

/**
 * Load window protocols like WM_DELETE_WINDOW
 * @param winInfo
 */
void loadProtocols(WindowInfo* winInfo);

/**
 * Load various window properties
 * @param winInfo
 * @see loadClassInfo(),loadTitleInfo(),loadWindowType(),loadWindowHints()
 */
void loadWindowProperties(WindowInfo* winInfo);


/**
 * Sets the WM_STATE from the window masks
 */
void setXWindowStateFromMask(WindowInfo* winInfo);
/**
 * Reads the WM_STATE fields from the given window and sets the window mask to be consistent with the state
 * If the WM_STATE cannot be read, then nothing is done
 * @see setWindowStateFromAtomInfo
 */
void loadSavedAtomState(WindowInfo* winInfo);

/**
 * Sets the window state based on a list of atoms
 * @param winInfo the window whose mask will be modified
 * @param atoms the list of atoms to add,remove or toggle depending on ACTION
 * @param numberOfAtoms the number of atoms
 * @param action whether to add,remove or toggle atoms. For toggle, if all of the corresponding masks for the list of atoms is set, then they all will be removed else they all will be added
 * @see XCB_EWMH_WM_STATE_ADD, XCB_EWMH_WM_STATE_REMOVE, XCB_EWMH_WM_STATE_TOGGLE
 */
void setWindowStateFromAtomInfo(WindowInfo* winInfo, const xcb_atom_t* atoms, int numberOfAtoms, int action);

/**
 * Determines if and how a given window should be managed
 * @param winInfo
 * @return 1 iff the window wasn't ignored
 */
int processNewWindow(WindowInfo* winInfo);



/**
 * Adds the window to our list of managed windows as a non-dock
 * and load any EWMH saved state
 * @param winInfo
 * @return 0 if the window doesn't exists
 */
int registerWindow(WindowInfo* winInfo);

/**
 * Enables tiling to be overridden at the indexes corresponding to mask
 *
 * @param winInfo
 * @param mask
 */
void enableTilingOverride(WindowInfo* winInfo, unsigned int mask);
/**
 * Disables tiling to be overridden at the indexes corresponding to mask
 *
 * @param winInfo
 * @param mask
 */
void disableTilingOverride(WindowInfo* winInfo, unsigned int mask);
/**
 * @see enableTilingOverride
 * @see disableTilingOverride
 *
 * @param winInfo
 * @param mask
 * @param value whether to enable or disable tiling
 */
void setTilingOverrideEnabled(WindowInfo* winInfo, unsigned int mask, bool value);
/**
 * Checks to see if tilling has been overridden at the given index
 *
 * @param winInfo
 * @param index
 *
 * @return true iff tiling override is enabled
 */
bool isTilingOverrideEnabledAtIndex(WindowInfo* winInfo, int index);
/**
 * Returns User set geometry that will override that generated when tiling
 * @param winInfo
 * @see WindowInfo->config
 */
short* getTilingOverride(WindowInfo* winInfo);
/**
 * Sets window geometry that will replace that which is generated when tiling
 * @param winInfo
 * @param geometry
 */
void setTilingOverride(WindowInfo* winInfo, short* geometry);
/**
 * Get the last recorded geometry of the window
 * @param winInfo
 */
short* getGeometry(WindowInfo* winInfo);
/**
 * Set the last recorded geometry of the window
 * @param winInfo
 * @param geometry
 */
void setGeometry(WindowInfo* winInfo, short* geometry);

/**
 * Locks the window so its geometry will not be kept up to date
 *
 * @param winInfo
 */
void lockWindow(WindowInfo* winInfo);
/**
 * Unlocks the window so its geometry will be kept up to date
 *
 * @param winInfo
 */
void unlockWindow(WindowInfo* winInfo);
/**
 * Queries the XServer for all top level windows and process all of them
 * that do not have override redirect
 */
void scan(xcb_window_t baseWindow);


/**
 * Creates a pointer to a WindowInfo object and sets its id to id
 *
 * The workspaceIndex of the window is set to NO_WORKSPACE
 * @param id    the id of the WindowInfo object
 * @return  pointer to the newly created object
 */
WindowInfo* createWindowInfo(WindowID id);

/**
 * Returns a struct with stored metadata on the given window
 * @param win
 * @return pointer to struct with info on the given window
 */
WindowInfo* getWindowInfo(WindowID win);


/**
 * @return a list of windows with the most recently used windows first
 */
ArrayList* getAllWindows();

/**
 *
 *
 * @param index
 * @param mask
 *
 * @return true if there exists at least one window  in workspace with the given mask
 */
bool doesWorkspaceHaveWindowsWithMask(int index, WindowMask mask);



/**
 * To be called when a master focuses a given window.
 *
 * If the window is not in the window list, nothing is done
 * else
 * The window is added to the active master's stack
 * if the master is not frozen, then the window is added/shifted to the head of the master stack
 * The window is shifted the head of the global window list
 * The master is promoted to the head of the master stack
 *
 * Note that if a new window is focused, it won't be added to the master stack if it is
 * frozen
 * @param win
 */
void onWindowFocus(WindowID win);







/**
 * Removes window from context and master(s)/workspace lists.
 * The Nodes containing the window and the windows value are freeded also
 * @param winToRemove
 */
int removeWindow(WindowID winToRemove);
/**
 * Frees winInfo and internal structs
 * @param winInfo
 * @see removeWindow()
 */
void deleteWindowInfo(WindowInfo* winInfo);


/**
 * Adds a window to the list of windows in context iff it
 * isn't already in the list
 * Note that the created window is not added to a master window list or
 * a workspace
 * @param wInfo    instance to add
 * @return 1 iff this pointer was added
 */
int addWindowInfo(WindowInfo* wInfo);

/**
 * @param winInfo
 *
 * @return true iff the window has been marked as a dock
 */
static inline int isDock(WindowInfo* winInfo){
    return winInfo->dock;
}
/**
 * Marks the window as a dock. Registered windows marked as docks will be used to resize monitor viewports
 * @param winInfo
 */
void markAsDock(WindowInfo* winInfo);



/**
 * @param winInfo
 * @return 1 iff the window is PARTIALLY_VISIBLE or VISIBLE
 */
static inline int isWindowVisible(WindowInfo* winInfo){
    return winInfo && hasMask(winInfo, PARTIALLY_VISIBLE);
}

/**
 *
 * @param winInfo the window whose map state to query
 * @return the window map state either UNMAPPED(0) or MAPPED(1)
 */
static inline int isMappable(WindowInfo* winInfo){
    return winInfo && hasMask(winInfo, MAPPABLE_MASK);
}
/**
 * @param winInfo
 * @return true if the user can interact (focus,type etc)  with the window
 * For this to be true the window would have to be mapped and not hidden
 */
static inline int isInteractable(WindowInfo* winInfo){
    return hasMask(winInfo, MAPPED_MASK) && !hasMask(winInfo, HIDDEN_MASK);
}
/**
 * Determines if a window should be tiled given its mapState and masks
 * @param winInfo
 * @return true if the window should be tiled
 */
static inline int isTileable(WindowInfo* winInfo){
    return isInteractable(winInfo) && !hasPartOfMask(winInfo, ALL_NO_TILE_MASKS);
}
/**
 * A window is activatable if it is MAPPABLE and not HIDDEN  and does not have the NO_ACTIVATE_MASK set
 * @param winInfo
 * @return true if the window can receive focus
 */
static inline int isActivatable(WindowInfo* winInfo){
    return !winInfo || hasMask(winInfo, MAPPABLE_MASK | INPUT_MASK) &&
           !hasPartOfMask(winInfo, HIDDEN_MASK | NO_ACTIVATE_MASK);
}


/**
 * @param winInfo
 * @return 1 iff external resize requests should be granted
 */
static inline int isExternallyResizable(WindowInfo* winInfo){
    return !winInfo || hasMask(winInfo, EXTERNAL_RESIZE_MASK) || !isMappable(winInfo);
}

/**
 * @param winInfo
 * @return 1 iff external move requests should be granted
 */
static inline int isExternallyMoveable(WindowInfo* winInfo){
    return !winInfo || hasMask(winInfo, EXTERNAL_MOVE_MASK) || !isMappable(winInfo);
}

/**
 * @param winInfo
 * @return 1 iff external border requests should be granted
 */
static inline int isExternallyBorderConfigurable(WindowInfo* winInfo){
    return !winInfo || hasMask(winInfo, EXTERNAL_BORDER_MASK) || !isMappable(winInfo);
}
/**
 * @param winInfo
 * @return 1 iff external raise requests should be granted
 */
static inline int isExternallyRaisable(WindowInfo* winInfo){
    return !winInfo || hasMask(winInfo, EXTERNAL_RAISE_MASK) || !isMappable(winInfo);
}
/**
 * Checks to see if the window has SRC* masks set that will allow. If not client requests with such a source will be ignored
 * @param winInfo
 * @param source
 * @return
 */
static inline int allowRequestFromSource(WindowInfo* winInfo, int source){
    return !winInfo || hasMask(winInfo, 1 << (source + SRC_INDICATION_OFFSET));
}
#endif
