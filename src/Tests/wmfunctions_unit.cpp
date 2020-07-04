#include "../globals.h"
#include "../logger.h"
#include "../state.h"
#include "../window-properties.h"
#include "../wmfunctions.h"
#include "../xsession.h"

#include "tester.h"
#include "test-event-helper.h"

SET_ENV(createXSimpleEnv, fullCleanup);
MPX_TEST("register_unregister", {
    WindowID win = createInputOnlyWindow();
    assertEquals(getAllWindows().size(), 0);
    assert(registerWindow(win, root) == 1);
    assertEquals(getNumberOfEventsTriggerSinceLastIdle(PRE_REGISTER_WINDOW), 1);
    assertEquals(getNumberOfEventsTriggerSinceLastIdle(POST_REGISTER_WINDOW), 1);
    assertEquals(getAllWindows().size(), 1);
    assert(unregisterWindow(getWindowInfo(win)) == 1);
    assertEquals(getAllWindows().size(), 0);
});
MPX_TEST_ITER("register_bad_window", 2, {
    WindowID win = 2;
    if(_i) {
        win = createNormalWindow();
        assert(!destroyWindow(win));
    }
    assert(registerWindow(win, root) == 0);
    assert(getAllWindows().size() == 0);
    assert(unregisterWindow(getWindowInfo(win)) == 0);
});
MPX_TEST("preregister_fail", {
    getEventRules(PRE_REGISTER_WINDOW).add(NO_PASSTHROUGH);
    assert(registerWindow(createNormalWindow(), root) == 0);
    assert(getAllWindows().size() == 0);
});

MPX_TEST_ITER("detect_input_only_window", 2, {
    WindowID win = createInputOnlyWindow();
    if(_i)
        scan(root);
    else
        registerWindow(win, root);
    assert(getWindowInfo(win)->isInputOnly());
});
MPX_TEST("filter_window", {
    static WindowID win = mapWindow(createNormalWindow());
    WindowID win2 = mapWindow(createNormalWindow());
    getEventRules(CLIENT_MAP_ALLOW).add(DEFAULT_EVENT(incrementCount));
    getEventRules(POST_REGISTER_WINDOW).add(DEFAULT_EVENT(incrementCount));
    getEventRules(CLIENT_MAP_ALLOW).add({
        +[](WindowInfo * winInfo) {
            auto returnVal = win != *winInfo;
            if(win == *winInfo)
                unregisterWindow(winInfo);
            return returnVal;
        }, "filter", PASSTHROUGH_IF_TRUE});
    scan(root);
    assertEquals(getCount(), 3);
    assert(getWindowInfo(win2));
    assert(!getWindowInfo(win));
});
MPX_TEST("detect_mapped_windows", {
    WindowID win = createNormalWindow();
    WindowID win2 = mapArbitraryWindow();
    scan(root);
    assert(!getWindowInfo(win)->hasMask(MAPPED_MASK));
    assert(getWindowInfo(win2)->hasMask(MAPPED_MASK));
});
MPX_TEST("test_window_scan", {
    addIgnoreOverrideRedirectWindowsRule();
    WindowID win = createOverrideRedirectWindow();
    scan(root);
    assert(!getWindowInfo(win));
});
MPX_TEST("test_window_scan", {
    scan(root);
    WindowID win = createUnmappedWindow();
    WindowID win2 = createUnmappedWindow();
    WindowID win3 = createUnmappedWindow();
    WindowID win4 = createOverrideRedirectWindow();
    assert(!isWindowMapped(win));
    assert(!isWindowMapped(win2));
    assert(!isWindowMapped(win3));
    mapWindow(win);
    xcb_flush(dis);
    scan(root);
    assert(getWindowInfo(win));
    assert(getWindowInfo(win2));
    assert(getWindowInfo(win3));
    assert(!getWindowInfo(win3)->isOverrideRedirectWindow());
    assert(getWindowInfo(win4));
    assert(getWindowInfo(win4)->isOverrideRedirectWindow());
    assertEquals(getAllWindows().size(), 4);
});

MPX_TEST("test_sticky_workspace_change", {
    WindowID win = mapArbitraryWindow();
    WindowInfo* winInfo = new WindowInfo(win);
    getAllWindows().add(winInfo);
    winInfo->moveToWorkspace(0);
    winInfo->addMask(STICKY_MASK);
    for(int i = 0; i < getNumberOfWorkspaces(); i++) {
        switchToWorkspace(i);
        assert(winInfo->getWorkspaceIndex() == i);
    }
});

MPX_TEST("test_workspace_change", {
    addWorkspaces(2);
    int nonEmptyIndex = 0;
    mapArbitraryWindow();
    mapArbitraryWindow();
    scan(root);
    for(WindowInfo* winInfo : getAllWindows())
        winInfo->moveToWorkspace(0);
    assert(getAllWindows().size() == 2);
    assert(getActiveWindowStack().size() == 2);
    assert(getNumberOfWorkspaces() > 1);
    for(int i = 0; i < getNumberOfWorkspaces(); i++) {
        switchToWorkspace(i);
        if(i == nonEmptyIndex)continue;
        assert(getActiveWorkspace()->getID() == i);
        assert(getActiveWindowStack().size() == 0);
        assert(getActiveMaster()->getWorkspaceIndex() == i);
    }
    switchToWorkspace(nonEmptyIndex);
    assert(getActiveWindowStack().size() == 2);
    getActiveWindowStack()[0]->moveToWorkspace(!nonEmptyIndex);
    assert(getActiveWindowStack().size() == 1);
    assert(getWorkspace(!nonEmptyIndex)->getWindowStack().size() == 1);
});
MPX_TEST("test_workspace_change_focus", {
    addBasicRules();
    addWorkspaces(2);
    mapArbitraryWindow();
    mapArbitraryWindow();
    mapArbitraryWindow();
    scan(root);
    for(WindowInfo* winInfo : getAllWindows()) {
        winInfo->addMask(INPUT_MASK);
        getActiveMaster()->onWindowFocus(winInfo->getID());
        winInfo->moveToWorkspace(0);
    }
    getAllWindows()[0]->moveToWorkspace(1);
    applyBatchEventRules();
    assert(focusWindow(getAllWindows()[1]));
    assertEquals(getActiveFocus(), getAllWindows()[1]->getID());
    switchToWorkspace(1);
    applyBatchEventRules();

    updateState();
    flush();
    assertEquals(getActiveFocus(), getAllWindows()[0]->getID());
});
MPX_TEST("test_destroy_focus", {
    int num = 3;
    for(int i = 0; i < num - 1; i++)
        addFakeMonitor({0, 0, 100, 100});
    addWorkspaces(num);
    assignUnusedMonitorsToWorkspaces();
    assert(!getWorkspace(num)->isVisible());
    WindowID wins[] = {mapArbitraryWindow(), mapArbitraryWindow(), mapArbitraryWindow(), root};
    assertEquals(LEN(wins), getNumberOfWorkspaces());
    scan(root);
    for(int i = LEN(wins) - 2; i >= 0; i--) {
        auto* winInfo = getWindowInfo(wins[i]);
        winInfo->addMask(INPUT_MASK);
        winInfo->moveToWorkspace(i);
        getActiveMaster()->onWindowFocus(winInfo->getID());
        assert(focusWindow(winInfo));
    }
    for(int i = 0; i < num; i++) {
        getActiveMaster()->onWindowFocus(wins[i]);
        getWindowInfo(wins[i])->moveToWorkspace(num);
        updateWindowWorkspaceState(getWindowInfo(wins[i]));
        assertEquals(getActiveFocus(), wins[i + 1]);
    }
});

SET_ENV(onSimpleStartup, fullCleanup);

MPX_TEST("test_activate_window", {
    WindowID win = createUnmappedWindow();
    WindowID win2 = mapWindow(createNormalWindow());
    WindowID win3 = mapWindow(createNormalWindow());
    scan(root);
    assert(getWindowInfo(win2)->isActivatable());
    assert(!activateWindow(getWindowInfo(win)));
    getWindowInfo(win)->addMask(MAPPABLE_MASK);
    assert(activateWindow(getWindowInfo(win)));
    assert(activateWindow(getWindowInfo(win2)));
    getWindowInfo(win2)->removeMask(ALL_MASK);
    assert(!activateWindow(getWindowInfo(win2)));
    assert(getWindowInfo(win3)->isActivatable());
    getWindowInfo(win3)->moveToWorkspace(1);
    getWindowInfo(win3)->removeMask(MAPPED_MASK);
    assert(!getWorkspace(1)->isVisible());
    assert(getWindowInfo(win3)->isActivatable());

    updateState();
    assert(activateWindow(getWindowInfo(win3)));

    updateState();
    assertEquals(getActiveWorkspaceIndex(), 1);

});

MPX_TEST("test_activate_workspace", {
    WindowID win = mapWindow(createNormalWindow());
    WindowID win2 = mapWindow(createNormalWindow());
    WindowID win3 = mapWindow(createNormalWindow());
    scan(root);
    getWindowInfo(win)->moveToWorkspace(1);
    getWindowInfo(win3)->moveToWorkspace(0);
    getWindowInfo(win2)->moveToWorkspace(0);
    assertEquals(activateWorkspace(1), 0);
    getActiveMaster()->onWindowFocus(win);
    assertEquals(getActiveWorkspace()->getID(), 1);
    assertEquals(activateWorkspace(0), 0);
    getActiveMaster()->onWindowFocus(win3);
    getActiveMaster()->onWindowFocus(win2);
    assertEquals(activateWorkspace(1), win);
    assertEquals(activateWorkspace(0), win2);

});

MPX_TEST_ITER("test_sticky_window", 2, {
    assert(getWorkspace(0)->isVisible());
    assert(!getWorkspace(1)->isVisible());
    mapArbitraryWindow();
    scan(root);
    WindowInfo* winInfo = getAllWindows()[0];
    winInfo->addMask(STICKY_MASK);
    winInfo->moveToWorkspace(0);
    Master* m = new Master(10, 11, "dummy");
    m->setWorkspaceIndex(1);
    getAllMasters().add(m);
    auto check = +[](int i) {
        WindowInfo* winInfo = getAllWindows()[0];
        assertEquals(winInfo->getWorkspaceIndex(), i);
        assert(isWindowMapped(winInfo->getID()));
        validate();
    };
    switchToWorkspace(2);
    applyBatchEventRules();
    setActiveMaster(getAllMasters()[_i]);
    check(2);
    for(int i = 1; i < getNumberOfWorkspaces() + 1; i++) {
        switchToWorkspace(i % getNumberOfWorkspaces());
        applyBatchEventRules();
        check(i % getNumberOfWorkspaces());
    }
});
MPX_TEST("test_sticky_workspaceless_window", {
    mapArbitraryWindow();
    scan(root);
    WindowInfo* winInfo = getAllWindows()[0];
    winInfo->addMask(STICKY_MASK);

    updateState();
    updateWindowWorkspaceState(winInfo);
});
MPX_TEST_ITER("test_workspace_activation", 3, {
    addWorkspaces(3);
    mapArbitraryWindow();
    mapArbitraryWindow();
    scan(root);
    WindowInfo* winInfo = getAllWindows()[0];
    WindowInfo* winInfo2 = getAllWindows()[1];
    WindowID stackingOrder[] = {winInfo->getID(), winInfo2->getID()};
    if(_i) {
        winInfo->moveToWorkspace(0);
        winInfo2->moveToWorkspace(0);
        if(_i == 2)
            switchToWorkspace(1);
    }
    assert(activateWindow(winInfo));
    assertEquals(getActiveFocus(getActiveMasterKeyboardID()), winInfo->getID());
    assert(activateWindow(winInfo2));
    assert(checkStackingOrder(stackingOrder, LEN(stackingOrder)));
    assertEquals(getActiveFocus(getActiveMasterKeyboardID()), winInfo2->getID());
});

static void createWMEnv(void) {
    createXSimpleEnv();
    registerForWindowEvents(root, ROOT_EVENT_MASKS);
}
SET_ENV(createWMEnv, fullCleanup);

MPX_TEST_ITER("test_raise_window_ignore_unmanaged", 2, {
    getEventRules(POST_REGISTER_WINDOW).clear();
    WindowID bottom = mapArbitraryWindow();
    WindowID top = mapArbitraryWindow();
    WindowID stackingOrder[] = {createOverrideRedirectWindow(), getWindowDivider(0), bottom, top, getWindowDivider(1), createOverrideRedirectWindow()};
    lowerWindow(stackingOrder[0]);
    scan(root);
    if(_i) {
        raiseWindow(getWindowInfo(bottom));
        raiseWindow(getWindowInfo(top));
        assert(checkStackingOrder(stackingOrder, LEN(stackingOrder)));
    }
    else {
        lowerWindow(getWindowInfo(top));
        lowerWindow(getWindowInfo(bottom));
        assert(checkStackingOrder(stackingOrder, LEN(stackingOrder)));
    }
});

MPX_TEST_ITER("window_divider", 2, {
    assertEquals(getWindowDivider(0), getWindowDivider(0));
    assertEquals(getWindowDivider(1), getWindowDivider(1));
    assert(getWindowDivider(0) != getWindowDivider(1));
    WindowID stack[] = {getWindowDivider(_i), getWindowDivider(!_i), getWindowDivider(_i)};
    assertEquals(stack[0], stack[2]);
    assert(checkStackingOrder(stack + _i, 2));
});

MPX_TEST("test_raise_window", {
    //windows are in same spot
    WindowID bottom = mapArbitraryWindow();
    WindowID top = mapArbitraryWindow();
    registerForWindowEvents(bottom, XCB_EVENT_MASK_VISIBILITY_CHANGE);
    registerForWindowEvents(top, XCB_EVENT_MASK_VISIBILITY_CHANGE);
    scan(root);
    raiseWindow(bottom);
    flush();
    WindowID stackingOrder[] = {top, bottom, top};
    assert(checkStackingOrder(stackingOrder, 2));
    raiseWindow(top);
    assert(checkStackingOrder(stackingOrder + 1, 2));
    lowerWindow(top);
    assert(checkStackingOrder(stackingOrder, 2));
});
MPX_TEST("test_raise_window_sibling", {
    //windows are in same spot
    WindowID bottom = mapArbitraryWindow();
    WindowID top = mapArbitraryWindow();
    WindowID sibling = mapArbitraryWindow();
    scan(root);
    lowerWindow(bottom);
    raiseWindow(top);
    raiseWindow(bottom, sibling);
    WindowID stackingOrder[] = {sibling, bottom, top};
    assert(checkStackingOrder(stackingOrder, 2));
    assert(checkStackingOrder(stackingOrder, 3));
    lowerWindow(sibling, bottom);
    assert(checkStackingOrder(stackingOrder, 3));
    lowerWindow(bottom, top);
    assert(checkStackingOrder(stackingOrder, 3));
    lowerWindow(top, sibling);
    assert(!checkStackingOrder(stackingOrder, 3));
});
MPX_TEST("test_raise_window_just_above", {
    WindowID bottom = mapArbitraryWindow();
    WindowID sibling = mapArbitraryWindow();
    WindowID top = mapArbitraryWindow();
    WindowID stackingOrder[] = {bottom, sibling,  top};
    assert(checkStackingOrder(stackingOrder, 3));
    raiseWindow(top, bottom);
    WindowID stackingOrder2[] = {bottom,  top, sibling };
    assert(checkStackingOrder(stackingOrder2, 3));
});

MPX_TEST("test_raise_lower_above_below_windows", {
    WindowID lower = mapArbitraryWindow();
    WindowID base = mapArbitraryWindow();
    WindowID upper = mapArbitraryWindow();
    WindowID stackingOrder[] = {lower, base, upper};
    scan(root);
    assert(checkStackingOrder(stackingOrder, 3));
    getWindowInfo(upper)->addMask(ABOVE_MASK);
    getWindowInfo(lower)->addMask(BELOW_MASK);
    for(WindowID win : stackingOrder)
        raiseWindow(getWindowInfo(win));
    assert(checkStackingOrder(stackingOrder, 3));
    lowerWindow(getWindowInfo(upper));
    assert(checkStackingOrder(stackingOrder, 3));
    raiseWindow(getWindowInfo(lower));
    assert(checkStackingOrder(stackingOrder, 3));
});

MPX_TEST("test_window_swap", {
    addWorkspaces(2);
    WindowID win[4];
    for(int i = 0; i < LEN(win); i++)
        win[i] = mapArbitraryWindow();
    scan(root);
    int nonActiveWorkspace = !getActiveWorkspaceIndex();
    WindowInfo* winInfo = getWindowInfo(win[0]);
    WindowInfo* winInfo2 = getWindowInfo(win[1]);
    WindowInfo* winInfo3 = getWindowInfo(win[2]);
    winInfo2->moveToWorkspace(0);
    winInfo3->moveToWorkspace(nonActiveWorkspace);
    assert(winInfo->getWorkspace() == NULL);

    for(int i = 0; i < 3; i++) {
        swapWindows(winInfo2, winInfo);
        validate();
        if(i % 2 == 0) {
            assert(winInfo2->getWorkspace() == NULL);
            assertEquals(winInfo->getWorkspaceIndex(), 0);
        }
        else {
            assert(winInfo->getWorkspace() == NULL);
            assertEquals(winInfo2->getWorkspaceIndex(), 0);
        }
    }

    assert(getWorkspace(0)->getWindowStack()[0] == winInfo);
    swapWindows(winInfo3, winInfo);
    assert(getWorkspace(1)->getWindowStack()[0] == winInfo);
    assert(getWorkspace(0)->getWindowStack()[0] == winInfo3);
    assert(winInfo->getWorkspaceIndex() == 1);
    swapWindows(winInfo, winInfo);
    assert(winInfo->getWorkspaceIndex() == 1);
});

MPX_TEST("test_configure_windows", {
    createNormalWindow();
    mapArbitraryWindow();
    createNormalWindow();
    scan(root);
    WindowMask mask = EXTERNAL_CONFIGURABLE_MASK ;
    ArrayList<WindowID> list = {mapArbitraryWindow(), createNormalWindow()};
    short values[] = {1, 2, 3, 4, 5};
    int allSizeMasks = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
        XCB_CONFIG_WINDOW_BORDER_WIDTH;
    int masks[] = {XCB_CONFIG_WINDOW_X, XCB_CONFIG_WINDOW_Y, XCB_CONFIG_WINDOW_WIDTH, XCB_CONFIG_WINDOW_HEIGHT,  XCB_CONFIG_WINDOW_BORDER_WIDTH};
    for(WindowInfo* winInfo : getAllWindows()) {
        list.add(winInfo->getID());
        if(winInfo->hasMask(MAPPED_MASK))
            assertEquals(0, processConfigureRequest(winInfo->getID(), values, 0, 0, allSizeMasks));
        winInfo->addMask(mask);
    }
    for(WindowID win : list) {
        uint32_t defaultValues[] = {10, 10, 10, 10, 10};
        for(int i = 0; i < LEN(masks); i++) {
            configureWindow(win, allSizeMasks, defaultValues);
            processConfigureRequest(win, values, 0, 0, masks[i]);
            waitForNormalEvent(XCB_CONFIGURE_NOTIFY);
            const RectWithBorder& rect = getRealGeometry(win);
            for(int n = 0; n < 4; n++)
                assert(rect[n] == (n == i ? values[0] : defaultValues[0]));
            assert(rect.border == (4 == i ? values[0] : defaultValues[0]));
        }
        configureWindow(win, allSizeMasks, defaultValues);
        processConfigureRequest(win, values, 0, 0, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT);
        const RectWithBorder& rect = getRealGeometry(win);
        assertEquals(rect.width, values[0]);
        assertEquals(rect.height, values[1]);
        WindowID stack[] = {getAllWindows()[0]->getID(), win};
        if(stack[0] != stack[1]) {
            int mask = XCB_CONFIG_WINDOW_STACK_MODE | XCB_CONFIG_WINDOW_SIBLING;
            assertEquals(mask, processConfigureRequest(win, values, getAllWindows()[0]->getID(), XCB_STACK_MODE_ABOVE, mask));
            assert(checkStackingOrder(stack, LEN(stack), 0));
        }
    }
});
MPX_TEST("remove_border", {
    WindowID win = createNormalWindow();
    scan(root);
    uint32_t border = 10;
    configureWindow(win, XCB_CONFIG_WINDOW_BORDER_WIDTH, &border);
    assertEquals(getRealGeometry(win).border, border);
    removeBorder((win));
    assertEquals(getRealGeometry(win).border, 0);
});

SET_ENV(openXDisplay, fullCleanup);
MPX_TEST_ERR("kill_window", 1, {
    suppressOutput();
    WindowID win = createNormalWindow();
    killClientOfWindow(win);
    flush();
    WAIT_UNTIL_TRUE(0);
});

MPX_TEST("kill_window_bad", {
    WindowInfo dummy = {.id = 1};
    dummy.addMask(WM_DELETE_WINDOW_MASK);
    killClientOfWindowInfo(&dummy);
});

MPX_TEST_ERR("test_kill_window", 1, {
    suppressOutput();
    registerWindow(mapArbitraryWindow(), root);
    killClientOfWindowInfo(getAllWindows()[0]);
    waitForNormalEvent(XCB_DESTROY_NOTIFY);
});
MPX_TEST("test_kill_window", {
    closeConnection();
    setLogLevel(LOG_LEVEL_NONE);
    WindowID win = 0;
    if(!spawnPipe(NULL)) {
        openXDisplay();
        win = createNormalWindow();
        assert(win);
        write(STATUS_FD_EXTERNAL_WRITE, &win, sizeof(win));
        suppressOutput();
        WAIT_UNTIL_TRUE(!waitForNormalEvent(1));
        closeConnection();
        quit(0);
    }
    int result = read(STATUS_FD_READ, &win, sizeof(win));
    if(result < sizeof(win)) {
        LOG(LOG_LEVEL_NONE, "killing failed %s", strerror(errno));
        assertEquals(result, sizeof(win));
    }
    assert(win);
    openXDisplay();
    killClientOfWindow(win);
    flush();
    closeConnection();
    assertEquals(waitForChild(0), 1);
});

MPX_TEST_ITER("test_delete_window_request", 8, {
    CRASH_ON_ERRORS = 0;
    bool forceKill = _i & 1;
    bool sleep = _i & 2;
    bool ping = _i & 4;
    KILL_TIMEOUT = 100;
    if(!spawnPipe(NULL)) {
        saveXSession();
        openXDisplay();
        WindowID win = mapArbitraryWindow();
        xcb_atom_t atoms[] = {WM_DELETE_WINDOW, ewmh->_NET_WM_PING};
        xcb_icccm_set_wm_protocols(dis, win, ewmh->WM_PROTOCOLS, ping ? 2 : 1, atoms);
        flush();
        consumeEvents();
        write(STATUS_FD_EXTERNAL_WRITE, &win, sizeof(win));
        close(STATUS_FD_EXTERNAL_WRITE);
        int result = read(STATUS_FD_EXTERNAL_READ, &win, sizeof(win));
        if(result < sizeof(win)) {
            assertEquals(result, sizeof(win));
        }
        close(STATUS_FD_EXTERNAL_READ);
        if(sleep) {
            msleep(KILL_TIMEOUT * 10);
            assert(!xcb_connection_has_error(dis));
            exit(1);
        }
        if(!forceKill) {
            /// wait for client message
            waitForNormalEvent(XCB_CLIENT_MESSAGE);
            exit(1);
        }
        else {
            flush();
            WAIT_UNTIL_TRUE(!xcb_connection_has_error(dis));
            exit(1);
        }
    }
    WindowID win;
    int result = read(STATUS_FD_READ, &win, sizeof(win));
    if(result < sizeof(win)) {
        assertEquals(result, sizeof(win));
    }
    assert(win);

    scan(root);
    WindowInfo* winInfo = getWindowInfo(win);
    assert(winInfo);
    loadWindowProperties(winInfo);
    assert(winInfo);
    assert(winInfo->hasMask(WM_DELETE_WINDOW_MASK));
    assertEquals(ping, winInfo->hasMask(WM_PING_MASK));
    consumeEvents();
    write(STATUS_FD, &win, sizeof(win));
    resetPipe();
    if(!forceKill) {
        killClientOfWindowInfo(winInfo);
        flush();
    }
    else {
        lock();
        delete getAllWindows().removeElement(win);
        assertEquals(killClientOfWindow(win), 0);
        unlock();
        LOG(0, "Killed client");
    }
    assertEquals(waitForChild(0), 1);
    fullCleanup();
});
MPX_TEST("test_delete_window_request_race", {
    createSimpleEnv();
    WindowInfo* winInfo = new WindowInfo(1, 1);
    winInfo->addMask(WM_DELETE_WINDOW_MASK);
    getAllWindows().add(winInfo);
    lock();
    killClientOfWindowInfo(winInfo);
    unregisterWindow(winInfo);
    unlock();
    waitForAllThreadsToExit();
});

/*
void multiWorkspaceStartup(void){
    CRASH_ON_ERRORS = -1;
    AUTO_FOCUS_NEW_WINDOW_TIMEOUT = -1;
    onSimpleStartup();
    switchToWorkspace(0);
    START_MY_WM;
    createNormalWindow();
    createNormalWindow();
    WAIT_UNTIL_TRUE(getAllWindows().size()== 2);
    lock();
    focusWindowInfo(getLast(getActiveWindowStack()));
    switchToWorkspace(1);
    createNormalWindow();
    createNormalWindow();
    unlock();
    WAIT_UNTIL_TRUE(getAllWindows().size()== 4);
    lock();
    focusWindowInfo(getHead(getActiveWindowStack()));
    switchToWorkspace(2);
    createNormalWindow();
    unlock();
    WAIT_UNTIL_TRUE(getAllWindows().size()== 5);
    assert(getWindowStackByMaster(getActiveMaster()).size()== 2);
}

MPX_TEST("test_workspaceless_window",{
    CRASH_ON_ERRORS = 0;
    setLogLevel(LOG_LEVEL_NONE);
    WindowInfo* winInfo = createWindowInfo(mapWindow(createNormalWindow()));
    addWindowInfo(winInfo);
    int index = getActiveWorkspaceIndex();
    START_MY_WM;
    int size = 6;
    for(int i = 0; i < size; i++){
        if(i == size / 2){
            WAIT_UNTIL_TRUE(getActiveWindowStack().size()== size / 2);
            ATOMIC(switchToWorkspace(!index));
        }
        ATOMIC(createNormalWindow());
    }
    WAIT_UNTIL_TRUE(getActiveWindowStack().size()== size / 2);
    assert(getAllWindows().size()== size + 1);
    ATOMIC(assert(getWorkspaceIndexOfWindow(winInfo) == NO_WORKSPACE));
    ATOMIC(assert(getWorkspaceOfWindow(winInfo) == NULL));
    activateWindow(winInfo);
    ATOMIC(assert(getWorkspaceIndexOfWindow(winInfo) == NO_WORKSPACE));
    ATOMIC(assert(getWorkspaceOfWindow(winInfo) == NULL));
    assert(getActiveWorkspaceIndex() == !index);
    ATOMIC(activateWorkspace(index));
    ATOMIC(activateWorkspace(!index));
    ATOMIC(switchToWorkspace(index));
    ATOMIC(switchToWorkspace(!index));
    ATOMIC(switchToWorkspace(index));
    ATOMIC(swapWorkspaces(index, !index));
    ATOMIC(swapWorkspaces(index, index));
    ATOMIC(swapWithWorkspace(index));
    ATOMIC(swapWithWorkspace(!index));
}
);

Suite* x11Suite(void){
    Suite* s = suite_create("Window Manager Functions");
    TCase* tc_core;
    tc_core = tcase_create("WorkspaceOperations");
    tcase_add_checked_fixture(tc_core, multiWorkspaceStartup, fullCleanup);
    tcase_add_test(tc_core, test_activate_workspace);
    tcase_add_test(tc_core, test_swap_workspace);
    tcase_add_test(tc_core, test_top_focused_window_synced);
    suite_add_tcase(s, tc_core);
    return s;
}
*/
