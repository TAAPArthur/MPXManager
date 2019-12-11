
#include <X11/Xatom.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/extensions/XI.h>
#include <X11/extensions/XInput2.h>
#include <X11/keysym.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>

#include "../globals.h"
#include "../state.h"
#include "../logger.h"
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
MPX_TEST("unregister_focus_transfer", {
    WindowID win = mapArbitraryWindow();
    WindowID win2 = mapArbitraryWindow();
    WindowID win3 = mapArbitraryWindow();
    scan(root);
    for(WindowInfo* winInfo : getAllWindows()) {
        winInfo->addMask(INPUT_MASK);
        winInfo->moveToWorkspace(0);
        assert(winInfo->isActivatable());
        assert(winInfo->isNotInInvisibleWorkspace());
    }
    getActiveMaster()->onWindowFocus(win2);
    getActiveMaster()->onWindowFocus(win);
    focusWindow(win);
    assertEquals(getActiveFocus(), win);
    assert(unregisterWindow(getWindowInfo(win)));
    flush();
    assertEquals(getActiveFocus(), win2);
    assert(unregisterWindow(getWindowInfo(win2)));
    assertEquals(getActiveFocus(), win3);
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
    static auto type = ewmh->_NET_WM_WINDOW_TYPE_TOOLTIP;
    WindowID win = mapWindow(createNormalWindowWithType(type));
    WindowID win2 = mapWindow(createNormalWindow());
    getEventRules(CLIENT_MAP_ALLOW).add(DEFAULT_EVENT(incrementCount));
    getEventRules(CLIENT_MAP_ALLOW).add({
        +[](WindowInfo * winInfo) {
            if(winInfo->getType() == type) {
                unregisterWindow(winInfo);
                return 0;
            }
            return 1;
        }, "filter", PASSTHROUGH_IF_TRUE});
    scan(root);

    assertEquals(getCount(), 2);
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
    WindowID win = createIgnoredWindow();
    scan(root);
    assert(!getWindowInfo(win));
});
MPX_TEST("test_window_scan", {
    scan(root);
    WindowID win = createUnmappedWindow();
    WindowID win2 = createUnmappedWindow();
    WindowID win3 = createUnmappedWindow();
    WindowID win4 = createIgnoredWindow();
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

MPX_TEST_ITER("syncMappedState", 2, {
    WindowInfo* winInfo = new WindowInfo(createNormalWindow());
    getAllWindows().add(winInfo);
    if(_i)
        winInfo->addMask(MAPPABLE_MASK);
    winInfo->moveToWorkspace(0);
    assert(winInfo->isNotInInvisibleWorkspace());
    assert(getWorkspace(0)->isVisible());
    syncMappedState(0);
    assert(isWindowMapped(winInfo->getID()) == _i);
    getWorkspace(0)->setMonitor(NULL);
    assert(!winInfo->isNotInInvisibleWorkspace());
    syncMappedState(0);
    flush();
    assert(!isWindowMapped(winInfo->getID()));
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
    assert(focusWindow(getAllWindows()[1]));
    assertEquals(getActiveFocus(), getAllWindows()[1]->getID());
    switchToWorkspace(1);
    markState();
    updateState();
    flush();
    assertEquals(getActiveFocus(), getAllWindows()[0]->getID());
});

SET_ENV(onSimpleStartup, fullCleanup);

MPX_TEST("test_activate_window", {
    WindowID win = createUnmappedWindow();
    WindowID win2 = mapWindow(createNormalWindow());
    WindowID win3 = mapWindow(createNormalWindow());
    scan(root);
    assert(getWindowInfo(win2)->isActivatable());
    assert(!activateWindow(getWindowInfo(win)));
    assert(activateWindow(getWindowInfo(win2)));
    getWindowInfo(win2)->removeMask(ALL_MASK);
    assert(!activateWindow(getWindowInfo(win2)));
    assert(getWindowInfo(win3)->isActivatable());
    getWindowInfo(win3)->moveToWorkspace(1);
    getWindowInfo(win3)->removeMask(MAPPED_MASK);
    assert(!getWorkspace(1)->isVisible());
    assert(getWindowInfo(win3)->isActivatable());
    markState();
    updateState();
    assert(activateWindow(getWindowInfo(win3)));
    markState();
    updateState();
    assertEquals(getActiveWorkspaceIndex(), 1);

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
        markState();
        updateState();
        assertEquals(winInfo->getWorkspaceIndex(), i % getNumberOfWorkspaces());
        validate();
    };
    switchToWorkspace(2);
    setActiveMaster(getAllMasters()[_i]);
    check(2);
    for(int i = 1; i < getNumberOfWorkspaces() + 1; i++) {
        switchToWorkspace(i % getNumberOfWorkspaces());
        check(i);
    }
});
MPX_TEST_ITER("test_workspace_activation", 3, {
    addWorkspaces(3);
    mapArbitraryWindow();
    mapArbitraryWindow();
    scan(root);
    WindowInfo* winInfo = getAllWindows()[0];
    WindowInfo* winInfo2 = getAllWindows()[1];
    WindowID stackingOrder[3] = {winInfo->getID(), winInfo2->getID(), winInfo->getID()};
    if(_i) {
        winInfo->moveToWorkspace(0);
        winInfo2->moveToWorkspace(0);
        if(_i == 2)
            switchToWorkspace(1);
    }
    assert(activateWindow(winInfo));
    assert(checkStackingOrder(stackingOrder + 1, 2));
    assertEquals(getActiveFocus(getActiveMasterKeyboardID()), winInfo->getID());
    assert(activateWindow(winInfo2));
    assert(checkStackingOrder(stackingOrder, 2));
    assertEquals(getActiveFocus(getActiveMasterKeyboardID()), winInfo2->getID());
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
});

static void createWMEnv(void) {
    createXSimpleEnv();
    registerForWindowEvents(root, ROOT_EVENT_MASKS);
}
SET_ENV(createWMEnv, fullCleanup);

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
            for(int n = 0; n < 5; n++)
                assert(rect[n] == (n == i ? values[0] : defaultValues[0]));
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

MPX_TEST("test_auto_focus_delete", {
    for(int i = 0; i < 3; i++)
        mapWindow(createNormalWindow());
    scan(root);
    assert(getAllWindows().size() == 3);
    for(WindowInfo* winInfo : getAllWindows()) {
        loadWindowProperties(winInfo);
        winInfo->moveToWorkspace(0);
        getActiveMaster()->onWindowFocus(winInfo->getID());
        raiseWindow(winInfo->getID());
    }
    getActiveWorkspace()->setActiveLayout(NULL);
    WindowInfo* head = getAllWindows()[0];
    WindowInfo* middle = getAllWindows()[1];
    WindowInfo* lastFocused = getAllWindows()[2];
    focusWindow(middle);
    getActiveMaster()->onWindowFocus(head->getID());
    getActiveMaster()->onWindowFocus(lastFocused->getID());

    WindowID stackingOrder[] = {head->getID(), middle->getID(), lastFocused->getID()};
    assert(checkStackingOrder(stackingOrder, 3));
    assert(destroyWindow(lastFocused->getID()) == 0);
    unregisterWindow(lastFocused);
    assert(getAllWindows().size() == 2);
    assert(checkStackingOrder(stackingOrder, 2));
    assert(head->getID() == getActiveFocus(getActiveMasterKeyboardID()));
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
        LOG(LOG_LEVEL_NONE, "killing failed %s\n", strerror(errno));
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
        LOG(0, "Killed client\n");
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
