
/**
 * @file defaults.h
 * @brief Sane defaults
 *
 */
#ifndef DEFAULTS_H_
#define DEFAULTS_H_


#include "bindings.h"
#include "mywm-structs.h"
#include <X11/extensions/XInput2.h>







void onHiearchyChangeEvent();
void onDeviceEvent();
/**
 * Set initial window config
 * @param event
 */
void onConfigureRequestEvent();
void onVisibilityEvent();
void onCreateEvent();
void onDestroyEvent();


/**
 * Called when the a client application changes the window's state from unmapped to mapped.
 * @param context
 */
void onMapRequestEvent();
void onUnmapEvent();
void onEnterEvent();
void onFocusInEvent();
void onFocusOutEvent();
void onPropertyEvent();
void onClientMessage();
void processConfigureRequest(Window win,short values[5],xcb_window_t sibling,int stackMode,int mask);
void detectMonitors();
void onGenericEvent();
void onStartup();


#endif /* DEFAULTS_H_ */
