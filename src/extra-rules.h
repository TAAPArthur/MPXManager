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
 * Adds a PRE_REGISTER window rule to ignore input only windows
 * @param flag
 * @return
 */
bool addIgnoreInputOnlyWindowsRule(AddFlag flag = ADD_UNIQUE);
/**
 * Adds a CLIENT_MAP_ALLOW rule to auto focus a window
 *
 * Focuses a window when it is mapped and:
 * it was created within AUTO_FOCUS_NEW_WINDOW_TIMEOUT ms from now
 * getUserTime returns non-zero for the window
 *
 * @param flag
 */
void addAutoFocusRule(AddFlag flag = ADD_UNIQUE) ;
/**
 * Adds rules to have windows avoid docks (default)
 * @param flag
 */
void addAvoidDocksRule(AddFlag flag = ADD_UNIQUE);
/**
 * Adds rules to the make Desktop windows behave like desktop window.
 * Adds STICKY, MAXIMIZED NO_TILE and BELOW masks and fixes the position at view port 0,0
 */
void addDesktopRule(AddFlag flag = ADD_UNIQUE);

/**
 * Adds a XCB_INPUT_KEY_PRESS to abort processing if the event was a key repeat
 *
 * @param flag
 */
void addIgnoreKeyRepeat(AddFlag flag = PREPEND_UNIQUE);
/**
 * Adds a XCB_REPARENT_NOTIFY rule to unregister a window if it is no longer a
 * direct child of the root window
 *
 * @param flag
 */
void addIgnoreNonTopLevelWindowsRule(AddFlag flag = ADD_UNIQUE);
/**
 * Best-effort attempt to keep transients on top of the window they are transient for
 *
 * @param flag
 */
void addKeepTransientsOnTopRule(AddFlag flag = ADD_UNIQUE);
/**
 * Non-tileable windows that are at the origin will be moved to the origin of the Workspace's monitor if any
 *
 * Windows left at the origin are assumed to still be at their creation time position which is believed to be arbitrary.
 */
void addMoveNonTileableWindowsToWorkspaceBounds() ;

/**
 * Calls printStatusMethod if set (and pipe is setup) on IDLE events
 */
void addPrintStatusRule(AddFlag flag = ADD_UNIQUE);
/**
 * Calls requestShutdown when the WM is not IDLE
 *
 * @param flag
 */
void addShutdownOnIdleRule(AddFlag flag = ADD_UNIQUE);

/**
 * Adds a rule for the PRIMARY_MONITOR_MASK to have any effect
 *
 * @param flag
 */
void addStickyPrimaryMonitorRule(AddFlag flag = ADD_UNIQUE);

#endif
