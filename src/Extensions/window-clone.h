/**
 * @file window-clone.h
 * Methods to enable window cloning
 *
 */
#ifndef MAXMANAGER_WINDOW_CLONE_
#define MAXMANAGER_WINDOW_CLONE_
#include <assert.h>

#include <xcb/xcb.h>

#include "../mywm-structs.h"

/// Holds meta data mapping a clone to its parent
struct CloneInfo {
    /// ID of the clone window
    WindowID clone;
    /// ID of the original window
    WindowID original;
    ///x,y offset for cloned windows; they will clone the original starting at this offset
    int offset[2];
};

/// How often autoUpdateClones will update cloned windows (in ms)
extern uint32_t CLONE_REFRESH_RATE;

/**
 * Creates a new window and immediate processes.
 *
 * This method sets everything up for the returned window to be treated as a clone and also grabs the needed events in case any auto updated in needed
 *
 * @param winInfo
 * @param parent the new parent. If 0, it will have the same parent as winInfo
 *
 * @return a clone of winInfo
 */
WindowInfo* cloneWindow(WindowInfo* winInfo, WindowID parent = 0);

/**
 *
 *
 * @param winInfo
 * @param createNew
 *
 * @return a list of clones for the given window or null
 */
ArrayList<WindowID>* getClonesOfWindow(WindowInfo* winInfo, bool createNew = 0);
/**
 *
 *
 * @param winInfo a window clone
 * @param createNew
 *
 * @return information about the clone
 */
CloneInfo* getCloneInfo(WindowInfo* winInfo, bool createNew = 1);

/**
 * Updates the displayed image for all cloned windows
 *
 * @param winInfo
 * @return number of clones updated
 */
uint32_t updateAllClonesOfWindow(WindowInfo* winInfo);
/**
 * Updates the displayed image for all cloned windows.
 * Will also garbage collect deleted clone metadata
 * @return number of clones updated
 */
uint32_t updateAllClones(void);

/**
 * Auto update cloned window every CLONE_REFRESH_RATE ms
 */
void autoUpdateClones();
/**
 * Call autoUpdateClones in a new thread
 */
void startAutoUpdatingClones(void);
/**
 * Add rules for seamless interaction with cloned windows
 *
 * Cloned windows will now swap with the original, when the mouse enters
 * Clones will be repainted when original is exposed
 */
void addCloneRules(void);
/**
 * Updates clones when parent is exposed
 */
void onExpose(void);
/**
 * Swaps clone with original
 */
void swapWithOriginalOnEnter(void);

/**
 * Redirects clone focus to its parent
 * Should be triggered on focus in events
 */
void focusParent(void);

/**
 * If the clone is mapped and the parent is not, they are swapped.
 */
void swapOnMapEvent(void);
/**
 * If the clone is mapped and the parent is not, they are swapped.
 */
void swapOnUnmapEvent(void);

/**
 * Destroy all cloned windows
 *
 * @param winInfo
 */
void killAllClones(WindowInfo* winInfo);
/**
 * Swaps the workspaces and positions in said workspaces between the two windows
 * @param winInfo1
 * @param winInfo2
 */
void swapWindows(WindowInfo* winInfo1, WindowInfo* winInfo2);

#endif
