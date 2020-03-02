#ifndef MPX_EXT_EXTRA_RULES_H_
#define MPX_EXT_EXTRA_RULES_H_

/**
 * @file
 */
#include "../mywm-structs.h"

/**
 * Adds a rule that will cause the WM to exit when it next becomes idle
 */
void addDieOnIdleRule(AddFlag flag = ADD_UNIQUE);
/**
 * Float all non-normal type windows
 * POST_REGISTER_WINDOW rule
 */
void addFloatRule(AddFlag flag = ADD_UNIQUE);
/**
 * Adds rules for focus to change when a mouse enters a new window (non-default)
 * (non-default)
 */
void addFocusFollowsMouseRule(AddFlag flag = ADD_UNIQUE);

/**
 * Adds a POST_REGISTER_WINDOW rule to scan children of the current window
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
 * Non default event
 * It sets focus to the window the master device just entered if the master supports it
 * It expects getLast to be a xcb_input_enter_event_t
 */
void focusFollowMouse();
#endif
