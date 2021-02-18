#include "../Extensions/ewmh-client.h"
#include "../communications.h"
#include "../windows.h"
#include "../wmfunctions.h"
#include "../system.h"
#include "test-wm-helper.h"
#include "test-event-helper.h"
#include "tester.h"
#include <signal.h>


uint32_t wmPid;
static void cleanup() {
    assert(wmPid);
    kill(wmPid, SIGINT);
    WAIT_UNTIL_TRUE(!isMPXManagerRunning());
}

static void initSetup() {
    setenv("ALLOW_MPX_EXT", "1", 1);
    wmPid = spawnChild("./mpxmanager --log-level 0");
    createXSimpleEnv();
    WAIT_UNTIL_TRUE(isMPXManagerRunning());
    waitUntilWMIdle();
}
static void restartWM() {
    WindowID win = getWMPrivateWindow();
    setWindowPropertyInt(win, WM_STATE, XCB_ATOM_CARDINAL, 1);
    send("restart", "");
    WAIT_UNTIL_TRUE(!hasOutStandingMessages());
    WAIT_UNTIL_TRUE(!getWindowPropertyValueInt(win, WM_STATE, XCB_ATOM_CARDINAL));
    WAIT_UNTIL_TRUE(isMPXManagerRunning());
    WAIT_UNTIL_TRUE(getWMIdleCount());
}
SCUTEST(test_window_container_restart) {
    focusWindow(mapWindow(createNormalWindow()));
    waitUntilWMIdle();
    send("create-container", "");
    send("create-container", "");
    WAIT_UNTIL_TRUE(!hasOutStandingMessages());
    send("release-container", "");
    WAIT_UNTIL_TRUE(!hasOutStandingMessages());
    restartWM();
}
SCUTEST_SET_ENV(initSetup, cleanup, .timeout = 4);
static inline void triggerKeyCombo(const int* keys, int num) {
    for(int i = 0; i < num; i++) {
        sendKeyPress(keys[i], getActiveMasterKeyboardID());
    }
    for(int i = num - 1; i >= 0; i--) {
        sendKeyRelease(keys[i], getActiveMasterKeyboardID());
    }
}
SCUTEST(hide_only_window) {
    Window win = mapWindow(createNormalWindow());
    waitUntilWMIdle();
    assert(isWindowMapped(win));
    if(_i) {
        sendChangeWindowStateRequest(win, XCB_EWMH_WM_STATE_ADD, ewmh->_NET_WM_STATE_HIDDEN, 0);
    }
    else {
        triggerKeyCombo((int[]) {XK_Super_L, XK_y}, 2);
    }
    waitUntilWMIdle();
    assert(!isWindowMapped(win));
    if(_i) {
        sendChangeWindowStateRequest(win, XCB_EWMH_WM_STATE_REMOVE, ewmh->_NET_WM_STATE_HIDDEN, 0);
    }
    else {
        triggerKeyCombo((int[]) {XK_Super_L, XK_Shift_L, XK_y}, 3);
    }
    waitUntilWMIdle();
    assert(isWindowMapped(win));
}
