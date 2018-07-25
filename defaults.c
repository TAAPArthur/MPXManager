#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xinput.h>
#include <xcb/xproto.h>
#include <X11/X.h>
#include <X11/Xlib.h>

#include "defaults.h"

#include "mywm-structs.h"
#include "logger.h"


/**Mask of all events we listen for on the root window*/
int ROOT_EVENT_MASKS=SubstructureRedirectMask|SubstructureNotifyMask|StructureNotifyMask;
/**Mask of all events we listen for on non-root windows*/
int NON_ROOT_EVENT_MASKS=VisibilityChangeMask;

/**Mask of all events we listen for on all Master devices when grabing specific buttons/keys
 * (ie not grabing the entire device).
 * */
int DEVICE_EVENT_MASKS=XCB_INPUT_XI_EVENT_MASK_FOCUS_IN|XCB_INPUT_XI_EVENT_MASK_HIERARCHY|XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS;
//int DEVICE_EVENT_MASKS=XI_FocusInMask|XI_HierarchyChangedMask|XI_ButtonPressMask;
/**
 * The masks to use when grabing the keyboard for chain bindings
 */
//int CHAIN_BINDING_GRAB_MASKS = XI_KeyPressMask|XI_KeyReleaseMask;
/**Lists of modifer masks to ignore. Sane default is ModMask2 (NumLock)*/
int IGNORE_MASK=Mod2Mask;
/**Number of workspaces default is 10*/
int NUMBER_OF_WORKSPACES=1;
/**Border width when borders are active*/
int DEFAULT_BORDER_WIDTH=1;

char IGNORE_SEND_EVENT=0;

int anyModier=AnyModifier;

/**Whether or not to manage override redirect windows. Note doing so breaks EWMH spec and
 * and you still won't receive events on these windows.
 * */
char MANAGE_OVERRIDE_REDIRECT_WINDOWS=0;

char* SHELL="/bin/sh";

int defaultWindowMask=0;

int bindingMasks[4]={
        XCB_INPUT_XI_EVENT_MASK_KEY_PRESS,
        XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE,
        XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS,
        XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE
};

void setDefaultWindowMasks(int mask){
    defaultWindowMask=mask;
}

Binding*deviceBindings[4];
int deviceBindingLengths[4];

Display *dpy;
xcb_connection_t *dis;
int root;
xcb_ewmh_connection_t*ewmh;
xcb_screen_t* screen;
int defaultScreenNumber;




