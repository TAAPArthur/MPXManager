/**
 * @file window-clone.h
 * Methods to enable window cloning
 *
 */
#ifndef MAXMANAGER_WINDOW_CLONE_
#define MAXMANAGER_WINDOW_CLONE_
#include "../mywm-structs.h"
#include "../masters.h"

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
WindowInfo* cloneWindow(WindowInfo* winInfo);
static inline void cloneFocusedWindow() {cloneWindow(getFocusedWindow());}

/**
 * Updates the displayed image for all cloned windows
 *
 * @param winInfo
 * @return number of clones updated
 */
void updateAllClonesOfWindow(WindowInfo* winInfo);
/**
 * Updates the displayed image for all cloned windows.
 * Will also garbage collect deleted clone metadata
 * @return number of clones updated
 */
uint32_t updateAllClones(void);

void updateAllClonesOfWindow(WindowInfo* parent);

/**
 * Auto update cloned window every CLONE_REFRESH_RATE ms
 */
void autoUpdateClones();
/**
 * Add rules for seamless interaction with cloned windows
 *
 * Cloned windows will now swap with the original, when the mouse enters
 * Clones will be repainted when original is exposed
 */
void addCloneRules(void);

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
