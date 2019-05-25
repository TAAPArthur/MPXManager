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
#include "../../communications.h"
#include "../../Extensions/mpx.h"

static void setup(){
    enableInterClientCommunication();
    loadDefaultOptions();
    onStartup();
}
START_TEST(test_send_receive_self){
    POLL_COUNT = 100;
    START_MY_WM;
    send("POLL_COUNT", "0");
    flush();
    WAIT_UNTIL_TRUE(POLL_COUNT == 0);
}
END_TEST
START_TEST(test_send_receive){
    POLL_COUNT = 100;
    SYNC_FOCUS = 0;
    SHELL = NULL;
    CRASH_ON_ERRORS = -1;
    KILL_TIMEOUT = 0;
    if(!fork()){
        openXDisplay();
        send("POLL_COUNT", "0");
        send("SYNC_FOCUS", "1");
        send("SHELL", "shell");
        send("KILL_TIMEOUT", "100000000000");
        send("CRASH_ON_ERRORS", "0");
        send("__bad__option__", "0");
        flush();
        exit(0);
    }
    waitForCleanExit();
    START_MY_WM;
    WAIT_UNTIL_TRUE(CRASH_ON_ERRORS == 0);
    assert(POLL_COUNT == 0);
    assert(SYNC_FOCUS == 1);
    assert(strcmp(SHELL, "shell") == 0);
    assert(KILL_TIMEOUT == 100000000000L);
}
END_TEST
START_TEST(test_send_receive_func){
    POLL_COUNT = 100;
    send("list-options", "0");
    send("dump", "0");
    send("dump-win", NULL);
    send("dump-win", "0");
    send("dump-all", NULL);
    send("dump-filter", "test");
    send("log-level", "10");
    int idle = getIdleCount();
    START_MY_WM;
    flush();
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    assert(getLogLevel() == 10);
}
END_TEST
START_TEST(test_quit){
    if(!fork()){
        runEventLoop(NULL);
    }
    openXDisplay();
    send("quit", NULL);
    flush();
    waitForCleanExit();
}
END_TEST
Suite* communicationsSuite(){
    Suite* s = suite_create("Communications");
    TCase* tc_core;
    tc_core = tcase_create("SendReceive");
    tcase_add_checked_fixture(tc_core, setup, fullCleanup);
    tcase_add_test(tc_core, test_send_receive_self);
    tcase_add_test(tc_core, test_send_receive);
    tcase_add_test(tc_core, test_send_receive_func);
    tcase_add_test(tc_core, test_quit);
    suite_add_tcase(s, tc_core);
    return s;
}


