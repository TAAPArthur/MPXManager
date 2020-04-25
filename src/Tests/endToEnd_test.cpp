#include "../communications.h"
#include "../ewmh.h"
#include "../logger.h"
#include "../window-properties.h"
#include "../windows.h"
#include "../xsession.h"
#include "test-event-helper.h"
#include "tester.h"


static int idleCount;
static void restartWM() {
    send("restart", "");
    WAIT_UNTIL_TRUE(!hasOutStandingMessages());
    WAIT_UNTIL_TRUE(isMPXManagerRunning());
    idleCount = 0;
}
static WindowID getWMPrivateWindow() {
    WindowID win;
    xcb_ewmh_get_supporting_wm_check_reply(ewmh, xcb_ewmh_get_supporting_wm_check(ewmh, root), &win, NULL);
    return win;
}
static int getWMIdleCount() {
    return getWindowPropertyValue(getWMPrivateWindow(), MPX_IDLE_PROPERTY, XCB_ATOM_CARDINAL);
}

static inline void waitUntilWMIdle() {
    WAIT_UNTIL_TRUE(idleCount != getWMIdleCount());
    idleCount = getWMIdleCount();
}
static void ping() {
    send("ping", "");
    WAIT_UNTIL_TRUE(!hasOutStandingMessages());
}
uint32_t wmPid;
static void initSetup() {
    wmPid = spawn("./mpxmanager");
    openXDisplay();
    WAIT_UNTIL_TRUE(isMPXManagerRunning());
    sendChangeWorkspaceRequest(0);
    waitUntilWMIdle();
}
static void cleanup() {
    send("quit", "");
    WAIT_UNTIL_TRUE(!isMPXManagerRunning());
}
SET_ENV(initSetup, cleanup);
MPX_TEST_ITER("remember_mapped_windows", 4, {
    WindowID win[] = {mapArbitraryWindow(), createNormalWindow(), mapArbitraryWindow(), mapWindow(createWindow())};

    for(int i = 0; i < LEN(win); i++)
        sendChangeWindowWorkspaceRequest(win[i], i);
    ping();
    assert(isWindowMapped(win[0]));
    mapWindow(win[1]);
    restartWM();
    sendChangeWorkspaceRequest(_i);
    ping();
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
