#include "../ewmh.h"
#include "../extra-rules.h"
#include "../globals.h"
#include "../layouts.h"
#include "../logger.h"
#include "../window-properties.h"
#include "../wm-rules.h"
#include "../wmfunctions.h"
#include "../xsession.h"

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

MPX_TEST_ITER("test_set_workspace_names", 2, {
    if(_i) {
        addWorkspaces(10);
        getWorkspace(1)->setName("Really_long name with some special characters !@#$%$&%");
    }
    updateWorkspaceNames();
    xcb_ewmh_get_utf8_strings_reply_t  names;
    assert(xcb_ewmh_get_desktop_names_reply(ewmh, xcb_ewmh_get_desktop_names(ewmh, defaultScreenNumber), &names, NULL));
    int index = 0;
    for(Workspace* w : getAllWorkspaces()) {
        assertEquals(w->getName(), &names.strings[index]);
        index += w->getName().length() + 1;
    }
    xcb_ewmh_get_utf8_strings_reply_wipe(&names);
});

MPX_TEST("test_private_window_properties", {
    assert("" != getWindowTitle(getPrivateWindow()));
});

MPX_TEST("ewmh_hooks", {
    addEWMHRules();
    addShutdownOnIdleRule();
    createNormalWindow();
    focusWindow(mapWindow(createNormalWindow()));
    runEventLoop();
});
MPX_TEST("remember_mapped_windows", {
    addBasicRules();
    addEWMHRules();
    addShutdownOnIdleRule();
    WindowID win = mapWindow(createWindow());
    WindowID win2 = mapWindow(createWindow());
    xcb_unmap_notify_event_t event = {.response_type = XCB_UNMAP_NOTIFY, .window = win2};
    catchError(xcb_send_event_checked(dis, 0, root, ROOT_EVENT_MASKS, (char*) &event));
    runEventLoop();
    getAllWindows().deleteElements();
    unmapWindow(win);
    unmapWindow(win2);
    scan(root);
    assert(getWindowInfo(win)->hasMask(MAPPABLE_MASK));
    assert(!getWindowInfo(win2)->hasMask(MAPPABLE_MASK));
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


MPX_TEST("test_sync_window_state", {
    assert(MASKS_TO_SYNC);
    assert(!getAllWindows().size());
    WindowID win = mapWindow(createNormalWindow());
    scan(root);
    WindowInfo* winInfo = getWindowInfo(win);
    winInfo->addMask(MASKS_TO_SYNC);
    setXWindowStateFromMask(winInfo);
    unregisterWindow(winInfo);
    scan(root);
    winInfo = getWindowInfo(win);
    loadSavedAtomState(winInfo);
    assertEquals(winInfo->getMask()&MASKS_TO_SYNC, MASKS_TO_SYNC);
});

SET_ENV(onSimpleStartup, fullCleanup);
MPX_TEST("test_toggle_show_desktop", {
    WindowID win = mapWindow(createNormalWindow());
    WindowID desktop = mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DESKTOP));

    WindowID stackingOrder[] = {desktop, win, desktop};
    flush();
    scan(root);
    raiseWindow(getWindowInfo(win));
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

MPX_TEST_ITER_ERR("test_client_close_window", 2, 1, {
    AUTO_FOCUS_NEW_WINDOW_TIMEOUT = -1;
    addEWMHRules();
    suppressOutput();
    addIgnoreOverrideRedirectWindowsRule();

    mapWindow(createNormalWindow());
    scan(root);
    sendCloseWindowRequest(_i ? createOverrideRedirectWindow() : getAllWindows()[0]->getID());
    runEventLoop();
    assert(xcb_connection_has_error(dis));
    exit(1);
});

static void clientMessageSetup() {
    CRASH_ON_ERRORS = -1;
    SRC_INDICATION = -1;
    MASKS_TO_SYNC = -1;
    addEWMHRules();
    onSimpleStartup();
    addAutoTileRules();
    WindowID win1 = mapWindow(createNormalWindow());
    WindowID win2 = mapWindow(createNormalWindow());
    scan(root);
    assertEquals(getAllWindows().size(), 2);
    assert(focusWindow(win1));
    assert(focusWindow(win2));
    for(WindowInfo* winInfo : getAllWindows())
        assert(winInfo->getWorkspace());
    getWindowInfo(win1)->moveToWorkspace(0);
    getWindowInfo(win2)->moveToWorkspace(1);
    startWM();
    waitUntilIdle();
}

SET_ENV(clientMessageSetup, fullCleanup);

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
    ATOMIC(activateWindow(getAllWindows()[0]));
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
    assert(getActiveWorkspaceIndex() < getNumberOfWorkspaces());
});
MPX_TEST_ITER("test_client_change_num_desktop", 2, {
    assert(getNumberOfWorkspaces() >= 2);
    lock();
    switchToWorkspace(_i);
    unlock();
    waitUntilIdle(1);
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
    assert(xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, winInfo->getID()), &reply, NULL));
    assertEquals(reply.atoms_len, 1);
    assertEquals(reply.atoms[0], ewmh->MANAGER);
    xcb_ewmh_get_atoms_reply_wipe(&reply);
});
MPX_TEST_ITER("test_client_set_window_state", 3, {
    suppressOutput();
    setLogLevel(LOG_LEVEL_VERBOSE);
    source = (xcb_ewmh_client_source_type_t)_i;
    WindowInfo* winInfo = getAllWindows()[0];
    sendChangeWindowStateRequest(winInfo->getID(), XCB_EWMH_WM_STATE_ADD, ewmh->_NET_WM_STATE_STICKY);
    waitUntilIdle();
});
MPX_TEST_ITER("test_client_set_window_state", 2, {
    WindowInfo* winInfo = getAllWindows()[0];
    bool allowRequest = _i;
    if(!allowRequest)
        SRC_INDICATION = 0;
    xcb_ewmh_wm_state_action_t states[] = {XCB_EWMH_WM_STATE_ADD, XCB_EWMH_WM_STATE_REMOVE, XCB_EWMH_WM_STATE_TOGGLE, XCB_EWMH_WM_STATE_TOGGLE};
    WindowID ignoredWindow = createOverrideRedirectWindow();
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
        assert(!fakeWinInfo.hasMask(X_MAXIMIZED_MASK | Y_MAXIMIZED_MASK));
    }
});
MPX_TEST_ITER("change_wm_state", 2, {
    Window win = mapArbitraryWindow();
    if(!_i)
        MASKS_TO_SYNC = 0;
    uint32_t state = XCB_ICCCM_WM_STATE_ICONIC;
    catchError(xcb_ewmh_send_client_message(dis, win, root, WM_CHANGE_STATE, sizeof(state), &state));
    waitUntilIdle();
    assertEquals(getWindowInfo(win)->hasMask(HIDDEN_MASK), _i);
});
MPX_TEST_ITER("test_handle_unsync_requests", 4, {
    MASKS_TO_SYNC = HIDDEN_MASK;
    ALLOW_SETTING_UNSYNCED_MASKS = _i / 2;
    bool masksAlreadySet = _i % 2;
    int newExpectedAtomsDistribution[][2] = {
        {0, 1},
        {1, 1}
    };
    int baseAtoms = 1;
    int expectedAtoms = baseAtoms + newExpectedAtomsDistribution[ALLOW_SETTING_UNSYNCED_MASKS][masksAlreadySet];
    WindowInfo* winInfo = getAllWindows()[0];
    if(masksAlreadySet) {
        MASKS_TO_SYNC |= FULLSCREEN_MASK;
        winInfo->addMask(FULLSCREEN_MASK);
    }
    sendChangeWindowStateRequest(winInfo->getID(), XCB_EWMH_WM_STATE_ADD, ewmh->_NET_WM_STATE_HIDDEN, ewmh->_NET_WM_STATE_FULLSCREEN);
    waitUntilIdle();
    assert(winInfo->hasMask(HIDDEN_MASK));
    assertEquals(masksAlreadySet, winInfo->hasMask(FULLSCREEN_MASK));

    xcb_ewmh_get_atoms_reply_t reply;
    assert(xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, winInfo->getID()), &reply, NULL));
    INFO(getAtomsAsString(reply.atoms, reply.atoms_len));
    assertEquals(reply.atoms_len, expectedAtoms);
    xcb_ewmh_get_atoms_reply_wipe(&reply);
    if(expectedAtoms) {
        sendChangeWindowStateRequest(winInfo->getID(), XCB_EWMH_WM_STATE_REMOVE, ewmh->_NET_WM_STATE_FULLSCREEN);
        assert(xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, winInfo->getID()), &reply, NULL));
        assertEquals(reply.atoms_len, expectedAtoms);
        xcb_ewmh_get_atoms_reply_wipe(&reply);
    }
})

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
    setActiveLayout(NULL);
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
    clientMessageSetup();
    lock();
    movePointer(0, 0);
    floatWindow(getAllWindows()[0]);
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
    for(int i = 0; i < 2; i++) {
        clickButton(1);
        movePointer(deltaX * 2, deltaY * 2);
        waitUntilIdle();
        assertEquals(rect, getRealGeometry(winInfo->getID()));
        grabPointer();
    }
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
    grabPointer();
    startWindowMoveResize(getAllWindows()[0], 0);
    generateMotionEvents(1000);
    waitUntilIdle();
    assert(!consumeEvents());
});
