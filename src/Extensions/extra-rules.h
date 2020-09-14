#ifndef MPX_EXTRA_RULES_H_
#define MPX_EXTRA_RULES_H_

/**
 * @file
 */
#include "mywm-structs.h"

/**
 * The function to be called when printing status.
 * @see spawnPipe, addPrintStatusRule
 */
extern void (*printStatusMethod)();

/**
 * Adds a CLIENT_MAP_ALLOW rule to auto focus a window
 *
 * Focuses a window when it is mapped and:
 * it was created within AUTO_FOCUS_NEW_WINDOW_TIMEOUT ms from now
 * getUserTime returns non-zero for the window
 *
 * @param remove
 */
void addAutoFocusRule(bool remove = 0) ;
/**
 * Adds rules to the make Desktop windows behave like desktop window.
 * Adds STICKY, MAXIMIZED NO_TILE and BELOW masks and fixes the position at view port 0,0
 */
void addDesktopRule(bool remove = 0);

/**
 * Adds a WINDOW_WORKSPACE_CHANGE to move windows from the end of the window stack to the front
 *
 * @param remove
 */
void addInsertWindowsAtHeadOfStackRule(bool remove = 0);

/**
 * Adds a XCB_INPUT_KEY_PRESS to abort processing if the event was a key repeat
 *
 * @param remove
 */
void addIgnoreKeyRepeat(bool remove = 0);
/**
 * Adds a XCB_REPARENT_NOTIFY rule to unregister a window if it is no longer a
 * direct child of the root window
 *
 * @param remove
 */
void addIgnoreNonTopLevelWindowsRule(bool remove = 0);
/**
 * Best-effort attempt to keep transients on top of the window they are transient for
 *
 * @param remove
 */
void addKeepTransientsOnTopRule(bool remove = 0);
/**
 * Non-tileable windows that are at the origin will be moved to the origin of the Workspace's monitor if any
 *
 * Windows left at the origin are assumed to still be at their creation time position which is believed to be arbitrary.
 */
void addMoveNonTileableWindowsToWorkspaceBounds() ;

/**
 * Calls printStatusMethod if set (and pipe is setup) on IDLE events
 */
void addPrintStatusRule(bool remove = 0);
/**
 * Calls requestShutdown when the WM is not IDLE
 *
 * @param remove
 */
void addShutdownOnIdleRule(bool remove = 0);

/**
 * Adds a rule for the PRIMARY_MONITOR_MASK to have any effect
 *
 * @param remove
 */
void addStickyPrimaryMonitorRule(bool remove = 0);

/**
 * Float all non-normal type windows
 * POST_REGISTER_WINDOW rule
 */
void addFloatRule(bool remove = 0);

#endif
