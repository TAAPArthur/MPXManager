#include <xcb/xtest.h>
#include <xcb/xinput.h>

#include "test-functions.h"
#include "defaults.h"
#include "mywm-util.h"

void sendDeviceAction(int id,int detail,int type){
    xcb_test_fake_input(dis, type, detail, XCB_CURRENT_TIME, XCB_NONE, 0, 0, id);
}

void sendButtonPress(int button){
    sendDeviceAction(getActiveMasterPointerID(),button,XCB_INPUT_BUTTON_PRESS);
}
void sendButtonRelease(int button){
    sendDeviceAction(getActiveMasterPointerID(),button,XCB_INPUT_BUTTON_RELEASE);
}
void sendKeyPress(int keycode){
    sendDeviceAction(getActiveMasterKeyboardID(),keycode,XCB_INPUT_KEY_PRESS);
}
void sendKeyRelease(int keycode){
    sendDeviceAction(getActiveMasterKeyboardID(),keycode,XCB_INPUT_KEY_RELEASE);
}
