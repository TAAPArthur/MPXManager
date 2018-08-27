/**
 * @file globals.h
 */
#ifndef GLOBALS_H_
#define GLOBALS_H_

/// \cond
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI.h>
#include <xcb/xinput.h>
#include <X11/Xlib.h>
/// \endcond

#include "util.h"

///Returns the length of the array
#define LEN(X) (sizeof X / sizeof X[0])

///If true, then check used saved window properties as defaults
extern char LOAD_SAVED_STATE;
extern long KILL_TIMEOUT;
/**Mask of all events we listen for on the root window*/
extern int ROOT_EVENT_MASKS;
/**Mask of all events we listen for on non-root windows*/
extern int NON_ROOT_EVENT_MASKS;
/**Mask of all events we listen for on relateding to Master devices
 * and the root window.
 */
extern int ROOT_DEVICE_EVENT_MASKS;
/**Mask of all events we listen for on relateding to Master devices
 * and non-root window.
 */
extern int NON_ROOT_DEVICE_EVENT_MASKS;
/// The default SHELL; This defaults to the SHELL environment var
extern char* SHELL;

extern int CLONE_REFRESH_RATE;
///Defaults masks to be applied to windows
extern int DEFAULT_WINDOW_MASKS;
///If unspecifed the mask of a Binding
extern int DEFAULT_BINDING_MASKS;
///If unspecifed the mask, used during a chaing binding grab
#define DEFAULT_CHAIN_BINDING_GRAB_MASK XCB_INPUT_XI_EVENT_MASK_KEY_PRESS

///The started workspace if !LOAD_STATE or the root CURRENT_DESKTOP property is not set
extern int DEFAULT_WORKSPACE_INDEX;

///The modifier that will match any other modifier
extern int WILDCARD_MODIFIER;
///Masks to ignore; Default is ModMask2 (NUM_LOCK)
extern int IGNORE_MASK;
///IF True we will crash if an error is received
/// This should only be used for testing as errors can happen just by
/// the async nature of X11
extern int CRASH_ON_ERRORS;

///Number of workspaces to create
extern int NUMBER_OF_WORKSPACES;

/**
 * List of all bindings
 */
extern Node*deviceBindings;

///If true ignore all events with the send_event bit
extern char IGNORE_SEND_EVENT;
/// If true ignor all device events with the key repeat flag set
extern char IGNORE_KEY_REPEAT;

/**XDisplay instance (only used for events/device commands)*/
extern Display *dpy;
/**XCB display instance*/
extern xcb_connection_t *dis;
/**EWMH instance*/
extern xcb_ewmh_connection_t *ewmh;
/**Root window*/
extern int root;
/**Default screen (assumed to be only 1*/
extern xcb_screen_t* screen;

#define GENERIC_EVENT_OFFSET LASTEvent
#define MONITOR_EVENT_OFFSET GENERIC_EVENT_OFFSET+XCB_INPUT_XI_SELECT_EVENTS
#define LAST_REAL_EVENT   MONITOR_EVENT_OFFSET+8

enum{
    //if all rules are passed through, then the window is added as a normal window
    onXConnection=LAST_REAL_EVENT,
    ProcessingWindow,RegisteringWindow,
    WorkspaceChange,WindowWorkspaceChange,
    WindowLayerChange,LayoutChange,
    Idle,
    NUMBER_OF_EVENT_RULES
};
extern Node* eventRules[NUMBER_OF_EVENT_RULES];

/// Used for select xcb functions
extern int defaultScreenNumber;

/**Border width when borders for windows*/
extern int DEFAULT_BORDER_WIDTH;

void init();

/**
 *Set the last event received.
 * @param event the last event received
 * @see getLastEvent
 */
void setLastEvent(void* event);
/**
 * Retries the last event received
 * @see getLastEvent
 */
void* getLastEvent();

extern void (*preStartUpMethod)();
extern void (*startUpMethod)();

#endif
