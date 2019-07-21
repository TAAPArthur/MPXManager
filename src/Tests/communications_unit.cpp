#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>
#include <fcntl.h>

#include "../logger.h"
#include "../globals.h"
#include "../wmfunctions.h"
#include "../extra-rules.h"
#include "../communications.h"

#include "tester.h"
#include "test-event-helper.h"
#include "test-x-helper.h"

static void setup() {
    enableInterClientCommunication();
    onStartup();
    saveXSession();
    atexit(fullCleanup);
}
SET_ENV(setup, fullCleanup);
MPX_TEST("test_send_receive_self", {
    POLL_COUNT = 10;
    send("POLL_COUNT", "0");
    flush();
    assert(hasOutStandingMessages());
    startWM();
    waitUntilIdle();
    assert(POLL_COUNT == 0);
    assert(!hasOutStandingMessages());
});

MPX_TEST("test_send_receive", {
    POLL_COUNT = 10;
    SYNC_FOCUS = 0;
    SHELL = "";
    CRASH_ON_ERRORS = -1;
    KILL_TIMEOUT = 0;
    if(!fork()) {
        RUN_AS_WM = 0;
        saveXSession();
        setup();
        send("POLL_COUNT", "0");
        send("SYNC_FOCUS", "1");
        send("SHELL", "shell");
        send("KILL_TIMEOUT", "1000");
        send("CRASH_ON_ERRORS", "0");
        WAIT_UNTIL_TRUE(!hasOutStandingMessages());
        send("__bad__option__", "0");
        flush();
        fullCleanup();
        quit(0);
    }
    startWM();
    WAIT_UNTIL_TRUE(CRASH_ON_ERRORS == 0);
    assert(POLL_COUNT == 0);
    assert(SYNC_FOCUS == 1);
    assert(SHELL == "shell");
    assert(KILL_TIMEOUT == 1000);
    assert(waitForChild(0) == 0);
});

MPX_TEST("test_send_receive_func", {
    POLL_COUNT = 1;
    suppressOutput();
    send("list-options", "0");
    send("dump", "0");
    send("dump", "test");
    send("log-level", "10");
    flush();
    addShutdownOnIdleRule();
    runEventLoop(NULL);
    assert(getLogLevel() == 10);
    assert(!hasOutStandingMessages());
});
MPX_TEST("test_stale", {
    if(!fork()) {
        openXDisplay();
        send("dump", "");
        send("quit", "");
        quit(0);
    }
    assert(waitForChild(0) == 0);
    CRASH_ON_ERRORS = 0;
    setLogLevel(LOG_LEVEL_NONE);
    runEventLoop(NULL);
});

MPX_TEST("test_bad_options", {
    CRASH_ON_ERRORS = 0;
    suppressOutput();
    send("bad_option", "0");
    const char* optionValues[] = {"CRASH_ON_ERRORS", "dump", "bad"};
    for(int n = 0; n < 2; n++) {
        unsigned int data[5] = {0};
        catchError(xcb_ewmh_send_client_message(dis, root, root, WM_INTERPROCESS_COM, sizeof(data), data));
        data[0] = getAtom(optionValues[n]);
        catchError(xcb_ewmh_send_client_message(dis, root, root, WM_INTERPROCESS_COM, sizeof(data), data));
        for(int i = 1; i < LEN(data); i++) {
            data[i] = 1;
            catchError(xcb_ewmh_send_client_message(dis, root, root, WM_INTERPROCESS_COM, sizeof(data), data));
        }
    }
    flush();
    addDieOnIdleRule();
    runEventLoop(NULL);
});
MPX_TEST("test_quit", {
    if(!fork()) {
        runEventLoop(NULL);
    }
    openXDisplay();
    send("quit", "");
    flush();
    assert(waitForChild(0) == 0);
});


