/**
 * @file window-clone.h
 * Methods to enable window cloning
 *
 */
#include <assert.h>

#include <xcb/xcb.h>

#include "mywm-structs.h"

/**
 * Clones a WindowInfo object and sets its id to id
 *
 * @param id    the id of the WindowInfo object
 * @param winInfo    the window to clone
 * @return  pointer to the newly created object
 */
WindowInfo* cloneWindowInfo(WindowID id, WindowInfo* winInfo);
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
 * Updates the displayed image for the cloned window
 *
 * @param winInfo
 * @param dest
 */
void updateClone(WindowInfo* winInfo, WindowInfo* dest);
/**
 * Updates the displayed image for all cloned windows
 *
 * @param winInfo
 */
void updateAllClones(WindowInfo* winInfo);
/**
 *
 * @param winInfo
 *
 * @return the original window id this clone was based off of
 */
int getCloneOrigin(WindowInfo* winInfo);

/**
 * Auto update cloned window every CLONE_REFRESH_RATE ms
 *
 * @return NULL
 */
void* autoUpdateClones();
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
void swapWithOriginal(void);
