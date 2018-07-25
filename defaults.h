#ifndef DEFAULTS_H_
#define DEFAULTS_H_

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <X11/Xlib.h>

#include "mywm-structs.h"

extern int ROOT_EVENT_MASKS;
extern int NON_ROOT_EVENT_MASKS;
extern int DEVICE_EVENT_MASKS;
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


#endif
