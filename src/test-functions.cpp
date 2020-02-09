#include <xcb/xinput.h>
#include <xcb/xtest.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XTest.h>

#include "logger.h"
#include "masters.h"
#include "test-functions.h"
#include "xsession.h"

void sendButtonPress(int button, MasterID id) {
    XDevice dev = {.device_id = id};
    XTestFakeDeviceButtonEvent(dpy, &dev, button, 1, NULL, 0, CurrentTime);
}
void sendButtonRelease(int button, MasterID id) {
    XDevice dev = {.device_id = id};
    XTestFakeDeviceButtonEvent(dpy, &dev, button, 0, NULL, 0, CurrentTime);
}
void clickButton(int button, MasterID id) {
    VERBOSE("Clicking button " << button << " for " << id);
    sendButtonPress(button, id);
    sendButtonRelease(button, id);
}

void sendKeyPress(int keyCode, MasterID id) {
    XDevice dev = {.device_id = id};
    XTestFakeDeviceKeyEvent(dpy, &dev, keyCode, 1, NULL, 0, CurrentTime);
}
void sendKeyRelease(int keyCode, MasterID id) {
    XDevice dev = {.device_id = id};
    XTestFakeDeviceKeyEvent(dpy, &dev, keyCode, 0, NULL, 0, CurrentTime);
}
void typeKey(int keycode, MasterID id) {
    sendKeyPress(keycode, id);
    sendKeyRelease(keycode, id);
}

void movePointerRelative(short x, short y, MasterID id) {
    movePointer(x, y, XCB_NONE, id);
}
void movePointer(short x, short y, WindowID relativeWindow, MasterID id) {
    LOG(LOG_LEVEL_TRACE, "moving pointer : id %d to %d, %d relative to %d ", id, x, y, relativeWindow);
    xcb_input_xi_warp_pointer(dis, None, relativeWindow, 0, 0, 0, 0, x << 16, y << 16, id);
}
