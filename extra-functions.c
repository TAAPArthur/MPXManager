/*
 * extra-functions.c
 *
 *  Created on: Jul 14, 2018
 *      Author: arthur
 */
#include <xcb/xtest.h>
#include <assert.h>

#include "mywm-util.h"
#include "wmfunctions.h"
#include "extra-functions.h"

void sendDeviceAction(int id,int detail,int type){

    xcb_void_cookie_t cookie=xcb_test_fake_input_checked(dis, type, detail, XCB_CURRENT_TIME, XCB_NONE, 0, 0, id);
    if(checkError(cookie, root, "could not send device action"))
        assert(0);
}
int getActivePointerId(){
    return getAssociatedMasterDevice(getActiveMaster()->id);
}
void sendButtonPress(int button){
    sendDeviceAction(getActivePointerId(),button,XCB_BUTTON_PRESS);
}
void sendButtonRelease(int button){
    sendDeviceAction(getActivePointerId(),button,XCB_BUTTON_RELEASE);
}
void sendKeyPress(int keycode){
    sendDeviceAction(getActiveMaster()->id,keycode,XCB_KEY_PRESS);
}
void sendKeyRelease(int keycode){
    sendDeviceAction(getActiveMaster()->id,keycode,XCB_KEY_RELEASE);
}
