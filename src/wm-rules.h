
/**
 * @file wm-rules.h
 * @brief Provides functionally common to WM and perhaps need for a good experience
 *
 */
#ifndef DEFAULT_RULES_H_
#define DEFAULT_RULES_H_

#include "mywm-structs.h"


/**
 * Handles the case where we get an unchecked error by logging the error
 */
void onError(void);


/**
 * Keeps visibility masks in sync with XServer state
 */
void onVisibilityEvent(void);

/**
 * Processes a window to see if we should keep track of it
 */
void onCreateEvent(void);
/**
 * Removes the destroyed window
 */
void onDestroyEvent(void);

/**
 * Keeps mapped masks in sync with XServer state
 */
void onMapEvent(void);
/**
 * Keeps mapped masks in sync with XServer state
 */
void onUnmapEvent(void);
/**
 * Called when a client application changes the window's state from unmapped to mapped.
 */
void onMapRequestEvent(void);
/**
 * Removes/adds windows from our list of managed windows when it is reparented
 */
void onReparentEvent(void);
/**
 * updates window geometry
 */
void onConfigureNotifyEvent(void);
/**
 * Set initial window config
 */
void onConfigureRequestEvent(void);
/**
 * Updates properties of the given window
 */
void onPropertyEvent(void);
/**
 * Quits if we have lost the WM_SELECTION
 */
void onSelectionClearEvent(void);

/**
 * Default method for key/button press/release and mouse motion events
 * After some processing, we check to see if the key/mod/mask combination matches any key bindings via checkBindigs()
 */
void onDeviceEvent(void);
/**
 * Updates the border color of the new window
 */
void onFocusInEvent(void);
/**
 * Updates the border color of the (old) focused window to not reflect the current master
 */
void onFocusOutEvent(void);
/**
 * Default method called on hierarchy change events which refer to modifications of master/slave device(s)
 * The event right before this method should be a xcb_input_hierarchy_event_t event
 * This method keeps our state synced with the X Sever state for master addition/removal
 */
void onHiearchyChangeEvent(void);

/**
 * Adds onDeviceEvent for the appropriate rules
 */
void addDefaultDeviceRules();


/**
 * Default hook that will be called on connection to the XServer.
 * It scans for existing windows and applies the default tiling layout to existing workspaces
 */
void onXConnect(void);

/**
 * Adds a bunch of rules needed for the WM to function as expected
 * The majority are wrappers to map X11 event to the corresponding function
 */
void addBasicRules(AddFlag flag = ADD_UNIQUE);
/**
 * TODO rename
 * Register ROOT_EVENT_MASKS
 * @see ROOT_EVENT_MASKS
 */
void registerForEvents();
void listenForNonRootEventsFromWindow(WindowInfo* winInfo);

/**
 * Attempts to translate the generic event receive into an extension event and applies corresponding Rules.
 */
void onGenericEvent(void);

/**
 * Adds rules to various events that may indicate that the state has changed. (markState)
 * When any of these are called, we check the state via a Periodic hook. If it indeed has, we retile the workspace.
 * We unmark the state if for any reason, a workspace has been retiled
 */
void addAutoTileRules(AddFlag flag = ADD_UNIQUE);
bool addIgnoreOverrideRedirectWindowsRule(AddFlag flag = ADD_UNIQUE);
void addApplyBindingsRule(AddFlag flag = ADD_UNIQUE);
#endif /* DEFAULT_RULES_H_ */
