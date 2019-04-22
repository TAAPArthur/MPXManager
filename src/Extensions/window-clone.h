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

/**
 * Creates a new window and immediates processes.
 *
 * This method sets everything up for the returned window to be treated as a clone and also grabs the needed events incase any auto updated in needed
 *
 * @param winInfo
 *
 * @return a clone of winInfo
 */
WindowInfo* cloneWindow(WindowInfo* winInfo);


/**
 * Updates the displayed image for all cloned windows
 *
 * @param win
 */
void updateAllClonesOfWindow(WindowID win);
/**
 * Updates the displayed image for all cloned windows.
 * Will also garbage collect deleted clone metadata
 */
void updateAllClones(void);

/**
 * Auto update cloned window every CLONE_REFRESH_RATE ms
 * @param arg unused
 *
 */
void* autoUpdateClones(void* arg __attribute__((unused)));
/**
 * Call autoUpdateClones in a new thread
 */
void startAutoUpdatingClones(void);
/**
 * Add rules for seemless interaction with cloned windows
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
#endif
