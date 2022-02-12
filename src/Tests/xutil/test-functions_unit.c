#include "../../devices.h"
#include "../../globals.h"
#include "../../system.h"
#include "../../user-events.h"
#include "../../xutil/device-grab.h"
#include "../../xutil/test-functions.h"
#include "../../xutil/xsession.h"
#include "../test-event-helper.h"
#include "../tester.h"

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xinput.h>

int keyDetail = 65;
int mouseDetail = 1;

static int checkDeviceEventMatchesType(void* e, int type, int detail, MasterID id) {
    xcb_input_key_press_event_t* event = (xcb_input_key_press_event_t*)e;
    int result = event->event_type == type && event->detail == detail && event->deviceid == id;
    free(event);
    return result;
}

static int checkKeyEventMatchesType(int type, int detail, MasterID id) {
    for(int i = 0; i < 2; i++) {
        xcb_input_key_press_event_t* event = (xcb_input_key_press_event_t*)getNextDeviceEvent();
        int result = event->event_type == type && event->detail == detail;
        int device = event->deviceid;
        int source = event->sourceid;
        free(event);
        if(!result)
            return 0;
        if(device == id)
            return 1;
        if(device != source)
            return 0;
    }
    return 0;
}
static MasterID pointerIDs[2];
static MasterID keyboardIDs[2];
static void setup(void) {
    setLogLevel(LOG_LEVEL_TRACE);
    TRACE("trace");
    WARN("here");
    int mask = XCB_INPUT_XI_EVENT_MASK_KEY_PRESS | XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE |
        XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS | XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE | XCB_INPUT_XI_EVENT_MASK_MOTION;
    openXDisplay();
    addDefaultMaster();
    createMasterDevice("test");
    initCurrentMasters();
    assertEquals(getAllMasters()->size, 2);
    int i = 0;
    FOR_EACH(Master*, master, getAllMasters()) {
        pointerIDs[i] = getPointerID(master);
        keyboardIDs[i] = getKeyboardID(master);
        i++;
    }
    passiveGrab(root, mask);
}
SCUTEST_SET_ENV(setup, cleanupXServer);
SCUTEST_ITER(test_send_key_press_release, 4) {
    int i = _i / 2;
    if(_i % 2) {
        sendKeyPress(keyDetail, keyboardIDs[i]);
        sendKeyRelease(keyDetail, keyboardIDs[i]);
    }
    else
        typeKey(keyDetail, keyboardIDs[i]);
    assert(checkKeyEventMatchesType(XCB_INPUT_KEY_PRESS, keyDetail, keyboardIDs[i]));
    assert(checkKeyEventMatchesType(XCB_INPUT_KEY_RELEASE, keyDetail, keyboardIDs[i]));
}

SCUTEST_ITER(test_send_button_press_release, 4) {
    int i = _i / 2;
    if(_i % 2) {
        sendButtonPress(mouseDetail, pointerIDs[i]);
        sendButtonRelease(mouseDetail, pointerIDs[i]);
    }
    else
        clickButton(mouseDetail, pointerIDs[i]);
    assert(checkDeviceEventMatchesType(getNextDeviceEvent(), XCB_INPUT_BUTTON_PRESS, mouseDetail, pointerIDs[i]));
    assert(checkDeviceEventMatchesType(getNextDeviceEvent(), XCB_INPUT_BUTTON_RELEASE, mouseDetail, pointerIDs[i]));
}

SCUTEST_ITER(test_all_button, 8) {
    int detail = _i + 1;
    clickButton(detail, getActiveMasterPointerID());
    assert(checkDeviceEventMatchesType(getNextDeviceEvent(), XCB_INPUT_BUTTON_PRESS, detail, getActiveMasterPointerID()));
}

SCUTEST_ITER(test_move_pointer, 2) {
    short pos[][2] = {{10, 11}, {100, 101}};
    assert(!consumeEvents());
    setActiveMaster(getMasterByDeviceID(pointerIDs[_i]));
    for(int i = 0; i < LEN(pos); i++) {
        movePointer(pos[i][0], pos[i][1]);
        xcb_input_motion_event_t* e = getNextDeviceEvent();
        assertEquals(e->deviceid, getActiveMasterPointerID());
        assertEquals(pos[i][0], e->root_x >> 16);
        assertEquals(pos[i][1], e->root_y >> 16);
        free(e);
        short result[2];
        getMousePosition(getActiveMasterPointerID(), root, result);
        assertEquals(pos[i][0], result[0]);
        assertEquals(pos[i][1], result[1]);
        assert(!consumeEvents());
    }
}
SCUTEST_ITER(test_move_pointer_relative, 2) {
    setActiveMaster(getMasterByDeviceID(pointerIDs[_i]));
    assert(!consumeEvents());
    movePointer(0, 0);
    consumeEvents();
    short pos[2];
    for(int i = 1; i < 10; i++) {
        movePointerRelative(1, 2);
        getMousePosition(getActiveMasterPointerID(), root, pos);
        assertEquals(i, pos[0]);
        assertEquals(i * 2, pos[1]);
        consumeEvents();
    }
}
