#include <xcb/xtest.h>

#include "test-functions.h"

#include "globals.h"
#include "mywm-util.h"
#include "logger.h"

void sendDeviceAction(int id,int detail,int type){
    LOG(LOG_LEVEL_TRACE,"Sending action: %d %d %d\n",id,detail,type);
    catchError(xcb_test_fake_input_checked(dis, type, detail, XCB_CURRENT_TIME, XCB_NONE, 0, 0, id));
}

void sendButtonPress(int button){
    sendDeviceAction(getActiveMasterPointerID(),button,XCB_INPUT_BUTTON_PRESS);
}
void sendButtonRelease(int button){
    sendDeviceAction(getActiveMasterPointerID(),button,XCB_INPUT_BUTTON_RELEASE);
}
void clickButton(int button){
    sendButtonPress(button);
    sendButtonRelease(button);
}

void sendKeyPress(int keycode){
    sendDeviceAction(getActiveMasterKeyboardID(),keycode,XCB_INPUT_KEY_PRESS);
}
void sendKeyRelease(int keycode){
    sendDeviceAction(getActiveMasterKeyboardID(),keycode,XCB_INPUT_KEY_RELEASE);
}
void typeKey(int keycode){
    sendKeyPress(keycode);
    sendKeyRelease(keycode);
}
