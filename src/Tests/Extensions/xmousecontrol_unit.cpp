#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>

#include "../../logger.h"
#include "../../globals.h"
#include "../../wmfunctions.h"
#include "../../functions.h"
#include "../../devices.h"
#include "../../Extensions/xmousecontrol.h"
#include "../tester.h"
#include "../test-mpx-helper.h"
#include "../test-event-helper.h"
#include "../test-x-helper.h"

static void setup(void) {
    CRASH_ON_ERRORS = -1;
    XMOUSE_CONTROL_UPDATER_INTERVAL = 10;
    ROOT_DEVICE_EVENT_MASKS |= XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS | XCB_INPUT_XI_EVENT_MASK_MOTION;
    addStartXMouseControlRule();
    onSimpleStartup();
}
static void cleanup() {
    requestShutdown();
    addXMouseControlMask(1);
    removeXMouseControlMask(1);
    fullCleanup();
}

SET_ENV(setup, cleanup);
MPX_TEST("test_scroll", {
    int masks[] = {
        SCROLL_UP_MASK, SCROLL_DOWN_MASK, SCROLL_LEFT_MASK, SCROLL_RIGHT_MASK,
        SCROLL_UP_MASK | SCROLL_LEFT_MASK, SCROLL_DOWN_MASK | SCROLL_RIGHT_MASK,
        SCROLL_UP_MASK | SCROLL_DOWN_MASK, SCROLL_LEFT_MASK | SCROLL_RIGHT_MASK
    };
    int NORMAL_MASK_LEN = 6;
    for(int i = 0; i < LEN(masks); i++) {
        addXMouseControlMask(masks[i]);
        if(i < NORMAL_MASK_LEN)
            waitToReceiveInput(XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS, masks[i]);
        else {
            msleep(100);
            assert(consumeEvents() == 0);
        }
        removeXMouseControlMask(masks[i]);
        consumeEvents();
    }
});
MPX_TEST("test_mouse", {
    int masks[] = {
        MOVE_UP_MASK, MOVE_DOWN_MASK, MOVE_LEFT_MASK, MOVE_RIGHT_MASK,
        MOVE_UP_MASK | MOVE_LEFT_MASK, MOVE_DOWN_MASK | MOVE_RIGHT_MASK,
        MOVE_UP_MASK | MOVE_DOWN_MASK, MOVE_LEFT_MASK | MOVE_RIGHT_MASK
    };
    int NORMAL_MASK_LEN = 6;
    for(int i = 0; i < LEN(masks); i++) {
        addXMouseControlMask(masks[i]);
        if(i < NORMAL_MASK_LEN) {
            xcb_input_xi_query_pointer_reply_t* reply1 =
                xcb_input_xi_query_pointer_reply(dis, xcb_input_xi_query_pointer(dis, root, getActiveMasterPointerID()), NULL);
            waitToReceiveInput(XCB_INPUT_XI_EVENT_MASK_MOTION, 0);
            waitToReceiveInput(XCB_INPUT_XI_EVENT_MASK_MOTION, 0);
            xcb_input_xi_query_pointer_reply_t* reply2 =
                xcb_input_xi_query_pointer_reply(dis, xcb_input_xi_query_pointer(dis, root, getActiveMasterPointerID()), NULL);
            assert(((masks[i] & (MOVE_UP_MASK | MOVE_DOWN_MASK)) ? 1 : 0) == (reply1->root_y != reply2->root_y));
            assert(((masks[i] & (MOVE_LEFT_MASK | MOVE_RIGHT_MASK)) ? 1 : 0) == (reply1->root_x != reply2->root_x));
            free(reply1);
            free(reply2);
        }
        else {
            msleep(100);
            assert(consumeEvents() == 0);
        }
        removeXMouseControlMask(masks[i]);
        consumeEvents();
    }
});
MPX_TEST("test_mouse_remove", {
    lock();
    createMasterDevice("test1");
    flush();
    initCurrentMasters();
    for(Master* master : getAllMasters()) {
        setActiveMaster(master);
        addXMouseControlMask(SCROLL_UP_MASK);
    }
    unlock();
    msleep(XMOUSE_CONTROL_UPDATER_INTERVAL * 2);
    ATOMIC(destroyAllNonDefaultMasters());
    flush();
    initCurrentMasters();
    WAIT_UNTIL_TRUE(getAllMasters().size() == 1);
});
SET_ENV(onSimpleStartup, fullCleanup);
MPX_TEST("test_scroll_scale", {
    passiveGrab(root, XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS);
    addXMouseControlMask(SCROLL_UP_MASK);
    int scaleFactor = 4;
    adjustScrollSpeed(0);
    xmousecontrolUpdate();
    int num = consumeEvents();
    assert(num);
    adjustScrollSpeed(scaleFactor);
    xmousecontrolUpdate();
    int num2 = consumeEvents();
    assertEquals(num2, num + scaleFactor);
    adjustScrollSpeed(-scaleFactor);
    xmousecontrolUpdate();
    int num3 = consumeEvents();
    assertEquals(num3, num);
});
MPX_TEST("test_move_scale", {
    grabPointer();
    grabKeyboard();
    addXMouseControlMask(MOVE_DOWN_MASK | MOVE_RIGHT_MASK);
    int displacement = 10;
    adjustSpeed(0);
    xmousecontrolUpdate();

    short result[2] = {0, 0};
    getMousePosition(getActiveMasterPointerID(), root, result);
    waitToReceiveInput(1 << XCB_INPUT_MOTION, 0);
    consumeEvents();
    adjustSpeed(displacement - 1);
    xmousecontrolUpdate();
    waitToReceiveInput(1 << XCB_INPUT_MOTION, 0);
    consumeEvents();

    short result2[2] = {0, 0};
    getMousePosition(getActiveMasterPointerID(), root, result2);

    assertEquals(result2[0] - result[0], displacement);
    assertEquals(result2[1] - result[1], displacement);
});
MPX_TEST_ITER("test_mpx_aware", 3 * 2, {
    createMasterDevice("test");
    initCurrentMasters();
    startWM();
    waitUntilIdle();
    static Master* targetMaster = getAllMasters()[_i % getAllMasters().size()];
    getEventRules(DEVICE_EVENT).add({[]() {incrementCount(); assertEquals(*targetMaster, *getActiveMaster());}, "_masterCheck"});
    setActiveMaster(targetMaster);
    bool move = _i % 2;
    if(move)
        addXMouseControlMask(MOVE_DOWN_MASK | MOVE_RIGHT_MASK);
    else
        addXMouseControlMask(SCROLL_UP_MASK);
    setActiveMaster(getAllMasters()[0]);
    passiveGrab(root, POINTER_MASKS);
    ATOMIC(xmousecontrolUpdate());
    waitUntilIdle();
    if(move)
        assertEquals(getCount(), 1);
    else
        assertEquals(getCount(), 2);
    assertEquals(*getActiveMaster(), *targetMaster);
});
MPX_TEST("warm_bindings", {
    addDefaultXMouseControlBindings();
    const UserEvent& event = {0};
    for(auto b : getDeviceBindings())
        b->trigger(event);
});
