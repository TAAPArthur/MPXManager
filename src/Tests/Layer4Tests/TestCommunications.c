#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>
#include <fcntl.h>

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
    send("POLL_COUNT", "0");
    flush();
    assert(hasOutStandingMessages());
    START_MY_WM;
    WAIT_UNTIL_TRUE(POLL_COUNT == 0);
    WAIT_UNTIL_TRUE(!hasOutStandingMessages());
}
END_TEST
START_TEST(test_send_receive){
    POLL_COUNT = 100;
    SYNC_FOCUS = 0;
    SHELL = NULL;
    CRASH_ON_ERRORS = -1;
    KILL_TIMEOUT = 0;
    if(!fork()){
        RUN_AS_WM = 0;
        setup();
        send("POLL_COUNT", "0");
        send("SYNC_FOCUS", "1");
        send("SHELL", "shell");
        send("KILL_TIMEOUT", "100000000000");
        send("CRASH_ON_ERRORS", "0");
        send("__bad__option__", "0");
        flush();
        START_MY_WM;
        WAIT_UNTIL_TRUE(!hasOutStandingMessages());
        quit(0);
    }
    START_MY_WM;
    WAIT_UNTIL_TRUE(CRASH_ON_ERRORS == 0);
    assert(POLL_COUNT == 0);
    assert(SYNC_FOCUS == 1);
    assert(strcmp(SHELL, "shell") == 0);
    assert(KILL_TIMEOUT == 100000000000L);
    waitForCleanExit();
}
END_TEST
START_TEST(test_send_receive_func){
    POLL_COUNT = 1;
    assert(dup2(open("/dev/null", O_WRONLY | O_APPEND), LOG_FD) == LOG_FD);
    send("list-options", "0");
    send("dump", "0");
    send("dump-win", NULL);
    send("dump-win", "0");
    send("dump-all", NULL);
    send("dump-filter", "test");
    send("log-level", "10");
    close(0);
    flush();
    static Rule dieOnIdleRule = CREATE_WILDCARD(BIND(requestShutdown));
    addToList(getEventRules(Idle), &dieOnIdleRule);
    runEventLoop(NULL);
    assert(getLogLevel() == 10);
    WAIT_UNTIL_TRUE(!hasOutStandingMessages());
}
END_TEST
START_TEST(test_stale){
    if(!fork()){
        openXDisplay();
        send("dump", 0);
        send("quit", 0);
        quit(0);
    }
    waitForCleanExit();
    CRASH_ON_ERRORS = 0;
    setLogLevel(LOG_LEVEL_NONE);
    runEventLoop(NULL);
}
END_TEST
START_TEST(test_bad_options){
    setLogLevel(LOG_LEVEL_DEBUG);
    CRASH_ON_ERRORS = 0;
    assert(dup2(open("/dev/null", O_WRONLY | O_APPEND), LOG_FD) == LOG_FD);
    send("bad_option", "0");
    char* optionValues[] = {"DEFAULT_BORDER_COLOR", "dump", "bad"};
    for(int n = 0; n < 2; n++){
        unsigned int data[5] = {0};
        catchError(xcb_ewmh_send_client_message(dis, root, root, WM_INTERPROCESS_COM, sizeof(data), data));
        data[0] = getAtom(optionValues[n]);
        catchError(xcb_ewmh_send_client_message(dis, root, root, WM_INTERPROCESS_COM, sizeof(data), data));
        for(int i = 1; i < LEN(data); i++){
            data[i] = 1;
            catchError(xcb_ewmh_send_client_message(dis, root, root, WM_INTERPROCESS_COM, sizeof(data), data));
        }
    }
    flush();
    static Rule dieOnIdleRule = CREATE_WILDCARD(BIND(requestShutdown));
    addToList(getEventRules(Idle), &dieOnIdleRule);
    runEventLoop(NULL);
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
    tc_core = tcase_create("SendReceiveBad");
    tcase_add_checked_fixture(tc_core, setup, fullCleanup);
    tcase_add_test(tc_core, test_bad_options);
    tcase_add_test(tc_core, test_stale);
    suite_add_tcase(s, tc_core);
    return s;
}


