#include "../communications.h"
#include "../extra-rules.h"
#include "../globals.h"
#include "../logger.h"
#include "../wmfunctions.h"

#include "tester.h"
#include "test-event-helper.h"
#include "test-x-helper.h"

MPX_TEST("find_option", {
    assert(findOption("poll-count", "1"));
});
MPX_TEST("call_option_string", {
    std::string fakeShell = "test";
    findOption("shell", fakeShell)->call(fakeShell);
    assertEquals(SHELL, fakeShell);
});
MPX_TEST("call_option_int", {
    POLL_COUNT = 10;
    findOption("poll-count", "1")->call("1");
    assertEquals(POLL_COUNT, 1);
});
static void checkAndSend(std::string name, std::string value) {
    assert(findOption(name, value));
    send(name, value);
    flush();
}
static void setup() {
    enableInterClientCommunication();
    onSimpleStartup();
    saveXSession();
    atexit(fullCleanup);
}
SET_ENV(setup, fullCleanup);

MPX_TEST("test_send_receive_self", {
    POLL_COUNT = 10;
    checkAndSend("poll-count", "0");
    assert(hasOutStandingMessages());
    startWM();
    waitUntilIdle();
    assertEquals(POLL_COUNT, 0);
    assert(!hasOutStandingMessages());
    assertEquals(getLastMessageExitCode(), 1);
});
MPX_TEST("test_send_receive_failed_exit_code", {
    std::string name = "name";
    const int returnValue = 1;
    getOptions().add({name, +[]{return returnValue ;}});
    checkAndSend(name, "");
    assert(hasOutStandingMessages());
    startWM();
    waitUntilIdle();
    assert(!hasOutStandingMessages());
    assertEquals(returnValue, getLastMessageExitCode());
});

MPX_TEST("touch_fork_options", {
    CRASH_ON_ERRORS = 0;
    suppressOutput();
    for(Option* option : getOptions()) {
        if((option->getFlags() & CONFIRM_EARLY) == 0)
            option->call("1");
    }
});

MPX_TEST("test_send_receive", {
    POLL_COUNT = 10;
    SHELL = "";
    CRASH_ON_ERRORS = -1;
    KILL_TIMEOUT = 0;
    STEAL_WM_SELECTION = 0;
    if(!fork()) {
        RUN_AS_WM = 0;
        saveXSession();
        setup();
        checkAndSend("poll-count", "2");
        checkAndSend("steal-wm-selection", "1");
        checkAndSend("kill-timeout", "1000");
        checkAndSend("shell", "shell");
        checkAndSend("crash-on-errors", "0");
        WAIT_UNTIL_TRUE(!hasOutStandingMessages());
        send("__bad__option__", "0");
        flush();
        fullCleanup();
        quit(0);
    }
    startWM();
    WAIT_UNTIL_TRUE(CRASH_ON_ERRORS == 0);
    assertEquals(POLL_COUNT, 2);
    assertEquals(STEAL_WM_SELECTION, 1);
    assertEquals(KILL_TIMEOUT, 1000);
    assertEquals(SHELL, "shell");
    assertEquals(waitForChild(0), 0);
});

MPX_TEST("test_send_receive_func", {
    suppressOutput();
    POLL_COUNT = 1;
    checkAndSend("list-options", "");
    checkAndSend("dump", "");
    checkAndSend("dump", "0");
    checkAndSend("dump", "test");
    checkAndSend("load", "0");
    checkAndSend("dump-options", "");
    checkAndSend("log-level", "0");
    flush();
    addShutdownOnIdleRule();
    runEventLoop();
    assertEquals(getLogLevel(), 0);
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
    runEventLoop();
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
    runEventLoop();
});
MPX_TEST("test_quit", {
    if(!fork()) {
        runEventLoop();
    }
    openXDisplay();
    send("quit", "");
    flush();
    assert(waitForChild(0) == 0);
});


