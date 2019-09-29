#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>

#include "../devices.h"
#include "../logger.h"
#include "../window-properties.h"
#include "../wmfunctions.h"
#include "../ewmh.h"
#include "../globals.h"
#include "../xsession.h"
#include "../session.h"
#include "../layouts.h"
#include "../wm-rules.h"
#include "../extra-rules.h"


#include "tester.h"
#include "test-event-helper.h"
#include "test-x-helper.h"

static void safeXSetup() {
    createXSimpleEnv();
    saveXSession();
    broadcastEWMHCompilence();
    registerForWindowEvents(root, ROOT_EVENT_MASKS);
}

SET_ENV(safeXSetup, fullCleanup);

MPX_TEST("test_lose_wm_selection", {
    int errSignal = 83;
    if(!fork()) {
        STEAL_WM_SELECTION = 1;
        openXDisplay();
        broadcastEWMHCompilence();
        flush();
        closeConnection();
        fullCleanup();
        exit(errSignal);
    }
    assertEquals(waitForChild(0), errSignal);
    waitForNormalEvent(XCB_SELECTION_CLEAR);
    waitForNormalEvent(XCB_CLIENT_MESSAGE);
});
MPX_TEST("test_lose_fake_selection", {
    setLogLevel(LOG_LEVEL_NONE);
    if(!fork()) {
        openXDisplay();
        broadcastEWMHCompilence();
        exit(20);
    }
    assert(waitForChild(0) != 20);
});
MPX_TEST("test_set_workspace_names", {
    updateWorkspaceNames();
});

MPX_TEST("ewmh_hooks", {
    addEWMHRules();
    addShutdownOnIdleRule();
    createNormalWindow();
    focusWindow(mapWindow(createNormalWindow()));
    activateWorkspace(0);
    runEventLoop(NULL);

});
SET_ENV(createXSimpleEnv, fullCleanup);
MPX_TEST("isMPXManagerRunning", {
    assert(!isMPXManagerRunning());
    broadcastEWMHCompilence();
    assert(isMPXManagerRunning());
    if(!fork()) {
        saveXSession();
        openXDisplay();
        assert(isMPXManagerRunning());
        quit(0);
    }
    assertEquals(waitForChild(0), 0);
});

MPX_TEST("test_toggle_show_desktop", {
    addBasicRules();
    WindowID win = mapWindow(createNormalWindow());
    WindowID desktop = mapWindow(createNormalWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DESKTOP));
    WindowID stackingOrder[] = {desktop, win, desktop};
    flush();
    scan(root);
    raiseWindowInfo(getWindowInfo(win));
    assert(checkStackingOrder(stackingOrder, 2));
    setShowingDesktop(1);
    flush();
    assert(isShowingDesktop());
    assert(checkStackingOrder(stackingOrder + 1, 2));
    setShowingDesktop(0);
    flush();
    assert(!isShowingDesktop());
    assert(checkStackingOrder(stackingOrder, 2));
});

static void clientMessageSetup() {
    CRASH_ON_ERRORS = -1;
    getEventRules(PostRegisterWindow).add(new BoundFunction(+[](WindowInfo * winInfo) {winInfo->addMask(SRC_ANY);},
    "_SRC_ANY_MASKS"));
    addEWMHRules();
    onStartup();
    WindowID win1 = mapWindow(createNormalWindow());
    WindowID win2 = mapWindow(createNormalWindow());
    mapWindow(createNormalWindow());
    scan(root);
    startWM();
    waitUntilIdle();
    focusWindow(win1);
    focusWindow(win2);
    getWindowInfo(win2)->moveToWorkspace(0);
    getWindowInfo(win2)->moveToWorkspace(1);
    waitUntilIdle();
}
SET_ENV(clientMessageSetup, []() {waitForAllThreadsToExit(); cleanupXServer();});
/* TODO uncomment
MPX_TEST_ERR("test_client_close_window", 1, {
    AUTO_FOCUS_NEW_WINDOW_TIMEOUT = -1;
    setLogLevel(0);
    //suppressOutput();
    sendCloseWindowRequest(getAllWindows()[0]);
    flush();
    waitForNormalEvent(XCB_DESTROY_NOTIFY);
    WAIT_UNTIL_TRUE(0);
});
*/
SET_ENV(clientMessageSetup, fullCleanup);
MPX_TEST("test_client_activate_window", {
    for(WindowInfo* winInfo : getAllWindows()) {
        Workspace* w = winInfo->getWorkspace();
        sendActivateWindowRequest(winInfo);
        flush();
        waitUntilIdle();
        assert(getActiveWorkspace() == w);
        assert(getFocusedWindow() == winInfo);
    }
});

MPX_TEST("test_client_change_desktop", {
    for(int i = 0; i < getNumberOfWorkspaces(); i++) {
        sendChangeWorkspaceRequest(i);
        flush();
        WAIT_UNTIL_TRUE(getActiveWorkspaceIndex() == i);
    }
});
MPX_TEST("test_client_change_num_desktop", {
    assert(getNumberOfWorkspaces() >= 2);
    switchToWorkspace(1);
    for(int n = 0; n < 2; n++) {
        for(int i = 1; i < getNumberOfWorkspaces() * 2; i++) {
            sendChangeNumWorkspaceRequestAbs(i);
            flush();
            WAIT_UNTIL_TRUE(getNumberOfWorkspaces() == i);
            assert(getActiveWorkspaceIndex() <= getNumberOfWorkspaces());
        }
    }
});


MPX_TEST("test_client_set_sticky_window", {
    mapArbitraryWindow();
    WAIT_UNTIL_TRUE(getAllWindows().size());
    WindowInfo* winInfo = getAllWindows()[0];
    sendChangeWindowWorkspaceRequest(winInfo, -1);
    flush();
    WAIT_UNTIL_TRUE(winInfo->hasMask(STICKY_MASK));
});

MPX_TEST("test_client_set_window_workspace", {
    int index = 3;
    WindowInfo* winInfo = getAllWindows()[0];
    sendChangeWindowWorkspaceRequest(winInfo, index);
    flush();
    WAIT_UNTIL_TRUE(getWorkspace(index)->getWindowStack().find(winInfo->getID()));
}
        );
MPX_TEST("test_client_set_window_state", {
    WindowInfo* winInfo = getAllWindows()[0];
    xcb_ewmh_wm_state_action_t states[] = {XCB_EWMH_WM_STATE_ADD, XCB_EWMH_WM_STATE_REMOVE, XCB_EWMH_WM_STATE_TOGGLE, XCB_EWMH_WM_STATE_TOGGLE};
    addUnknownWindowIgnoreRule();
    WindowID ignoredWindow = createIgnoredWindow();
    for(int i = 0; i < LEN(states); i++) {
        WindowInfo fakeWinInfo = {.id = ignoredWindow};
        sendChangeWindowStateRequest(&fakeWinInfo, states[i], ewmh->_NET_WM_STATE_MAXIMIZED_HORZ,
                                     ewmh->_NET_WM_STATE_MAXIMIZED_VERT);
        sendChangeWindowStateRequest(winInfo, states[i], ewmh->_NET_WM_STATE_MAXIMIZED_HORZ,
                                     ewmh->_NET_WM_STATE_MAXIMIZED_VERT);
        flush();
        if(i % 2 == 0)
            WAIT_UNTIL_TRUE(winInfo->hasMask(X_MAXIMIZED_MASK | Y_MAXIMIZED_MASK));
        else
            WAIT_UNTIL_TRUE(!winInfo->hasPartOfMask(X_MAXIMIZED_MASK | Y_MAXIMIZED_MASK));
        loadSavedAtomState(&fakeWinInfo);
        assert(fakeWinInfo.hasMask(X_MAXIMIZED_MASK | Y_MAXIMIZED_MASK) == (i % 2 == 0));
    }
});

MPX_TEST("test_client_show_desktop", {
    xcb_ewmh_request_change_showing_desktop(ewmh, defaultScreenNumber, 1);
    flush();
    WAIT_UNTIL_TRUE(isShowingDesktop());
    xcb_ewmh_request_change_showing_desktop(ewmh, defaultScreenNumber, 0);
    flush();
    WAIT_UNTIL_TRUE(!isShowingDesktop());
});

MPX_TEST("test_client_request_frame_extents", {
    WindowInfo* winInfo = getAllWindows()[0];
    xcb_ewmh_request_frame_extents(ewmh, defaultScreenNumber, winInfo->getID());
    flush();
    waitUntilIdle();
});

MPX_TEST("test_client_request_restack", {
    WindowInfo* winInfo = getAllWindows()[0];
    WindowInfo* winInfo2 = getAllWindows()[1];
    winInfo2->moveToWorkspace(0);
    WindowID stackingOrder[] = {winInfo->getID(), winInfo2->getID(), winInfo->getID()};
    int idle;
    lock();
    idle = getIdleCount();
    assert(raiseWindowInfo(winInfo2));
    unlock();
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    assert(checkStackingOrder(stackingOrder, 2));
    winInfo->addMask(EXTERNAL_RAISE_MASK);
    //processConfigureRequest(winInfo->getID(), NULL, winInfo2->getID(), XCB_STACK_MODE_ABOVE,  XCB_CONFIG_WINDOW_STACK_MODE|XCB_CONFIG_WINDOW_SIBLING);
    lock();
    idle = getIdleCount();
    xcb_ewmh_request_restack_window(ewmh, defaultScreenNumber, winInfo->getID(), winInfo2->getID(), XCB_STACK_MODE_ABOVE);
    flush();
    unlock();
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    assert(checkStackingOrder(stackingOrder + 1, 2));
});

MPX_TEST_ITER("test_client_ping", 2, {
    WindowID win = _i ? getPrivateWindow() : root;
    getAllWindows().add(new WindowInfo(win));
    WindowInfo* winInfo = getWindowInfo(win);
    assert(winInfo);
    if(_i) {
        const uint32_t data[] = { ewmh->_NET_WM_PING, 0, win};
        xcb_ewmh_send_client_message(ewmh->connection, win, root, ewmh->WM_PROTOCOLS, sizeof(data), data);
    }
    else
        xcb_ewmh_send_wm_ping(ewmh, winInfo->getID(), 0);
    flush();
    WAIT_UNTIL_TRUE(winInfo->getPingTimeStamp());
    waitUntilIdle();
});

MPX_TEST_ITER("wm_move_resize_window", 3, {
    movePointer(0, 0);
    WindowInfo* winInfo = getAllWindows()[0];
    floatWindow(winInfo);
    sendWMMoveResizeRequest(winInfo, 0, 0, XCB_EWMH_WM_MOVERESIZE_SIZE_RIGHT, 1);
    //TODO assert something
});
/* TODO uncomment
MPX_TEST("test_client_request_move_resize", {
    setLogLevel(0);
    Rect rect = {1, 2, 3, 4};
    WindowInfo* winInfo = getAllWindows()[0];
    winInfo->addMask(EXTERNAL_MOVE_MASK | EXTERNAL_RESIZE_MASK);
    sendMoveResizeRequest(winInfo, rect, CONFIG_X | CONFIG_Y | CONFIG_WIDTH | CONFIG_HEIGHT);
    flush();
    WAIT_UNTIL_TRUE(rect == winInfo->getGeometry());
});
*/
