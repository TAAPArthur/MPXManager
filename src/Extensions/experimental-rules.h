#ifndef MPX_EXT_EXTRA_RULES_H_
#define MPX_EXT_EXTRA_RULES_H_

/**
 * @file
 */
#include "../mywm-structs.h"

/**
 * Adds a rule that will cause the WM to exit when it next becomes idle
 */
void addDieOnIdleRule(bool remove = 0);
/**
 * Float all non-normal type windows
 * POST_REGISTER_WINDOW rule
 */
void addFloatRule(bool remove = 0);
/**
 * Adds rules for focus to change when a mouse enters a new window (non-default)
 * (non-default)
 */
void addFocusFollowsMouseRule(bool remove = 0);

/**
 * Adds a POST_REGISTER_WINDOW rule to scan children of the current window
 * and register them
 *
 * @param remove
 */
void addScanChildrenRule(bool remove = 0);
/**
 * Add ProcessingWindow rule that will cause the WM to ignore windows that don't have their window type set (@see isUnknown). The window manager will not interact at all with these windows like to set focus
 * (non-default)
 */
void addUnknownInputOnlyWindowIgnoreRule(bool remove = 0) ;
/**
 * Non default event
 * It sets focus to the window the master device just entered if the master supports it
 * It expects getLast to be a xcb_input_enter_event_t
 */
void focusFollowMouse();
#endif
