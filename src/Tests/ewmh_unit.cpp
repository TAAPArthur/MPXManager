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

MPX_TEST("test_private_window_properties", {
    assert("" != getWindowTitle(getPrivateWindow()));
});

MPX_TEST("ewmh_hooks", {
    addEWMHRules();
    addShutdownOnIdleRule();
    createNormalWindow();
    focusWindow(mapWindow(createNormalWindow()));
    runEventLoop(NULL);
});
MPX_TEST("test_sync_state", {
    addWorkspaces(2);
    saveXSession();
    setShowingDesktop(0);
    assert(!isShowingDesktop());
    assert(getActiveWorkspaceIndex() == 0);
    if(!fork()) {
        openXDisplay();
        getActiveMaster()->setWorkspaceIndex(1);
        setActiveProperties();
        setShowingDesktop(1);
        assert(isShowingDesktop());
        flush();
        closeConnection();
        quit(0);
    }
    assertEquals(waitForChild(0), 0);
    syncState();
    assertEquals(getActiveWorkspaceIndex(), 1);
    assert(isShowingDesktop());
});
MPX_TEST("test_invalid_state", {
    WindowID win = mapArbitraryWindow();
    xcb_ewmh_set_wm_desktop(ewmh, win, getNumberOfWorkspaces() + 1);
    xcb_ewmh_set_current_desktop(ewmh, defaultScreenNumber, getNumberOfWorkspaces() + 1);
    flush();

    addEWMHRules();
    syncState();
    scan(root);
    assert(getActiveWorkspaceIndex() < getNumberOfWorkspaces());
    assert(getWindowInfo(win)->getWorkspaceIndex() < getNumberOfWorkspaces());
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
MPX_TEST_ITER("auto_resume_workspace", 2, {
    addWorkspaces(2);
    WindowID win = mapArbitraryWindow();
    scan(root);
    WindowInfo* winInfo = getWindowInfo(win);
    winInfo->moveToWorkspace(getNumberOfWorkspaces() - 1);
    setSavedWorkspaceIndex(winInfo);
    winInfo->removeFromWorkspace();
    autoResumeWorkspace(getWindowInfo(win));
    if(_i)
        removeWorkspaces(getNumberOfWorkspaces() - 1);
    autoResumeWorkspace(getWindowInfo(win));
    assertEquals(winInfo->getWorkspaceIndex(), getNumberOfWorkspaces() - 1);
});

MPX_TEST("test_toggle_show_desktop", {
    addBasicRules();
    WindowID win = mapWindow(createNormalWindow());
    WindowID desktop = mapWindow(createNormalWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DESKTOP));
    WindowID stackingOrder[] = {desktop, win, desktop};
    flush();
    scan(root);
    raiseWindow(win);
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
    getEventRules(PostRegisterWindow).add(DEFAULT_EVENT(+[](WindowInfo * winInfo) {winInfo->addMask(SRC_ANY);}));
    addEWMHRules();
    onSimpleStartup();
    addAutoTileRules();
    WindowID win1 = mapWindow(createNormalWindow());
    WindowID win2 = mapWindow(createNormalWindow());
    startWM();
    waitUntilIdle();
    assert(focusWindow(win1));
    assert(focusWindow(win2));
    for(WindowInfo* winInfo : getAllWindows())
        assert(winInfo->getWorkspace());
    getWindowInfo(win1)->moveToWorkspace(0);
    getWindowInfo(win2)->moveToWorkspace(1);
    waitUntilIdle();
    assertEquals(getAllWindows().size(), 2);
}

SET_ENV(clientMessageSetup, fullCleanup);

MPX_TEST_ITER_ERR("test_client_close_window", 2, 1, {
    AUTO_FOCUS_NEW_WINDOW_TIMEOUT = -1;
    suppressOutput();
    addIgnoreOverrideRedirectWindowsRule();
    sendCloseWindowRequest(_i ? createIgnoredWindow() : getAllWindows()[0]->getID());
    flush();
    waitForAllThreadsToExit();
    assert(xcb_connection_has_error(dis));
    exit(1);
});
MPX_TEST("client_list", {
    xcb_ewmh_get_windows_reply_t reply;
    assert(xcb_ewmh_get_client_list_reply(ewmh, xcb_ewmh_get_client_list(ewmh, defaultScreenNumber), &reply, nullptr));
    auto size = reply.windows_len;
    assertEquals(size, getAllWindows().size());
    for(uint32_t i = 0; i <  size; i++)
        assertEquals(reply.windows[i], getAllWindows()[i]->getID());
    xcb_ewmh_get_windows_reply_wipe(&reply);
    // we get focus out event before destroy event
    CRASH_ON_ERRORS = 0;
    setLogLevel(LOG_LEVEL_NONE);
    for(WindowInfo* winInfo : getAllWindows()) {
        assert(!destroyWindow(winInfo->getID()));
    }
    waitUntilIdle();
    assert(xcb_ewmh_get_client_list_reply(ewmh, xcb_ewmh_get_client_list(ewmh, defaultScreenNumber), &reply, nullptr));
    assertEquals(reply.windows_len, 0);
    xcb_ewmh_get_windows_reply_wipe(&reply);
});

MPX_TEST_ITER("test_client_activate_window", 2, {
    for(WindowInfo* winInfo : getAllWindows()) {
        Workspace* w = winInfo->getWorkspace();
        assert(w);
        if(_i)
            setClientPointerForWindow(winInfo->getID(), getActiveMaster()->getPointerID());
        assert(winInfo->isActivatable());
        sendActivateWindowRequest(winInfo->getID());
        flush();
        waitUntilIdle();
        assertEquals(getActiveWorkspace(), w);
        assertEquals(getFocusedWindow(), winInfo);
    }
});
MPX_TEST("test_client_activate_window_bad", {
    activateWindow(getAllWindows()[0]);
    //don't crash
    sendActivateWindowRequest(0);
    sendActivateWindowRequest(1);
    waitUntilIdle();
    assertEquals(getActiveFocus(), getAllWindows()[0]->getID());
});

MPX_TEST("test_client_change_desktop", {
    for(int i = 0; i < getNumberOfWorkspaces(); i++) {
        sendChangeWorkspaceRequest(i);
        flush();
        WAIT_UNTIL_TRUE(getActiveWorkspaceIndex() == i);
    }
    sendChangeWorkspaceRequest(-2);
});
MPX_TEST("test_client_change_num_desktop", {
    assert(getNumberOfWorkspaces() >= 2);
    switchToWorkspace(1);
    for(int n = 0; n < 2; n++) {
        for(int i = 1; i < 10; i++) {
            sendChangeNumWorkspaceRequestAbs(i);
            flush();
            waitUntilIdle();
            assertEquals(getNumberOfWorkspaces(), i);
            assert(getActiveWorkspaceIndex() <= getNumberOfWorkspaces());
        }
    }
});


MPX_TEST("test_client_set_sticky_window", {
    mapArbitraryWindow();
    WAIT_UNTIL_TRUE(getAllWindows().size());
    WindowInfo* winInfo = getAllWindows()[0];
    sendChangeWindowWorkspaceRequest(winInfo->getID(), -1);
    flush();
    WAIT_UNTIL_TRUE(winInfo->hasMask(STICKY_MASK));
});

MPX_TEST("test_client_set_window_workspace", {
    int index = 3;
    WindowInfo* winInfo = getAllWindows()[0];
    sendChangeWindowWorkspaceRequest(winInfo->getID(), index);
    flush();
    WAIT_UNTIL_TRUE(getWorkspace(index)->getWindowStack().find(winInfo->getID()));
    // just don't crash
    sendChangeWindowWorkspaceRequest(winInfo->getID(), -2);
    waitUntilIdle();
});
MPX_TEST("test_client_set_window_unknown_state", {
    WindowInfo* winInfo = getAllWindows()[0];
    catchError(xcb_ewmh_set_wm_state_checked(ewmh, winInfo->getID(), 1, &ewmh->MANAGER));
    setXWindowStateFromMask(winInfo);
    xcb_ewmh_get_atoms_reply_t reply;
    if(xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, winInfo->getID()), &reply, NULL)) {
        assertEquals(reply.atoms[0], ewmh->MANAGER);
        xcb_ewmh_get_atoms_reply_wipe(&reply);
    }

});
MPX_TEST_ITER("test_client_set_window_state", 2, {
    WindowInfo* winInfo = getAllWindows()[0];
    bool allowRequest = _i;
    if(!allowRequest)
        winInfo->removeMask(SRC_ANY);
    xcb_ewmh_wm_state_action_t states[] = {XCB_EWMH_WM_STATE_ADD, XCB_EWMH_WM_STATE_REMOVE, XCB_EWMH_WM_STATE_TOGGLE, XCB_EWMH_WM_STATE_TOGGLE};
    WindowID ignoredWindow = createIgnoredWindow();
    for(int i = 0; i < LEN(states); i++) {
        WindowInfo fakeWinInfo = {.id = ignoredWindow};
        sendChangeWindowStateRequest(fakeWinInfo.getID(), states[i], ewmh->_NET_WM_STATE_MAXIMIZED_HORZ,
            ewmh->_NET_WM_STATE_MAXIMIZED_VERT);
        sendChangeWindowStateRequest(winInfo->getID(), states[i], ewmh->_NET_WM_STATE_MAXIMIZED_HORZ,
            ewmh->_NET_WM_STATE_MAXIMIZED_VERT);
        waitUntilIdle();
        if(i % 2 == 0 && allowRequest)
            assert(winInfo->hasMask(X_MAXIMIZED_MASK | Y_MAXIMIZED_MASK));
        else
            assert(!winInfo->hasPartOfMask(X_MAXIMIZED_MASK | Y_MAXIMIZED_MASK));
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
    raiseWindow(winInfo2->getID());
    assert(checkStackingOrder(stackingOrder, 2));
    winInfo->addMask(EXTERNAL_RAISE_MASK);
    //processConfigureRequest(winInfo->getID(), NULL, winInfo2->getID(), XCB_STACK_MODE_ABOVE,  XCB_CONFIG_WINDOW_STACK_MODE|XCB_CONFIG_WINDOW_SIBLING);
    lock();
    xcb_ewmh_request_restack_window(ewmh, defaultScreenNumber, winInfo->getID(), winInfo2->getID(), XCB_STACK_MODE_ABOVE);
    flush();
    unlock();
    waitUntilIdle();
    assert(checkStackingOrder(stackingOrder + 1, 2));
});
MPX_TEST("test_client_request_restack_bad", {
    xcb_ewmh_request_restack_window(ewmh, defaultScreenNumber, 0, 0, XCB_STACK_MODE_ABOVE);
    waitUntilIdle();
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
MPX_TEST("test_client_ping_bad", {
    ATOMIC(xcb_ewmh_send_wm_ping(ewmh, destroyWindow(createNormalWindow()), 0));
    waitUntilIdle();
});

static void wmMoveSetup() {
    getDeviceBindings().add({0, 0, DEFAULT_EVENT(updateWindowMoveResize), {.noGrab = 1, .mask = XCB_INPUT_XI_EVENT_MASK_MOTION}});
    clientMessageSetup();
    lock();
    movePointer(0, 0);
    WindowInfo* winInfo = getAllWindows()[0];
    floatWindow(winInfo);
    grabPointer();
    unlock();
}
SET_ENV(wmMoveSetup, fullCleanup);
MPX_TEST_ITER("wm_move_resize_window", 4, {
    int deltaX = 10;
    int deltaY = 20;
    WindowInfo* winInfo = getAllWindows()[0];
    RectWithBorder rect = getRealGeometry(winInfo->getID());
    if(_i) {
        rect.width += deltaX * (_i & 1);
        rect.height += deltaY * (_i & 2 ? 1 : 0);
        xcb_ewmh_moveresize_direction_t action = _i == 1 ? XCB_EWMH_WM_MOVERESIZE_SIZE_RIGHT : _i == 2 ?
        XCB_EWMH_WM_MOVERESIZE_SIZE_BOTTOM : XCB_EWMH_WM_MOVERESIZE_SIZE_BOTTOMRIGHT;
        sendWMMoveResizeRequest(winInfo->getID(), 0, 0, action, 1);
    }
    else {
        rect.x += deltaX;
        rect.y += deltaY;
        sendWMMoveResizeRequest(winInfo->getID(), 0, 0, XCB_EWMH_WM_MOVERESIZE_MOVE, 1);
    }
    waitUntilIdle();
    movePointer(deltaX, deltaY);
    flush();
    waitUntilIdle();
    assertEquals(rect, getRealGeometry(winInfo->getID()));
    consumeEvents();
});
MPX_TEST("wm_move_resize_window_bad", {
    sendWMMoveResizeRequest(0, 0, 0, XCB_EWMH_WM_MOVERESIZE_SIZE_BOTTOMRIGHT, 1);
    waitUntilIdle();
});
MPX_TEST("wm_move_resize_window_invert", {
    movePointer(20, 40);
    WindowInfo* winInfo = getAllWindows()[0];
    setWindowPosition(winInfo->getID(), {10, 20, 10, 20});
    RectWithBorder rect = {0, 0, 10, 20};
    xcb_ewmh_moveresize_direction_t action = XCB_EWMH_WM_MOVERESIZE_SIZE_BOTTOMRIGHT;
    sendWMMoveResizeRequest(winInfo->getID(), 0, 0, action, 1);
    waitUntilIdle();
    movePointer(0, 0);
    flush();
    waitUntilIdle();
    assertEquals(rect, getRealGeometry(winInfo->getID()));
});
MPX_TEST_ITER("wm_move_resize_window_cancel_commit", 2, {
    WindowInfo* winInfo = getAllWindows()[0];
    RectWithBorder rect = getRealGeometry(winInfo->getID());
    sendWMMoveResizeRequest(winInfo->getID(), 0, 0, XCB_EWMH_WM_MOVERESIZE_SIZE_BOTTOMRIGHT, 1);
    waitUntilIdle();
    movePointer(10, 10);
    flush();
    waitUntilIdle();
    if(_i)
        commitWindowMoveResize(getActiveMaster());
    assert(rect != getRealGeometry(winInfo->getID()));
    sendWMMoveResizeRequest(winInfo->getID(), 0, 0, XCB_EWMH_WM_MOVERESIZE_CANCEL, 1);
    waitUntilIdle();
    if(_i)
        assert(rect != getRealGeometry(winInfo->getID()));
    else
        assertEquals(rect, getRealGeometry(winInfo->getID()));
});
MPX_TEST("wm_move_resize_window_no_change", {
    WindowInfo* winInfo = getAllWindows()[0];
    consumeEvents();
    startWindowMoveResize(winInfo, 0);
    updateWindowMoveResize();
    assertEquals(consumeEvents(), 0);
});
MPX_TEST("wm_move_resize_window_zero", {
    WindowInfo* winInfo = getAllWindows()[0];
    lock();
    setWindowPosition(winInfo->getID(), {0, 0, 150, 150});
    RectWithBorder rect = getRealGeometry(winInfo->getID());
    movePointer(rect.width, rect.height);
    startWindowMoveResize(winInfo, 0);
    movePointer(0, 0);
    updateWindowMoveResize();
    assert(rect != getRealGeometry(winInfo->getID()));
    assertEquals(Rect(rect.x, rect.y, 1, 1), getRealGeometry(winInfo->getID()));
    unlock();
});
MPX_TEST("test_client_request_move_resize", {
    Rect rect = {1, 2, 3, 4};
    WindowInfo* winInfo = getAllWindows()[0];
    winInfo->addMask(EXTERNAL_MOVE_MASK | EXTERNAL_RESIZE_MASK);
    sendMoveResizeRequest(winInfo->getID(), rect, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT);
    flush();
    WAIT_UNTIL_TRUE(rect == winInfo->getGeometry());
});

MPX_TEST("event_speed", {
    startWindowMoveResize(getAllWindows()[0], 0);
    generateMotionEvents(10000);
    waitUntilIdle();
    assert(!consumeEvents());
});
