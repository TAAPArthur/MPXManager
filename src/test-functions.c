/**
 * @file test-functions.c
 */

/// \cond
#include <xcb/xtest.h>
/// \endcond

#include "logger.h"
#include "masters.h"
#include "mywm-util.h"
#include "test-functions.h"
#include "xsession.h"

void sendDeviceActionToWindow(int id,int detail,int type,int window){
    catchError(xcb_test_fake_input_checked(dis, type, detail, XCB_CURRENT_TIME, window, 0, 0, id));
}
void sendDeviceAction(int id,int detail,int type){
    LOG(LOG_LEVEL_TRACE,"Sending action: id %d detail %d type %d\n",id,detail,type);
    sendDeviceActionToWindow(id,detail,type,XCB_NONE);
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

void movePointer(int id,int relativeWindow,int x,int y){
    xcb_input_xi_warp_pointer(dis, None, relativeWindow, 0, 0, 0, 0, x, y, id);
}
