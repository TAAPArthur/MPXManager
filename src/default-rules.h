
/**
 * @file default-rules.h
 * @brief Sane defaults
 *
 */
#ifndef DEFAULT_RULES_H_
#define DEFAULT_RULES_H_

#include "bindings.h"


/**
 * Default method called on hiearchy change events which refer to modifications of master/slave device(s)
 * The event right before this method should be a xcb_input_hierarchy_event_t event
 * This method keeps our state synced with the X Sever state for master/removal deletion
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
 * Updates the border color of the previously focused window and the new window
 */
void onFocusInEvent(void);
/**
 * Updates properties of the given window
 */
void onPropertyEvent(void);
/**
 * Responds to client messages accoring the the ICCCM/EWMH spec
 */
void onClientMessage(void);


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
 */
void addFocusFollowsMouseRule(void);

/**
 * Handles initilizing the WindowManager and proccessing user settings from the config file
 */
void onStartup(void);
#endif /* DEFAULT_RULES_H_ */
