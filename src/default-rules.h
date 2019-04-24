
/**
 * @file default-rules.h
 * @brief Provides functionally common to WM and perhaps need for a good experience
 *
 */
#ifndef DEFAULT_RULES_H_
#define DEFAULT_RULES_H_

#include "mywm-structs.h"
/**
 * Default method called on hiearchy change events which refer to modifications of master/slave device(s)
 * The event right before this method should be a xcb_input_hierarchy_event_t event
 * This method keeps our state synced with the X Sever state for master addition/removal
 */
void onHiearchyChangeEvent(void);
/**
 * Default method for key/button press/release and mouse motion events
 * After some processing, we check to see if the key/mod/mask combination matches any key bindings via checkBindigs()
 */
void onDeviceEvent(void);
/**
 * Adds onDeviceEvent for the appropriate rules
 */
void addDefaultDeviceRules();
/**
 * Set initial window config
 */
void onConfigureRequestEvent(void);
/**
 * updates window geometry
 */
void onConfigureNotifyEvent(void);
/**
 * Keeps visibility masks in sync with XServer state
 */
void onVisibilityEvent(void);
/**
 * Proccess a window to see if we should keep track of it
 */
void onCreateEvent(void);
/**
 * Removes the destroyed window
 */
void onDestroyEvent(void);

/**
 * Handles the case where we get an unchecked error by logging the error
 */
void onError(void);

/**
 * Called when the a client application changes the window's state from unmapped to mapped.
 */
void onMapRequestEvent(void);
/**
 * Keeps mapped masks in sync with XServer state
 */
void onMapEvent(void);
/**
 * Keeps mapped masks in sync with XServer state
 */
void onUnmapEvent(void);
/**
 * Non default event
 * It sets focus to the window the master device just entered if the master supports it
 * It expects getLast to be a xcb_input_enter_event_t
 */
void focusFollowMouse(void);
/**
 * Updates the border color of the new window
 */
void onFocusInEvent(void);
/**
 * Updates the border color of the (old) focused window to not reflect the current master
 */
void onFocusOutEvent(void);
/**
 * Quits if we have lost the WM_SELECTION
 */
void onSelectionClearEvent(void);
/**
 * Updates properties of the given window
 */
void onPropertyEvent(void);
/**
 * Responds to client messages accoring the the ICCCM/EWMH spec
 */
void onClientMessage(void);

/**
 * Add ProcessingWindow rule that will cause the WM to ignore windows that don't have their window type set (@see isUnknown). The window manager will not interact at all with these windows like to set focus
 * (non-default)
 */
void addUnknownWindowIgnoreRule(void);
/**
 * Add RegisteringWindow rule that will cause the WM to not add the window to any workspace. This is like permentatnly setting FLOATING_MASK and STICKY_MASK
 * (non-default)
 * @see isUnknown
 */
void addUnknownWindowRule(void);
/**
 * Default hook that will be called on connection to the XServer.
 * It scans for existing windows and applies the default tiling layout to existing workspaces
 */
void onXConnect(void);

/**
 * Adds rules to have windows avoid docks (default)
 */
void addAvoidDocksRule(void);

/**
 * Adds rules to strip docks of INPUT_MASK on PropertyLoad so they won't be able to be focused (non-default)
 */
void addNoDockFocusRule(void);
/**
 * Adds the full set of default rules
 */
void addDefaultRules(void);
/**
 * Adds rules for focus to change when a mouse enters a new window (non-default)
 * (non-default)
 */
void addFocusFollowsMouseRule(void);

/**
 * winInfo will be automatically added to workspace
 * If LOAD_SAVED_STATE will will added to its WM_DESKTOP workspace if set.
 * else if will be added to the active workspace
 * This is added to RegisterWindow
 * @param winInfo
 */
void autoAddToWorkspace(WindowInfo* winInfo);
/**
 * Adds rules to various events that may indicate that the state has changed. (markState)
 * When any of these are called, we check the state via a Periodic hook. If it indeed has, we retile the workspace.
 * We unmark the state if for any reason, a workspace has been retiled
 */
void addAutoTileRules(void);

/**
 * Calls printStatusMethod if set (and pipe is setup) on Idle events
 */
void addPrintRule(void);
/**
 * Float all non-normal type windows
 * RegisteringWindow rule
 */
void addFloatRules(void);
/**
 * Handles initilizing the WindowManager and proccessing user settings from the config file
 */
void onStartup(void);
/**
 * Adds a bunch of rules needed for the WM to function as expected
 * The majority are wrappers to map X11 event to the corrosponding function
 */
void addBasicRules(void);
/**
 * for i in arr, add basic rule i for both normal and batch events
 *
 * @param arr
 * @param len
 */
void addSomeBasicRules(int* arr, int len);
/**
 * Adds a rule that will cause the WM to exit when it next becomes idle
 */
void addDieOnIdleRule(void);
#endif /* DEFAULT_RULES_H_ */
