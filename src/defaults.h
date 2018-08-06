#ifndef DEFAULTS_H_
#define DEFAULTS_H_

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI.h>
#include <xcb/xinput.h>
#include <X11/Xlib.h>

#include "mywm-structs.h"

extern int ROOT_EVENT_MASKS;
extern int NON_ROOT_EVENT_MASKS;
extern int ROOT_DEVICE_EVENT_MASKS;
extern int NON_ROOT_DEVICE_EVENT_MASKS;
extern char* SHELL;
extern char MANAGE_OVERRIDE_REDIRECT_WINDOWS;
extern int defaultWindowMask;
extern int CHAIN_BINDING_GRAB_MASKS;

extern int anyModier;
extern int IGNORE_MASK;

enum{KEY_PRESS,KEY_RELEASE,MOUSE_PRESS,MOUSE_RELEASE};
extern Binding*deviceBindings[4];
extern int deviceBindingLengths[4];
extern int bindingMasks[4];
extern char IGNORE_SEND_EVENT;

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
    ProcessingWindow,
    WorkspaceChange,WindowWorkspaceChange,
    WindowLayerChange,LayoutChange,
    TilingWindows,
    NUMBER_OF_EVENT_RULES
};
extern Node* eventRules[NUMBER_OF_EVENT_RULES];

extern int defaultScreenNumber;

extern int NUMBER_OF_WORKSPACES;
extern int DEFAULT_BORDER_WIDTH;

#endif
