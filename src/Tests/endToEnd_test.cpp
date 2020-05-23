#include "../communications.h"
#include "../ewmh.h"
#include "../logger.h"
#include "../window-properties.h"
#include "../windows.h"
#include "../xsession.h"
#include "test-event-helper.h"
#include "tester.h"


static void restartWM() {
    send("restart", "");
    WAIT_UNTIL_TRUE(!hasOutStandingMessages());
    WAIT_UNTIL_TRUE(isMPXManagerRunning());
}
static void cleanup() {
    send("quit", "");
    WAIT_UNTIL_TRUE(!isMPXManagerRunning());
}

SET_ENV(openXDisplay, cleanup);

MPX_TEST_ITER("init_stacking_order", 2, {
    WindowID dock = mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DOCK));
    if(_i) {
        spawn("./mpxmanager");
        WAIT_UNTIL_TRUE(isMPXManagerRunning());
        waitUntilWMIdle();
    }
    WindowID win1 = mapArbitraryWindow();
    WindowID win2 = mapArbitraryWindow();
    if(!_i) {
        spawn("./mpxmanager");
        WAIT_UNTIL_TRUE(isMPXManagerRunning());
    }
    waitUntilWMIdle();
    WindowID relativePos[] = {win1, win2, dock};
    assert(checkStackingOrder(relativePos, LEN(relativePos)));
})

uint32_t wmPid;
static void initSetup() {
    wmPid = spawn("./mpxmanager");
    openXDisplay();
    WAIT_UNTIL_TRUE(isMPXManagerRunning());
    sendChangeWorkspaceRequest(0);
    waitUntilWMIdle();
}
SET_ENV(initSetup, cleanup);
MPX_TEST_ITER("remember_mapped_windows", 4, {
    WindowID win[] = {mapArbitraryWindow(), createNormalWindow(), mapArbitraryWindow(), mapWindow(createWindow())};

    for(int i = 0; i < LEN(win); i++)
        sendChangeWindowWorkspaceRequest(win[i], i);
    waitUntilWMIdle();
    assert(isWindowMapped(win[0]));
    mapWindow(win[1]);
    restartWM();
    sendChangeWorkspaceRequest(_i);
    waitUntilWMIdle();
    assert(isWindowMapped(win[_i]));
});

MPX_TEST("remember_focus", {
    addDefaultMaster();
    WindowID win = mapArbitraryWindow();
    waitUntilWMIdle();
    assertEquals(getActiveFocus(), win);
    restartWM();
    waitUntilWMIdle();
    assert(isWindowMapped(win));
    assertEquals(getActiveFocus(), win);
});

MPX_TEST_ITER("wm_restart_recursive", 3, {
    for(int i = 0; i < 5; i++) {
        WindowID win = getWMPrivateWindow();
        setWindowProperty(win, WM_WINDOW_ROLE, XCB_ATOM_CARDINAL, 1);
        if(_i == 0)
            restartWM();
        else if(_i == 1)
            kill(wmPid, SIGUSR1);
        else if(_i == 2)
            kill(wmPid, SIGHUP);
        WAIT_UNTIL_TRUE(!getWindowPropertyValue(win, WM_WINDOW_ROLE, XCB_ATOM_CARDINAL));
        WAIT_UNTIL_TRUE(isMPXManagerRunning());
    }
});

MPX_TEST("no_expose_on_restart", {
    NON_ROOT_EVENT_MASKS |= XCB_EVENT_MASK_EXPOSURE;
    WindowID wins[] = {mapArbitraryWindow(), mapArbitraryWindow()};

    waitUntilWMIdle();
    for(auto win : wins) {
        registerForWindowEvents(win, XCB_EVENT_MASK_EXPOSURE);
        assert(isWindowMapped(win));
    }
    consumeEvents();
    restartWM();
    waitUntilWMIdle();
    assertEquals(0, consumeEvents());
});
MPX_TEST("relative_position_after_restart", {
    send("auto-focus-new-window-timeout", "0");
    WindowID dock = mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DOCK));
    WindowID above = mapArbitraryWindow();
    WindowID normal = mapArbitraryWindow();
    WindowID bottom = mapWindow(createWindow());
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
});
