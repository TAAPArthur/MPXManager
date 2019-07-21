#include "../ld-preload.h"
#include <xcb/xinput.h>
#include <X11/extensions/XInput2.h>

static int KEYBOARD_MASKS = XCB_INPUT_XI_EVENT_MASK_KEY_PRESS | XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE;
static int POINTER_MASKS = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS | XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE |
                           XCB_INPUT_XI_EVENT_MASK_MOTION;


static int KEYBOARD = 0;
static int POINTER = 1;
static int getDeviceID(int type){
    char* var = getenv(type ? "CLIENT_POINTER" : "CLIENT_KEYBOARD");
    if(var && var[0])
        return atoi(var);
    return 0;
}
INTERCEPT(int, XGrabKeyboard, Display* dpy, Window grab_window, Bool owner_events, int pointer_mode, int keyboard_mode,
          Time time){
    int deviceID = getDeviceID(KEYBOARD);
    if(deviceID){
        XIEventMask eventmask = {deviceID, 2, (unsigned char*)& KEYBOARD_MASKS};
        return XIGrabDevice(dpy, deviceID,  grab_window, time, None, keyboard_mode,
                            pointer_mode, owner_events, &eventmask);
    }
    return BASE(XGrabKeyboard)(dpy, grab_window, owner_events, pointer_mode, keyboard_mode, time);
}
END_INTERCEPT
INTERCEPT(int, XGrabPointer, Display* dpy, Window grab_window, Bool owner_events, unsigned int event_mask,
          int pointer_mode, int keyboard_mode, Window confine_to, Cursor cursor, Time time){
    int deviceID = getDeviceID(POINTER);
    if(deviceID){
        XIEventMask eventmask = {deviceID, 2, (unsigned char*)& POINTER_MASKS};
        return XIGrabDevice(dpy, deviceID,  grab_window, time, None, pointer_mode,
                            keyboard_mode, owner_events, &eventmask);
    }
    return BASE(XGrabPointer)(dpy, grab_window, owner_events, event_mask, pointer_mode, keyboard_mode, confine_to, cursor,
                              time);
}
END_INTERCEPT
