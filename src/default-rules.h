
/**
 * @file default-rules.h
 * @brief Sane defaults
 *
 */
#ifndef DEFAULT_RULES_H_
#define DEFAULT_RULES_H_

#include "bindings.h"


#define MOVE_WINDOW_WITH_MOUSE(mod,btn)\
        AUTO_CHAIN_GRAB(XCB_INPUT_XI_EVENT_MASK_MOTION,1,\
        {-1,-1,NULL,.noGrab=1},        \
        {mod,btn,NULL},\
        END_CHAIN(WILDCARD_MODIFIER,0)\
        )

void onHiearchyChangeEvent(void);
void onDeviceEvent(void);
void addDefaultDeviceRules();
/**
 * Set initial window config
 * @param event
 */
void onConfigureRequestEvent(void);
void onVisibilityEvent(void);
void onCreateEvent(void);
void onDestroyEvent(void);
void onError(void);
void onExpose(void);

/**
 * Called when the a client application changes the window's state from unmapped to mapped.
 * @param context
 */
void onMapRequestEvent(void);
void onMapEvent(void);
void onUnmapEvent(void);
void focusFollowMouse(void);
void onFocusInEvent(void);
void onFocusOutEvent(void);
void onPropertyEvent(void);
void onClientMessage(void);

void detectMonitors(void);

void onStartup(void);
void clearAllRules(void);
void addRule(int i,Rule*rule);

/**
 * Default hook that will be called on connection to the XServer.
 * It scans for existing windows and applies the default tiling layout to existing workspaces
 */
void onXConnect(void);

void addAvoidDocksRule(void);
void addDefaultRules(void);
void addFocusFollowsMouseRule(void);
void focusFollowMouse(void);
#endif /* DEFAULT_RULES_H_ */
