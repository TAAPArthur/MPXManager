#include <xcb/xinput.h>
#include <xcb/xtest.h>
#include <X11/extensions/XInput2.h>

#include "logger.h"
#include "masters.h"
#include "test-functions.h"
#include "xsession.h"

void sendDeviceAction(MasterID id, int detail, int type, WindowID window) {
    LOG(LOG_LEVEL_DEBUG, "Sending action: id %d detail %d type %d to win %d\n", id, detail, type, window);
    assert(detail);
    assert(id < getMaxNumberOfDevices());
    catchError(xcb_test_fake_input_checked(dis, type, detail, XCB_CURRENT_TIME, window, 0, 0, id));
}

void sendButtonPress(int button, MasterID id, WindowID win) {
    sendDeviceAction(id, button, XCB_INPUT_BUTTON_PRESS, win);
}
void sendButtonRelease(int button, MasterID id, WindowID win) {
    sendDeviceAction(id, button, XCB_INPUT_BUTTON_RELEASE, win);
}
void clickButton(int button, MasterID id, WindowID win) {
    sendButtonPress(button, id, win);
    sendButtonRelease(button, id, win);
}

void sendKeyPress(int keycode, MasterID id, WindowID win) {
    sendDeviceAction(id, keycode, XCB_INPUT_KEY_PRESS, win);
}
void sendKeyRelease(int keycode, MasterID id, WindowID win) {
    sendDeviceAction(id, keycode, XCB_INPUT_KEY_RELEASE, win);
}
void typeKey(int keycode, MasterID id, WindowID win) {
    sendKeyPress(keycode, id, win);
    sendKeyRelease(keycode, id, win);
}


void movePointerRelative(short x, short y, MasterID id) {
    movePointer(x, y, id, XCB_NONE);
}
void movePointer(short x, short y, MasterID id, WindowID relativeWindow) {
    LOG(LOG_LEVEL_VERBOSE, "moving pointer : id %d to %d, %d relative to %d \n", id, x, y, relativeWindow);
    xcb_input_xi_warp_pointer(dis, None, relativeWindow, 0, 0, 0, 0, x << 16, y << 16, id);
}
