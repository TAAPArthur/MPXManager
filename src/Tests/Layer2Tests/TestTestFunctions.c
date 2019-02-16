#include "../UnitTests.h"
#include "../TestX11Helper.h"

#include "../../test-functions.h"
#include "../../xsession.h"
#include "../../globals.h"
#include "../../devices.h"
#include <xcb/xcb.h>
#include <xcb/xinput.h>
#include <xcb/xcb_ewmh.h>

int keyDetail=65;
int mouseDetail=1;

static int checkDeviceEventMatchesType(xcb_input_key_press_event_t*event,int type){
    int result= event->event_type==type && event->detail==keyDetail||event->detail==mouseDetail;
    free(event);
    return result;
}

START_TEST(test_send_key_press){
    sendKeyPress(keyDetail);
    assert(checkDeviceEventMatchesType(getNextDeviceEvent(),XCB_INPUT_KEY_PRESS));
}END_TEST
START_TEST(test_send_key_release){
    sendKeyPress(keyDetail);
    sendKeyRelease(keyDetail);
    WAIT_UNTIL_TRUE(checkDeviceEventMatchesType(getNextDeviceEvent(),XCB_INPUT_KEY_RELEASE))
}END_TEST

START_TEST(test_send_button_press){
    sendButtonPress(mouseDetail);
    assert(checkDeviceEventMatchesType(getNextDeviceEvent(),XCB_INPUT_BUTTON_PRESS));
}END_TEST
START_TEST(test_send_button_release){
    sendButtonPress(mouseDetail);
    sendButtonRelease(mouseDetail);
    WAIT_UNTIL_TRUE(checkDeviceEventMatchesType(getNextDeviceEvent(),XCB_INPUT_BUTTON_RELEASE))
}END_TEST
START_TEST(test_click_button){
    clickButton(mouseDetail);
    WAIT_UNTIL_TRUE(checkDeviceEventMatchesType(getNextDeviceEvent(),XCB_INPUT_BUTTON_PRESS))
    WAIT_UNTIL_TRUE(checkDeviceEventMatchesType(getNextDeviceEvent(),XCB_INPUT_BUTTON_RELEASE))
}END_TEST
START_TEST(test_type_key){
    typeKey(keyDetail);
    WAIT_UNTIL_TRUE(checkDeviceEventMatchesType(getNextDeviceEvent(),XCB_INPUT_KEY_PRESS))
    WAIT_UNTIL_TRUE(checkDeviceEventMatchesType(getNextDeviceEvent(),XCB_INPUT_KEY_RELEASE))
}END_TEST

START_TEST(test_move_pointer){

    for(int i=0;i<10;i++){
        movePointer(getActiveMasterPointerID(), root, 10, 10);
        movePointer(getActiveMasterPointerID(), root, 100, 100);
        waitToReceiveInput(1<<XCB_INPUT_MOTION);
    }
}END_TEST

static void setup(){

    int mask=XCB_INPUT_XI_EVENT_MASK_KEY_PRESS|XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE|
            XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS|XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE|XCB_INPUT_XI_EVENT_MASK_MOTION;
    clearList(getDeviceBindings());
    createContextAndSimpleConnection();
    passiveGrab(root, mask);
}

Suite *testFunctionSuite() {
    Suite*s = suite_create("Test Functions");
    TCase* tc_core = tcase_create("Test Functions");
    tcase_add_checked_fixture(tc_core, setup, closeConnection);
    tcase_add_test(tc_core, test_send_key_press);
    tcase_add_test(tc_core, test_send_key_release);
    tcase_add_test(tc_core, test_send_button_press);
    tcase_add_test(tc_core, test_send_button_release);
    tcase_add_test(tc_core, test_type_key);
    tcase_add_test(tc_core, test_click_button);
    tcase_add_test(tc_core, test_move_pointer);
    suite_add_tcase(s, tc_core);
    return s;
}
