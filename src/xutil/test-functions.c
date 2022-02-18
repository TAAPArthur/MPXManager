#include <X11/extensions/XInput2.h>
#include <X11/extensions/XTest.h>
#include <xcb/xinput.h>
#include <xcb/xtest.h>
#include <xcb/xtest.h>

#include "../masters.h"
#include "../util/logger.h"
#include "test-functions.h"
#include "xsession.h"


static void sendEvent(int type, int detail, MasterID id) {
    const xcb_query_extension_reply_t * reply = xcb_get_extension_data(dis, &xcb_input_id);
    xcb_test_fake_input_checked(dis, reply->first_event + type - XCB_INPUT_KEY_PRESS + 1, detail, XCB_CURRENT_TIME, root, 0,0, id);
}
void sendButtonPress(int button, MasterID id) {
    sendEvent(XCB_INPUT_BUTTON_PRESS, button, id);
}
void sendButtonRelease(int button, MasterID id) {
    sendEvent(XCB_INPUT_BUTTON_RELEASE, button, id);
}
void clickButton(int button, MasterID id) {
    sendButtonPress(button, id);
    sendButtonRelease(button, id);
}

void sendKeyPress(int keyCode, MasterID id) {
    sendEvent(XCB_INPUT_KEY_PRESS, keyCode, id);
}
void sendKeyRelease(int keyCode, MasterID id) {
    sendEvent(XCB_INPUT_KEY_RELEASE, keyCode, id);
}
void typeKey(int keycode, MasterID id) {
    sendKeyPress(keycode, id);
    sendKeyRelease(keycode, id);
}

void warpPointer(short x, short y, WindowID relativeWindow, MasterID id) {
    TRACE("moving pointer : id %d to %d, %d relative to %d ", id, x, y, relativeWindow);
    xcb_input_xi_warp_pointer(dis, None, relativeWindow, 0, 0, 0, 0, x << 16, y << 16, id);
}

void movePointer(short x, short y) {
    warpPointer(x, y, root, getActiveMasterPointerID());
}
void movePointerRelative(short x, short y) {
    warpPointer(x, y, XCB_NONE, getActiveMasterPointerID());
}
