
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
#include "../logger.h"
#include "../window-properties.h"
#include "../wmfunctions.h"
#include "../xsession.h"


#include "tester.h"
#include "test-event-helper.h"

SET_ENV(createXSimpleEnv, NULL);
MPX_TEST("register_unregister", {
    WindowID win = createInputOnlyWindow();
    assertEquals(getAllWindows().size(), 0);
    assert(registerWindow(win, root) == 1);
    assertEquals(getNumberOfEventsTriggerSinceLastIdle(PreRegisterWindow), 1);
    assertEquals(getNumberOfEventsTriggerSinceLastIdle(PostRegisterWindow), 1);
    assertEquals(getAllWindows().size(), 1);
    assert(unregisterWindow(getWindowInfo(win)) == 1);
    assertEquals(getAllWindows().size(), 0);
});
MPX_TEST_ITER("register_bad_window", 2, {
    WindowID win = 2;
    if(_i) {
        win = createNormalWindow();
        assert(!catchError(xcb_destroy_window_checked(dis, win)));
    }
    assert(registerWindow(win, root) == 0);
    assert(getAllWindows().size() == 0);
    assert(unregisterWindow(getWindowInfo(win)) == 0);
}
             );

MPX_TEST_ITER("detect_input_only_window", 2, {
    WindowID win = createInputOnlyWindow();
    if(_i)
        scan(root);
    else
        registerWindow(win, root);
    assert(getWindowInfo(win)->hasMask(INPUT_ONLY_MASK));
});
MPX_TEST_ITER("detect_mapped_windows", 2, {
    WindowID win = createNormalWindow();
    WindowID win2 = mapArbitraryWindow();
    if(_i)
        scan(root);
    else {
        registerWindow(win, root);
        registerWindow(win2, root);
    }
    assert(!getWindowInfo(win)->hasMask(MAPPED_MASK));
    assert(getWindowInfo(win2)->hasMask(MAPPED_MASK));
});
MPX_TEST("test_window_scan", {
    scan(root);
    WindowID win = createUnmappedWindow();
    WindowID win2 = createUnmappedWindow();
    WindowID win3 = createUnmappedWindow();
    assert(!isWindowMapped(win));
    assert(!isWindowMapped(win2));
    assert(!isWindowMapped(win3));
    mapWindow(win);
    xcb_flush(dis);
    scan(root);
    assert(getWindowInfo(win));
    assert(getWindowInfo(win2));
    assert(getWindowInfo(win3));
    assert(getAllWindows().size() == 3);
}
        );
MPX_TEST("test_child_window_scan", {
    IGNORE_SUBWINDOWS = 1;
    int parent = createNormalWindow();
    WindowID win = createNormalSubWindow(parent);
    int child = createNormalSubWindow(win);
    int grandChild = createNormalSubWindow(child);
    scan(parent);
    assert(getWindowInfo(win));
    assert(!getWindowInfo(child));
    assert(getAllWindows().size() == 1);
    IGNORE_SUBWINDOWS = 0;
    scan(win);
    assert(getWindowInfo(child));
    assert(getWindowInfo(grandChild));
    assert(getAllWindows().size() == 3);
});


MPX_TEST("attempToMapWindow", {
    WindowInfo* winInfo = new WindowInfo(createNormalWindow());
    WindowInfo* winInfo2 = new WindowInfo(createNormalWindow());
    getAllWindows().add(winInfo);
    getAllWindows().add(winInfo2);
    winInfo->addMask(MAPPABLE_MASK);
    assert(attemptToMapWindow(winInfo->getID()) == 0);
    assert(isWindowMapped(winInfo->getID()));
    assert(attemptToMapWindow(winInfo2->getID()));
    assert(!isWindowMapped(winInfo2->getID()));
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
SET_ENV(onStartup, fullCleanup);

MPX_TEST("test_activate_window", {
    WindowID win = createUnmappedWindow();
    WindowID win2 = mapWindow(createNormalWindow());
    scan(root);
    assert(getWindowInfo(win2)->isActivatable());
    assert(!activateWindow(getWindowInfo(win)));
    assert(activateWindow(getWindowInfo(win2)));
    getWindowInfo(win2)->removeMask(ALL_MASK);
    assert(!activateWindow(getWindowInfo(win2)));
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
    assert(getActiveMaster()->getFocusedWindow() == winInfo);
    assert(checkStackingOrder(stackingOrder + 1, 2));
    assertEquals(getActiveFocus(getActiveMasterKeyboardID()), winInfo->getID());
    assert(activateWindow(winInfo2));
    assert(getActiveMaster()->getFocusedWindow() == winInfo2);
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
    WindowInfo* infoBottom = getWindowInfo(bottom);
    WindowInfo* infoTop = getWindowInfo(top);
    assert(raiseWindowInfo(infoBottom));
    flush();
    WindowID stackingOrder[] = {top, bottom, top};
    assert(checkStackingOrder(stackingOrder, 2));
    assert(raiseWindowInfo(infoTop));
    assert(checkStackingOrder(stackingOrder + 1, 2));
    assert(lowerWindowInfo(infoTop));
    assert(checkStackingOrder(stackingOrder, 2));
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

    swapWindows(winInfo2, winInfo);
    assert(winInfo->getWorkspaceIndex() == 0);
    assert(winInfo2->getWorkspace() == NULL);

    assert(getWorkspace(0)->getWindowStack()[0] == winInfo);
    swapWindows(winInfo3, winInfo);
    assert(getWorkspace(1)->getWindowStack()[0] == winInfo);
    assert(getWorkspace(0)->getWindowStack()[0] == winInfo3);
    assert(winInfo->getWorkspaceIndex() == 1);
    swapWindows(winInfo, winInfo);
    assert(winInfo->getWorkspaceIndex() == 1);
});

MPX_TEST("test_configure_windows", {
    WindowInfo* winInfo[] = {new WindowInfo(mapWindow(createNormalWindow())), new WindowInfo(mapWindow(createNormalWindow()))};
    WindowMask mask = EXTERNAL_RESIZE_MASK | EXTERNAL_MOVE_MASK | EXTERNAL_BORDER_MASK;
    for(int n = 0; n < LEN(winInfo); n++) {
        winInfo[n]->addMask(mask);
        getAllWindows().add(winInfo[n]);
        WindowID win = winInfo[n]->getID();
        short values[] = {1, 2, 3, 4, 5};
        int allMasks = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
                       XCB_CONFIG_WINDOW_BORDER_WIDTH;
        int masks[] = {XCB_CONFIG_WINDOW_X, XCB_CONFIG_WINDOW_Y, XCB_CONFIG_WINDOW_WIDTH, XCB_CONFIG_WINDOW_HEIGHT,  XCB_CONFIG_WINDOW_BORDER_WIDTH};
        int defaultValues[] = {10, 10, 10, 10, 10};
        for(int i = 0; i < LEN(masks); i++) {
            xcb_configure_window(dis, win, allMasks, defaultValues);
            processConfigureRequest(win, values, 0, 0, masks[i]);
            waitForNormalEvent(XCB_CONFIGURE_NOTIFY);
            xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, win), NULL);
            for(int n = 0; n < 5; n++)
                assert((&reply->x)[n] == (n == i ? values[0] : defaultValues[0]));
            free(reply);
        }
        xcb_configure_window(dis, win, allMasks, defaultValues);
        processConfigureRequest(win, values, 0, 0, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT);
        xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, win), NULL);
        assert(reply->width == values[0]);
        assert(reply->height == values[1]);
        free(reply);
        //TODO check below
        processConfigureRequest(win, NULL, winInfo[!n]->getID(), XCB_STACK_MODE_ABOVE,
                                XCB_CONFIG_WINDOW_STACK_MODE | XCB_CONFIG_WINDOW_SIBLING);
    }
});

/*
 * TODO

volatile int count = 0;
void markAndSleep(void){
    count = 1;
    unlock();
    while(!isShuttingDown())
        msleep(50);
    lock();
}
MPX_TEST("test_auto_focus_tiling",{
    AUTO_FOCUS_NEW_WINDOW_TIMEOUT = 10000;
    setActiveLayout(&DEFAULT_LAYOUTS[FULL]);
    int first = mapWindow(createNormalWindow());
    static Rule sleeperRule = {NULL, 0, BIND(markAndSleep)};
    flush();
    int idle = 0;
    START_MY_WM;
    //wait until first has settled in
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    assert(getFocusedWindow()->getID() == first);
    addToList(getEventRules(XCB_MAP_NOTIFY), &sleeperRule);
    int second = mapWindow(createNormalWindow());
    WindowID stackingOrder[] = {first, second};
    WAIT_UNTIL_TRUE(count);
    // there is a race between retiling and manually updating the focus state
    // the following code and the sleeprRule resolve the race such that retiling.
    // A layout like full will raise the focused window so if the state isn't updated,
    // the stacking order will be messed up
    lock();
    retile();
    flush();
    assert(checkStackingOrder(stackingOrder, 2));
    unlock();
}
);
*/
MPX_TEST("test_auto_focus_delete", {
    for(int i = 0; i < 3; i++)
        mapWindow(createNormalWindow());
    scan(root);
    assert(getAllWindows().size() == 3);
    for(WindowInfo* winInfo : getAllWindows()) {
        loadWindowProperties(winInfo);
        winInfo->moveToWorkspace(0);
        getActiveMaster()->onWindowFocus(winInfo->getID());
        raiseWindowInfo(winInfo);
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

SET_ENV(openXDisplay, NULL);
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

MPX_TEST("test_kill_window", {
    closeConnection();
    setLogLevel(LOG_LEVEL_NONE);
    WindowID win = 0;
    if(!spawnPipe(NULL, 0)) {
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

MPX_TEST_ITER("test_delete_window_request", 3, {
    CRASH_ON_ERRORS = 0;
    KILL_TIMEOUT = _i == 0 ? 0 : 100;
    if(!spawnPipe(NULL, 0)) {
        saveXSession();
        openXDisplay();
        WindowID win = mapArbitraryWindow();
        xcb_atom_t atoms[] = {WM_DELETE_WINDOW, ewmh->_NET_WM_PING};
        xcb_icccm_set_wm_protocols(dis, win, ewmh->WM_PROTOCOLS, _i == 0 ? 2 : 1, atoms);
        flush();
        consumeEvents();
        write(STATUS_FD_EXTERNAL_WRITE, &win, sizeof(win));
        close(STATUS_FD_EXTERNAL_WRITE);
        if(_i) {
            msleep(KILL_TIMEOUT * 10);
        }
        if(_i == 0) {
            /// wait for client message
            waitForNormalEvent(XCB_CLIENT_MESSAGE);
        }
        else {
            flush();
            WAIT_UNTIL_TRUE(!xcb_connection_has_error(dis));
        }
        suppressOutput();
        closeConnection();
        quit(10);
    }
    WindowID win;
    int result = read(STATUS_FD_READ, &win, sizeof(win));
    if(result < sizeof(win)) {
        LOG(LOG_LEVEL_ERROR, "killing failed %s\n", strerror(errno));
        assertEquals(result, sizeof(win));
    }
    resetPipe();
    assert(win);
    scan(root);
    WindowInfo* winInfo = getWindowInfo(win);
    assert(winInfo);
    loadWindowProperties(winInfo);
    assert(winInfo);
    assert(winInfo->hasMask(WM_DELETE_WINDOW_MASK));
    consumeEvents();
    lock();
    if(_i != 1) {
        startWM();
        killClientOfWindowInfo(winInfo);
        flush();
    }
    else {
        removeWindow(win);
        assert(killClientOfWindow(win) == 0);
        LOG(0, "Killed client\n");
    }
    unlock();
    assertEquals(waitForChild(0), 1);
    fullCleanup();
});

/*
void multiWorkspaceStartup(void){
    CRASH_ON_ERRORS = -1;
    AUTO_FOCUS_NEW_WINDOW_TIMEOUT = -1;
    onStartup();
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
