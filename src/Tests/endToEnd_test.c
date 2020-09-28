#include "../communications.h"
#include "../windows.h"
#include "../wmfunctions.h"
#include "../system.h"
#include "test-wm-helper.h"
#include "test-event-helper.h"
#include "tester.h"
#include <signal.h>


uint32_t wmPid;
static void restartWM() {
    WindowID win = getWMPrivateWindow();
    setWindowPropertyInt(win, WM_STATE, XCB_ATOM_CARDINAL, 1);
    send("restart", "");
    WAIT_UNTIL_TRUE(!hasOutStandingMessages());
    WAIT_UNTIL_TRUE(!getWindowPropertyValueInt(win, WM_STATE, XCB_ATOM_CARDINAL));
    WAIT_UNTIL_TRUE(isMPXManagerRunning());
    WAIT_UNTIL_TRUE(getWMIdleCount());
}
static void cleanup() {
    assert(wmPid);
    kill(wmPid, SIGINT);
    WAIT_UNTIL_TRUE(!isMPXManagerRunning());
}

static void initSetup() {
    wmPid = spawnChild("./mpxmanager --log-level 0");
    createXSimpleEnv();
    WAIT_UNTIL_TRUE(isMPXManagerRunning());
    waitUntilWMIdle();
}
SCUTEST_SET_ENV(initSetup, cleanup, .timeout = 4);
SCUTEST(remember_mapped_windows) {
    Window win = mapWindow(createNormalWindow());
    waitUntilWMIdle();
    assert(isWindowMapped(win));
    unmapWindow(win);
    restartWM();
    waitUntilWMIdle();
    assert(isWindowMapped(win));
}

SCUTEST(test_long_lived) {
    for(int i = 0; i < 3; i++) {
        mapWindow(createNormalWindow());
        waitUntilWMIdle();
    }
    spawnAndWait("./mpxmanager sum");
    Window win = mapWindow(createNormalWindow());
    waitUntilWMIdle();
    assert(isWindowMapped(win));
}

SCUTEST(remember_focus) {
    WindowID win = mapArbitraryWindow();
    waitUntilWMIdle();
    focusWindow(win);
    waitUntilWMIdle();
    assertEquals(getActiveFocus(), win);
    restartWM();
    assert(isWindowMapped(win));
    assertEquals(getActiveFocus(), win);
}

SCUTEST_ITER(wm_restart_recursive, 3) {
    for(int i = 0; i < 5; i++) {
        WindowID win = getWMPrivateWindow();
        setWindowPropertyInt(win, WM_WINDOW_ROLE, XCB_ATOM_CARDINAL, 1);
        if(_i == 0)
            restartWM();
        else if(_i == 1)
            kill(wmPid, SIGUSR1);
        else if(_i == 2)
            kill(wmPid, SIGHUP);
        WAIT_UNTIL_TRUE(!getWindowPropertyValueInt(win, WM_WINDOW_ROLE, XCB_ATOM_CARDINAL));
        WAIT_UNTIL_TRUE(isMPXManagerRunning());
    }
}
SCUTEST(test_send_as) {
    char buffer[255];
    createMasterDevice("test");
    createMasterDevice("test2");
    initCurrentMasters();
    sprintf(buffer, "./mpxmanager --log-level 0 --as %d focus-win %d", getMasterByName("test")->id, root);
    spawnAndWait(buffer);
    assertEquals(getActiveFocusOfMaster(getMasterByName("test")->id), root);
    assert(getActiveFocusOfMaster(getMasterByName("test2")->id) != root);
}

SCUTEST(no_expose_on_restart) {
    NON_ROOT_EVENT_MASKS |= XCB_EVENT_MASK_EXPOSURE;
    WindowID wins[] = {mapArbitraryWindow(), mapArbitraryWindow()};
    waitUntilWMIdle();
    for(int i = 0; i < LEN(wins); i++) {
        registerForWindowEvents(wins[i], XCB_EVENT_MASK_EXPOSURE);
        assert(isWindowMapped(wins[i]));
    }
    consumeEvents();
    restartWM();
    waitUntilWMIdle();
    assertEquals(0, consumeEvents());
}
/*TODO
SCUTEST(relative_position_after_restart) {
    send("auto-focus-new-window-timeout", "0");
    WindowID dock = mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DOCK));
    WindowID above = mapArbitraryWindow();
    WindowID normal = mapArbitraryWindow();
    WindowID bottom = mapWindow(createNormalWindow());
    waitUntilWMIdle();
    sendChangeWindowStateRequest(above, XCB_EWMH_WM_STATE_ADD, ewmh->_NET_WM_STATE_ABOVE);
    sendChangeWindowStateRequest(bottom, XCB_EWMH_WM_STATE_ADD, ewmh->_NET_WM_STATE_BELOW);
    waitUntilWMIdle();
    WindowID relativePos[] = {bottom, normal, above};
    WindowID relativePos2[] = {bottom, normal, dock};
    assert(checkStackingOrder(relativePos, LEN(relativePos)));
    assert(checkStackingOrder(relativePos2, LEN(relativePos)));
    restartWM();
    waitUntilWMIdle();
    assert(checkStackingOrder(relativePos, LEN(relativePos)));
    assert(checkStackingOrder(relativePos2, LEN(relativePos)));
    WindowID normal2 = mapArbitraryWindow();
    waitUntilWMIdle();
    WindowID relativePos3[] = {bottom, normal2, above};
    assert(checkStackingOrder(relativePos3, LEN(relativePos)));
}
*/
