#include "test-x-helper.h"
#include "tester.h"
#include "../system.h"
#include "../arraylist.h"
#include "../globals.h"
#include "../logger.h"

#include "../test-functions.h"
#include "../xsession.h"
#include "../globals.h"
#include "../devices.h"
#include "../device-grab.h"
#include "../user-events.h"
#include <xcb/xcb.h>
#include <xcb/xinput.h>
#include <xcb/xcb_ewmh.h>

int keyDetail = 65;
int mouseDetail = 1;

static int checkDeviceEventMatchesType(void* e, int type, int detail) {
    xcb_input_key_press_event_t* event = (xcb_input_key_press_event_t*)e;
    int result = event->event_type == type && event->detail == detail;
    free(event);
    return result;
}
static void setup(void) {
    int mask = XCB_INPUT_XI_EVENT_MASK_KEY_PRESS | XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE |
               XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS | XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE | XCB_INPUT_XI_EVENT_MASK_MOTION;
    openXDisplay();
    addDefaultMaster();
    passiveGrab(root, mask);
}
SET_ENV(setup, cleanupXServer);
void* getNextDeviceEvent() {
    flush();
    void* event = xcb_wait_for_event(dis);
    LOG(LOG_LEVEL_DEBUG, "Event received%d\n\n", ((xcb_generic_event_t*)event)->response_type);
    if(((xcb_generic_event_t*)event)->response_type == XCB_GE_GENERIC) {
        loadGenericEvent((xcb_ge_generic_event_t*)event);
        xcb_input_key_press_event_t* dEvent = (xcb_input_key_press_event_t*)event;
        LOG(LOG_LEVEL_DEBUG, "Detail %d type %s @ (%d,%d)\n", dEvent->detail,
            eventTypeToString(GENERIC_EVENT_OFFSET + dEvent->event_type), dEvent->root_x >> 16, dEvent->root_y >> 16);
    }
    return event;
}
void waitToReceiveInput(int mask, int detailMask) {
    LOG(LOG_LEVEL_ALL, "waiting for input %d\n\n", mask);
    while(mask || detailMask) {
        xcb_input_key_press_event_t* e = (xcb_input_key_press_event_t*)getNextDeviceEvent();
        LOG(LOG_LEVEL_ALL, "type %d\n", e->response_type);
        LOG(LOG_LEVEL_ALL, "type %d (%d); detail %d remaining mask:%d %d\n", e->event_type, (1 << e->event_type), e->detail,
            mask, detailMask);
        mask &= ~(1 << e->event_type);
        detailMask &= ~(1 << e->detail);
        free(e);
        msleep(10);
    }
}

MPX_TEST("test_send_key_press", {
    sendKeyPress(keyDetail);
    assert(checkDeviceEventMatchesType(getNextDeviceEvent(), XCB_INPUT_KEY_PRESS, keyDetail));
});
MPX_TEST("test_send_key_release", {
    sendKeyPress(keyDetail);
    sendKeyRelease(keyDetail);
    WAIT_UNTIL_TRUE(checkDeviceEventMatchesType(getNextDeviceEvent(), XCB_INPUT_KEY_RELEASE, keyDetail));
});


MPX_TEST("test_send_button_press", {
    sendButtonPress(mouseDetail);
    assert(checkDeviceEventMatchesType(getNextDeviceEvent(), XCB_INPUT_BUTTON_PRESS, mouseDetail));
});

MPX_TEST("test_send_button_release", {
    sendButtonPress(mouseDetail);
    sendButtonRelease(mouseDetail);
    WAIT_UNTIL_TRUE(checkDeviceEventMatchesType(getNextDeviceEvent(), XCB_INPUT_BUTTON_RELEASE, mouseDetail));
});

MPX_TEST_ITER("test_all_button", 8, {
    int detail = _i + 1;
    sendButtonPress(detail);
    assert(checkDeviceEventMatchesType(getNextDeviceEvent(), XCB_INPUT_BUTTON_PRESS, detail));
});


MPX_TEST_ITER("test_click_button", 3, {
    clickButton(mouseDetail);
    WAIT_UNTIL_TRUE(checkDeviceEventMatchesType(getNextDeviceEvent(), XCB_INPUT_BUTTON_PRESS, mouseDetail));
    WAIT_UNTIL_TRUE(checkDeviceEventMatchesType(getNextDeviceEvent(), XCB_INPUT_BUTTON_RELEASE, mouseDetail));
});

MPX_TEST("test_type_key", {
    typeKey(keyDetail);
    WAIT_UNTIL_TRUE(checkDeviceEventMatchesType(getNextDeviceEvent(), XCB_INPUT_KEY_PRESS, keyDetail));
    WAIT_UNTIL_TRUE(checkDeviceEventMatchesType(getNextDeviceEvent(), XCB_INPUT_KEY_RELEASE, keyDetail));
});


MPX_TEST("test_move_pointer", {
    short pos[][2] = {{10, 11}, {100, 101}};
    assert(!consumeEvents());
    for(int i = 0; i < LEN(pos); i++) {
        movePointer(pos[i][0], pos[i][1]);
        auto* e = (xcb_input_motion_event_t*)getNextDeviceEvent();
        assert(pos[i][0] == e->root_x >> 16);
        assert(pos[i][1] == e->root_y >> 16);
        free(e);
        consumeEvents();
    }
});


