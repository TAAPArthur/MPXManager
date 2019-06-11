#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>

#include "../UnitTests.h"
#include "../TestX11Helper.h"
#include "../../logger.h"
#include "../../globals.h"
#include "../../wmfunctions.h"
#include "../../functions.h"
#include "../../devices.h"
#include "../../Extensions/xmousecontrol.h"

void setup(void){
    CRASH_ON_ERRORS = -1;
    XMOUSE_CONTROL_UPDATER_INTERVAL = 10;
    runInNewThread(runXMouseControl, NULL, "xmousecontrol");
    ROOT_DEVICE_EVENT_MASKS |= XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS | XCB_INPUT_XI_EVENT_MASK_MOTION;
    enableXMouseControl();
    onStartup();
    START_MY_WM;
}
START_TEST(test_scroll){
    int masks[] = {
        SCROLL_UP_MASK, SCROLL_DOWN_MASK, SCROLL_LEFT_MASK, SCROLL_RIGHT_MASK,
        SCROLL_UP_MASK | SCROLL_LEFT_MASK, SCROLL_DOWN_MASK | SCROLL_RIGHT_MASK,
        SCROLL_UP_MASK | SCROLL_DOWN_MASK, SCROLL_LEFT_MASK | SCROLL_RIGHT_MASK
    };
    int NORMAL_MASK_LEN = 6;
    for(int i = 0; i < LEN(masks); i++){
        addXMouseControlMask(masks[i]);
        if(i < NORMAL_MASK_LEN)
            ATOMIC(waitToReceiveInput(XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS, masks[i]));
        else {
            msleep(100);
            assert(consumeEvents() == 0);
        }
        removeXMouseControlMask(masks[i]);
        consumeEvents();
    }
}
END_TEST
START_TEST(test_mouse){
    int masks[] = {
        MOVE_UP_MASK, MOVE_DOWN_MASK, MOVE_LEFT_MASK, MOVE_RIGHT_MASK,
        MOVE_UP_MASK | MOVE_LEFT_MASK, MOVE_DOWN_MASK | MOVE_RIGHT_MASK,
        MOVE_UP_MASK | MOVE_DOWN_MASK, MOVE_LEFT_MASK | MOVE_RIGHT_MASK
    };
    int NORMAL_MASK_LEN = 6;
    for(int i = 0; i < LEN(masks); i++){
        addXMouseControlMask(masks[i]);
        if(i < NORMAL_MASK_LEN){
            xcb_input_xi_query_pointer_reply_t* reply1 =
                xcb_input_xi_query_pointer_reply(dis, xcb_input_xi_query_pointer(dis, root, getActiveMasterPointerID()), NULL);
            ATOMIC(waitToReceiveInput(XCB_INPUT_XI_EVENT_MASK_MOTION, 0));
            ATOMIC(waitToReceiveInput(XCB_INPUT_XI_EVENT_MASK_MOTION, 0));
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
}
END_TEST
START_TEST(test_mouse_remove){
    createMasterDevice("test1");
    flush();
    WAIT_UNTIL_TRUE(getSize(getAllMasters()) == 2);
    lock();
    FOR_EACH(Master*, master, getAllMasters()){
        setActiveMaster(master);
        addXMouseControlMask(SCROLL_UP_MASK);
    }
    unlock();
    msleep(XMOUSE_CONTROL_UPDATER_INTERVAL * 2);
    ATOMIC(destroyAllNonDefaultMasters());
    flush();
    WAIT_UNTIL_TRUE(getSize(getAllMasters()) == 1);
}
END_TEST
START_TEST(test_scroll_scale){
    addXMouseControlMask(SCROLL_UP_MASK);
    int scaleFactor = 100;
    adjustScrollSpeed(0);
    xmousecontrolUpdate();
    int num = consumeEvents();
    assert(num);
    adjustScrollSpeed(scaleFactor);
    xmousecontrolUpdate();
    int num2 = consumeEvents();
    assert(num2 >= num * scaleFactor / 2);
    adjustScrollSpeed(-scaleFactor);
    xmousecontrolUpdate();
    int num3 = consumeEvents();
    assert(num3 <= num * scaleFactor / 2);
}
END_TEST
START_TEST(test_move_scale){
    grabActivePointer();
    grabActiveKeyboard();
    addXMouseControlMask(MOVE_DOWN_MASK | MOVE_RIGHT_MASK);
    int scaleFactor = 10;
    int displacement = 10;
    adjustSpeed(0);
    xmousecontrolUpdate();
    waitToReceiveInput(1 << XCB_INPUT_MOTION, 0);
    consumeEvents();
    adjustSpeed(displacement);
    xmousecontrolUpdate();
    waitToReceiveInput(1 << XCB_INPUT_MOTION, 0);
    consumeEvents();
    short result[2];
    getMouseDelta(getActiveMaster(), result);
    assert(result[0] == displacement);
    assert(result[1] == displacement);
    adjustSpeed(scaleFactor);
    xmousecontrolUpdate();
    waitToReceiveInput(1 << XCB_INPUT_MOTION, 0);
    getMouseDelta(getActiveMaster(), result);
    assert(result[0] == displacement * scaleFactor);
    assert(result[1] == displacement * scaleFactor);
}
END_TEST
Suite* xmousecontrolSuite(){
    Suite* s = suite_create("XMouseControl");
    TCase* tc_core;
    tc_core = tcase_create("XMouseControl");
    tcase_add_checked_fixture(tc_core, setup, fullCleanup);
    tcase_add_test(tc_core, test_scroll);
    tcase_add_test(tc_core, test_mouse);
    tcase_add_test(tc_core, test_mouse_remove);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("XMouseControlScale");
    tcase_add_checked_fixture(tc_core, onStartup, fullCleanup);
    tcase_add_test(tc_core, test_scroll_scale);
    tcase_add_test(tc_core, test_move_scale);
    suite_add_tcase(s, tc_core);
    return s;
}
