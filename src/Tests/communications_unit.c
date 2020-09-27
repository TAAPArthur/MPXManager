#include "tester.h"
#include "test-event-helper.h"
#include "../communications.h"
#include "../wmfunctions.h"

static void checkAndSend(const char* name, const char* value) {
    assert(findOption(name, value));
    send(name, value);
    flush();
}
static void setup() {
    addInterClientCommunicationRule();
    initOptions();
    createXSimpleEnv();
    registerForWindowEvents(root, XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT);
}
SCUTEST_SET_ENV(setup, cleanupXServer);

SCUTEST(test_send_receive_func) {
    POLL_COUNT = 1;
    checkAndSend("dump", "");
    checkAndSend("dump", "0");
    checkAndSend("dump", "test");
    checkAndSend("log-level", "0");
    flush();
    addShutdownOnIdleRule();
    runEventLoop();
    assertEquals(getLogLevel(), 0);
    assert(!hasOutStandingMessages());
}

SCUTEST(bad_pid) {
    CRASH_ON_ERRORS = 0;
    catchError(xcb_ewmh_set_wm_pid_checked(ewmh, getPrivateWindow(), -1));
    send("dump", "");
    addShutdownOnIdleRule();
    runEventLoop();
}


SCUTEST(test_bad_options) {
    CRASH_ON_ERRORS = 0;
    suppressOutput();
    send("bad_option", "0");
    const char* optionValues[] = {"CRASH_ON_ERRORS", "dump", "bad"};
    for(int n = 0; n < 2; n++) {
        unsigned int data[5] = {0};
        catchError(xcb_ewmh_send_client_message(dis, root, root, MPX_WM_INTERPROCESS_COM, sizeof(data), data));
        data[0] = getAtom(optionValues[n]);
        catchError(xcb_ewmh_send_client_message(dis, root, root, MPX_WM_INTERPROCESS_COM, sizeof(data), data));
        for(int i = 1; i < LEN(data); i++) {
            data[i] = 1;
            catchError(xcb_ewmh_send_client_message(dis, root, root, MPX_WM_INTERPROCESS_COM, sizeof(data), data));
        }
    }
    flush();
    addShutdownOnIdleRule();
    runEventLoop();
}
