#include <xcb/xinput.h>
#include <xcb/xtest.h>
#include <X11/extensions/XInput2.h>

#include "logger.h"
#include "masters.h"
#include "test-functions.h"
#include "xsession.h"

void sendDeviceAction(MasterID id, int detail, int type, WindowID window) {
    LOG(LOG_LEVEL_DEBUG, "Sending action: id %d detail %d type %d to win %d", id, detail, type, window);
    assert(detail);
    assert(id < getMaxNumberOfDevices());
    XCALL(xcb_test_fake_input, dis, type, detail, XCB_CURRENT_TIME, window, 0, 0, id);
}

void sendButtonPress(int button, WindowID win, MasterID id) {
    sendDeviceAction(id, button, XCB_INPUT_BUTTON_PRESS, win);
}
void sendButtonRelease(int button, WindowID win, MasterID id) {
    sendDeviceAction(id, button, XCB_INPUT_BUTTON_RELEASE, win);
}
void clickButton(int button, WindowID win, MasterID id) {
    sendButtonPress(button, win, id);
    sendButtonRelease(button, win, id);
}

void sendKeyPress(int keycode, WindowID win, MasterID id) {
    sendDeviceAction(id, keycode, XCB_INPUT_KEY_PRESS, win);
}
void sendKeyRelease(int keycode, WindowID win, MasterID id) {
    sendDeviceAction(id, keycode, XCB_INPUT_KEY_RELEASE, win);
}
void typeKey(int keycode, WindowID win, MasterID id) {
    sendKeyPress(keycode, win, id);
    sendKeyRelease(keycode, win, id);
}


void movePointerRelative(short x, short y, MasterID id) {
    movePointer(x, y, XCB_NONE, id);
}
void movePointer(short x, short y, WindowID relativeWindow, MasterID id) {
    LOG(LOG_LEVEL_TRACE, "moving pointer : id %d to %d, %d relative to %d ", id, x, y, relativeWindow);
    xcb_input_xi_warp_pointer(dis, None, relativeWindow, 0, 0, 0, 0, x << 16, y << 16, id);
}
