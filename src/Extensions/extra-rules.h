#ifndef MPX_EXTRA_RULES_H_
#define MPX_EXTRA_RULES_H_

/**
 * @file
 */
#include "../mywm-structs.h"

/**
 * Adds rules to the make Desktop windows behave like desktop window.
 * Adds STICKY, MAXIMIZED NO_TILE and BELOW masks and fixes the position at view port 0,0
 */
void addDesktopRule();

/**
 * Adds a XCB_INPUT_KEY_PRESS to abort processing if the event was a key repeat
 *
 * @param remove
 */
void addIgnoreKeyRepeat();
/**
 * Adds a XCB_REPARENT_NOTIFY rule to unregister a window if it is no longer a
 * direct child of the root window
 *
 * @param remove
 */
void addIgnoreNonTopLevelWindowsRule();

void addDetectReparentEventRule();
/**
 * Best-effort attempt to keep transients on top of the window they are transient for
 *
 * @param remove
 */
void addKeepTransientsOnTopRule();
/**
 * Non-tileable windows that are at the origin will be moved to the origin of the Workspace's monitor if any
 *
 * Windows left at the origin are assumed to still be at their creation time position which is believed to be arbitrary.
 */
void addMoveNonTileableWindowsToWorkspaceBounds() ;

/**
 * Calls printStatusMethod if set (and pipe is setup) on IDLE events
 */
void addPrintStatusRule();
/**
 * Calls requestShutdown when the WM is not IDLE
 *
 * @param remove
 */
void addShutdownOnIdleRule();

/**
 * Adds a rule for the PRIMARY_MONITOR_MASK to have any effect
 *
 * @param remove
 */
void addStickyPrimaryMonitorRule();

/**
 * Float all non-normal type windows
 * POST_REGISTER_WINDOW rule
 */
void addFloatRule();

typedef enum {
    HEAD_OF_STACK,
    BEFORE_FOCUSED,
    AFTER_FOCUSED,
} InsertWindowPosition;
void addInsertWindowsAtPositionRule(InsertWindowPosition arg);

#endif
