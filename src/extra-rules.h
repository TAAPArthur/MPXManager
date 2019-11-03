#ifndef EXTRA_RULES_H_
#define EXTRA_RULES_H_

/**
 * @file
 */
#include "mywm-structs.h"

/**
 * Adds rules to enforce ALWAYS_ON_TOP and ALWAYS_ON_BOTTOM
 *
 * @param flag
 */
void addAlwaysOnTopBottomRules(AddFlag flag = ADD_UNIQUE);
/**
 * Adds rules to have windows avoid docks (default)
 */
void addAvoidDocksRule(AddFlag flag = ADD_UNIQUE);

/**
 * Adds rules to strip docks of INPUT_MASK on PropertyLoad so they won't be able to be focused (non-default)
 */
void addNoDockFocusRule(AddFlag flag = ADD_UNIQUE);
/**
 * Non default event
 * It sets focus to the window the master device just entered if the master supports it
 * It expects getLast to be a xcb_input_enter_event_t
 */
void focusFollowMouse();

/**
 * Adds rules for focus to change when a mouse enters a new window (non-default)
 * (non-default)
 */
void addFocusFollowsMouseRule(AddFlag flag = ADD_UNIQUE);
/**
 * Adds rules to the make Desktop windows behave like desktop window.
 * Adds STICKY, MAXIMIZED NO_TILE and BELOW masks and fixes the position at view port 0,0
 */
void addDesktopRule(AddFlag flag = ADD_UNIQUE);

/**
 * winInfo will be automatically added to workspace
 * If LOAD_SAVED_STATE will will added to its WM_DESKTOP workspace if set.
 * else if will be added to the active workspace
 * This is added to RegisterWindow
 * @param winInfo
 */
void autoAddToWorkspace(WindowInfo* winInfo);

/**
 * Calls printStatusMethod if set (and pipe is setup) on Idle events
 */
void addPrintStatusRule(AddFlag flag = ADD_UNIQUE);
/**
 * The function to be called when printing status.
 * @see spawnPipe, addPrintStatusRule
 */
extern void (*printStatusMethod)();
/**
 * Float all non-normal type windows
 * PostRegisterWindow rule
 */
void addFloatRule(AddFlag flag = ADD_UNIQUE);
/**
 * Adds a rule that will cause the WM to exit when it next becomes idle
 */
void addDieOnIdleRule(AddFlag flag = ADD_UNIQUE);
/**
 * Calls requestShutdown when the WM is not Idle
 *
 * @param flag
 */
void addShutdownOnIdleRule(AddFlag flag = ADD_UNIQUE);
/**
 * Adds a ClientMapAllow rule to auto focus a window
 *
 * Focuses a window when it is mapped and:
 * it was created within AUTO_FOCUS_NEW_WINDOW_TIMEOUT ms from now
 * getUserTime returns non-zero for the window
 *
 * @param flag
 */
void addAutoFocusRule(AddFlag flag = ADD_UNIQUE) ;
/**
 * Ignores window whose program specified size area is less than 5
 *
 * @param flag
 */
void addIgnoreSmallWindowRule(AddFlag flag = ADD_UNIQUE) ;
/**
 * Adds a PostRegisterWindow rule to scan children of the current window
 * and register them
 *
 * @param flag
 */
void addScanChildrenRule(AddFlag flag = ADD_UNIQUE);
/**
 * Add ProcessingWindow rule that will cause the WM to ignore windows that don't have their window type set (@see isUnknown). The window manager will not interact at all with these windows like to set focus
 * (non-default)
 */
void addUnknownInputOnlyWindowIgnoreRule(AddFlag flag = ADD_UNIQUE) ;
/**
 * Adds a XCB_INPUT_KEY_PRESS to abort processing if the event was a key repeat
 *
 * @param flag
 */
void addIgnoreKeyRepeat(AddFlag flag = PREPEND_UNIQUE);
/**
 * Best-effort attempt to keep transients on top of the window they are transient for
 *
 * @param flag
 */
void addKeepTransientsOnTopRule(AddFlag flag = ADD_UNIQUE);
/**
 * Adds a border of size DEFAULT_BORDER_WIDTH to all non-INPUT_ONLY windows
 * in a workspace
 *
 * @param flag
 */
void addDefaultBorderRule(AddFlag flag = ADD_UNIQUE);
/**
 * Adds a XCB_REPARENT_NOTIFY rule to unregister a window if it is no longer a
 * direct child of the root window
 *
 * @param flag
 */
void addIgnoreNonTopLevelWindowsRule(AddFlag flag = ADD_UNIQUE);
#endif
