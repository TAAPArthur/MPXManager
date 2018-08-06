#include "../UnitTests.h"
#include "../TestX11Helper.h"

#include "../../test-functions.h"
#include "../../xsession.h"
#include "../../defaults.h"
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

static void setup(){

    ROOT_DEVICE_EVENT_MASKS|=XCB_INPUT_XI_EVENT_MASK_KEY_PRESS|XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE|
            XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS|XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE;
    for(int i=0;i<4;i++)
        deviceBindingLengths[i]=0;
    createContextAndConnection();
    passiveGrab(root, ROOT_DEVICE_EVENT_MASKS);
}

Suite *testFunctionSuite() {
    Suite*s = suite_create("Test Events");
    TCase* tc_core = tcase_create("Test Events");
    tcase_add_checked_fixture(tc_core, setup, closeConnection);
    tcase_add_test(tc_core, test_send_key_press);
    tcase_add_test(tc_core, test_send_key_release);
    tcase_add_test(tc_core, test_send_button_press);
    tcase_add_test(tc_core, test_send_button_release);
    suite_add_tcase(s, tc_core);
    return s;
}
