/**
 * @file wm-rules.h
 * @brief Provides functionally common to WM and perhaps need for a good experience
 *
 */
#ifndef MPX_WM_RULES_H_
#define MPX_WM_RULES_H_

#include "bindings.h"
#include "mywm-structs.h"


/**
 * Adds onDeviceEvent for the appropriate rules
 */
void addDefaultDeviceRules();

/**
 * Adds a bunch of rules needed for the WM to function as expected
 * The majority are wrappers to map X11 event to the corresponding function
 */
void addBasicRules();
/**
 * Register ROOT_EVENT_MASKS
 * @see ROOT_EVENT_MASKS
 */
void registerForEvents();
/**
 * Listens for NON_ROOT_EVENT_MASKS and NON_ROOT_DEVICE_EVENT_MASKS
 *
 * @param winInfo
 * @return 1 on success
 */
bool listenForNonRootEventsFromWindow(WindowInfo* winInfo);

/**
 * Rules to automatically tile workspaces when they have changed
 *
 * @param remove
 */
void addAutoTileRules();
/**
 * Adds a POST_REGISTER_WINDOW rule to ignore override redirect windows
 *
 * This method is enabled by default although it reduces functionality.
 *
 * From the ICCCM spec,
 * "The user will not be able to move, resize, restack, or transfer the input
 * focus to override-redirect windows, since the window manager is not managing them"
 * This is interpreted as it being safe to simply ignore such windows.
 *
 * In the same breath we are ignoring input only windows as they will never be mapped and aren't interesting
 *
 * TODO move comment
 * However,
 * there are cases where one may want apply rules like ALWAYS_ON_TOP_MASK to these windows.
 * For such reasons, there are other rule in addBasicRules that will help keep sane
 * behavior even if this rule is removed.
 * @param remove
 *
 */
void addIgnoreOverrideRedirectAndInputOnlyWindowsRule();
/**
 * Adds a PRE_REGISTER_WINDOW rule indicating that override redirect windows should not be managed.
 * This should prevent them from being automatically added to workspaces/auto focused
 *
 * @param remove
 *
 * @return
 */
bool addDoNotManageOverrideRedirectWindowsRule() ;

/**
 * Adds DEVICE_EVENT rule to trigger any of getDeviceBindings() using getLastUserEvent
 *
 * @see checkBindings()
 * @param remove
 */
void addApplyBindingsRule();

void addNonDocksToActiveWorkspace(WindowInfo* winInfo);
#endif /* DEFAULT_RULES_H_ */
