#include "../bindings.h"
#include "../devices.h"
#include "../extra-rules.h"
#include "../globals.h"
#include "../layouts.h"
#include "../logger.h"
#include "../state.h"
#include "../window-properties.h"
#include "../wm-rules.h"
#include "../wmfunctions.h"
#include "test-event-helper.h"
#include "tester.h"

/*
static Layout* startingLayout = &DEFAULT_LAYOUTS[FULL];
static void deviceEventsetup(){
    LOAD_SAVED_STATE = 0;
    CRASH_ON_ERRORS = -1;
    DEFAULT_WINDOW_MASKS = SRC_ANY;
    if(!startupMethod)
        startupMethod = sampleStartupMethod;
    NUMBER_OF_DEFAULT_LAYOUTS = 0;
    onSimpleStartup();
    setActiveLayout(startingLayout);
    defaultMaster = getActiveMaster();
    WindowID win1 = mapWindow(createNormalWindow());
    WindowID win2 = mapWindow(createNormalWindow());
    scan(root);
    xcb_icccm_set_wm_protocols(dis, win2, ewmh->WM_PROTOCOLS, 1, &ewmh->_NET_WM_PING);
    flush();
    createMasterDevice("device2");
    createMasterDevice("device3");
    flush();
    retile();
    int idle = getIdleCount();
    startWM();
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    winInfo = getWindowInfo(win1);
    winInfo2 = getWindowInfo(win2);
}

static void nonDefaultDeviceEventsetup(){
    startingLayout = &DEFAULT_LAYOUTS[GRID];
    startupMethod = addFocusFollowsMouseRule;
    deviceEventsetup();
}
*/

SET_ENV(NULL, fullCleanup);
MPX_TEST("test_on_startup", {
    onSimpleStartup();
    assert(getAllMonitors().size());
    assert(getAllMasters().size());
    assert(dis);
    assert(!xcb_connection_has_error(dis));
    assert(getActiveWorkspace()->isVisible());
});
MPX_TEST_ERR("test_handle_error", 1, {
    setLogLevel(LOG_LEVEL_NONE);
    CRASH_ON_ERRORS = -1;
    onSimpleStartup();
    xcb_generic_event_t event = {0};
    xcb_send_event(dis, 0, root, ROOT_EVENT_MASKS, (char*) &event);
    runEventLoop();
    assert(0);
});
MPX_TEST("handle_error_continue", {
    setLogLevel(LOG_LEVEL_NONE);
    CRASH_ON_ERRORS = 0;
    onSimpleStartup();
    xcb_generic_event_t event = {0};
    xcb_send_event(dis, 0, root, ROOT_EVENT_MASKS, (char*) &event);
    addShutdownOnIdleRule();
    runEventLoop();
});
MPX_TEST("auto_grab_bindings", {
    Binding b = {0, 1, requestShutdown};
    getDeviceBindings().add(b);
    onSimpleStartup();
    triggerBinding(&b);
    runEventLoop();
});

MPX_TEST("test_auto_tile", {
    getEventRules(X_CONNECTION).add(DEFAULT_EVENT(mapArbitraryWindow));
    addBasicRules();
    getEventRules(TILE_WORKSPACE).deleteElements();
    getEventRules(TILE_WORKSPACE).add(DEFAULT_EVENT(incrementCount));
    getEventRules(POST_REGISTER_WINDOW).add(DEFAULT_EVENT(autoResumeWorkspace));
    addAutoTileRules();
    createSimpleEnv();
    openXDisplay();
    applyEventRules(PERIODIC, NULL);
    applyBatchEventRules();
    applyEventRules(IDLE, NULL);
    assert(!isStateMarked());

    assertEquals(getCount(), 1);
    setActiveLayout(GRID);
    markState();
    updateState();
    assert(!isStateMarked());


    mapArbitraryWindow();
    mapArbitraryWindow();
    getAllMonitors()[0]->setViewport({0, 0, 60, 60});
    auto count = getCount();
    startWM();
    WAIT_UNTIL_TRUE(getCount() > count);
    waitUntilIdle();
    assertEquals(getCount(), count + 1);
});

SET_ENV(onSimpleStartup, fullCleanup);
MPX_TEST("load_properties_on_scan", {
    WindowID win = mapArbitraryWindow();
    xcb_atom_t atoms[] = {WM_DELETE_WINDOW, ewmh->_NET_WM_PING};
    xcb_icccm_set_wm_protocols(dis, win, ewmh->WM_PROTOCOLS, _i == 0 ? 2 : 1, atoms);
    flush();
    scan(root);
    assert(getWindowInfo(win)->hasMask(MAPPED_MASK));
    assert(getWindowInfo(win)->hasMask(WM_DELETE_WINDOW_MASK));
});

static inline void createWMEnvWithRunningWM() {
    POLL_COUNT = 1;
    POLL_INTERVAL = 10;
    onSimpleStartup();
    getEventRules(CLIENT_MAP_ALLOW).add(DEFAULT_EVENT(+[](WindowInfo * winInfo) {markState(); winInfo->moveToWorkspace(getActiveWorkspaceIndex());}));
    startWM();
    assert(getDeviceBindings().size() == 0);
    waitUntilIdle();
    for(Workspace* w : getAllWorkspaces())
        w->setActiveLayout(NULL);
}

SET_ENV(createWMEnvWithRunningWM, fullCleanup);

MPX_TEST("test_detect_primary_change", {
    getEventRules(SCREEN_CHANGE).add(DEFAULT_EVENT(incrementCount));
    setPrimary(NULL);
    setPrimary(getAllMonitors()[0]);
    waitUntilIdle();
    assert(getPrimaryMonitor());
    setPrimary(NULL);
    waitUntilIdle();
    assert(!getPrimaryMonitor());
    assertEquals(getCount(), 2);
});


MPX_TEST_ITER("test_detect_new_windows", 2, {
    NON_ROOT_EVENT_MASKS &= ~XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
    lock();
    WindowID win = createUnmappedWindow();
    WindowID child = createNormalSubWindow(win);
    WindowID win2 = createUnmappedWindow();
    destroyWindow(createUnmappedWindow());
    flush();
    if(_i)
        scan(root);
    unlock();
    waitUntilIdle();
    assertEquals(getAllWindows().size(), 2);
    createOverrideRedirectWindow();
    WindowID win3 = createUnmappedWindow();
    waitUntilIdle();
    assert(getAllWindows().find(win) &&
        getAllWindows().find(win2) &&
        getAllWindows().find(win3));
    assert(!getAllWindows().find(child));
    assertEquals(getAllWindows().size(), 3);
});
MPX_TEST("test_detect_new_override_redirect_windows", {
    assert(!addIgnoreOverrideRedirectWindowsRule(ADD_REMOVE));
    createOverrideRedirectWindow();
    createOverrideRedirectWindow();
    createOverrideRedirectWindow();
    waitUntilIdle();
    assertEquals(getAllWindows().size(), 3);
    for(WindowInfo* winInfo : getAllWindows())
        assert(winInfo->isOverrideRedirectWindow());
});

MPX_TEST("test_delete_windows", {
    CRASH_ON_ERRORS = 0;
    setLogLevel(LOG_LEVEL_NONE);
    lock();
    WindowID win = createUnmappedWindow();
    WindowID win2 = createUnmappedWindow();
    WindowID win3 = createUnmappedWindow();
    destroyWindow(win);
    mapWindow(win2);
    assert(getAllWindows().size() == 0);
    unlock();
    WindowID win4 = createUnmappedWindow();
    waitUntilIdle();
    assert(getAllWindows().find(win4)&&
        getAllWindows().find(win2)&&
        getAllWindows().find(win3));
    assert(!getAllWindows().find(win));
    destroyWindow(win2);
    destroyWindow(win3);
    destroyWindow(win4);
    waitUntilIdle();
    assert(!(getAllWindows().find(win2) ||
            getAllWindows().find(win3) ||
            getAllWindows().find(win4)));
    assert(getAllWindows().size() == 0);
});


MPX_TEST("test_property_update", {
    WindowID win = mapWindow(createNormalWindow());
    waitUntilIdle();
    std::string title = "dummy title";
    setWindowTitle(win, title);
    waitUntilIdle();
    assert(getWindowInfo(win)->getTitle() == title);
});
MPX_TEST("test_property_update_dock", {
    WindowID win = mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DOCK));
    waitUntilIdle();
    getWindowInfo(win)->setDock();
    int size = 20;
    setDockProperties(win, 0, size);
    waitUntilIdle();
    assertEquals(getWindowInfo(win)->getDockProperties()[0], size);
    assert(getAllMonitors()[0]->getBase() != getAllMonitors()[0]->getViewport());
    getWindowInfo(win)->removeFromWorkspace();
    unmapWindow(win);
    waitUntilIdle();
    assertEquals(getAllMonitors()[0]->getBase(), getAllMonitors()[0]->getViewport());
});
MPX_TEST("test_property_update_other", {
    WindowID win = mapWindow(createNormalWindow());
    waitUntilIdle();
    setWindowClass(win, "clazz", "Clazz");
    waitUntilIdle();
});
MPX_TEST("test_property_update_urgent", {
    WindowID win = mapWindow(createNormalWindow());
    waitUntilIdle();
    xcb_icccm_wm_hints_t hints;
    xcb_icccm_wm_hints_set_urgency(&hints);
    xcb_icccm_set_wm_hints(dis, win, &hints);
    waitUntilIdle();
    assert(getWindowInfo(win)->hasMask(URGENT_MASK));
});

MPX_TEST("test_map_windows", {
    WindowID win = createUnmappedWindow();
    WindowID win2 = createUnmappedWindow();
    WindowID win3 = createUnmappedWindow();
    waitUntilIdle();
    assertEquals(getAllWindows().size(), 3);
    mapWindow(win3);
    mapWindow(win2);
    waitUntilIdle();
    assert(getWindowInfo(win2)->hasMask(MAPPED_MASK));
    assert(getWindowInfo(win3)->hasMask(MAPPED_MASK));
    assert(!getWindowInfo(win)->hasMask(MAPPED_MASK));
    mapWindow(win);
    waitUntilIdle();
    for(WindowInfo* winInfo : getAllWindows())
        assert(winInfo->hasMask(MAPPED_MASK));
});

MPX_TEST("test_unmap", {
    addIgnoreOverrideRedirectWindowsRule(ADD_REMOVE);
    WindowID win = createUnmappedWindow();
    WindowID winOR = mapWindow(createOverrideRedirectWindow());
    waitUntilIdle();
    xcb_unmap_notify_event_t event = {.response_type = XCB_UNMAP_NOTIFY, .window = win};
    catchError(xcb_send_event_checked(dis, 0, root, ROOT_EVENT_MASKS, (char*) &event));
    flush();
    waitUntilIdle();
    assert(!getWindowInfo(win)->hasPartOfMask(MAPPED_MASK | MAPPABLE_MASK));
    assert(getWindowInfo(winOR)->hasPartOfMask(MAPPED_MASK | MAPPABLE_MASK));
    unmapWindow(winOR);
    waitUntilIdle();
    assert(!getWindowInfo(win)->hasPartOfMask(MAPPED_MASK | MAPPABLE_MASK));
    assert(!getWindowInfo(winOR)->hasPartOfMask(MAPPED_MASK | MAPPABLE_MASK));
});

MPX_TEST("test_map_unmap_dock", {
    WindowID win = createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DOCK);
    getBatchEventRules(SCREEN_CHANGE).add(DEFAULT_EVENT(incrementCount));
    waitUntilIdle();
    setDockProperties(win, 0, 10);
    getWindowInfo(win)->setDock();
    getWindowInfo(win)->moveToWorkspace(0);
    mapWindow(win);
    waitUntilIdle();
    assert(getAndResetCount());
    assert(getAllMonitors()[0]->getBase() != getAllMonitors()[0]->getViewport());
    unmapWindow(win);
    waitUntilIdle();
    assert(getAndResetCount());
    assertEquals(getAllMonitors()[0]->getBase(), getAllMonitors()[0]->getViewport());
    setDockProperties(win, 0, 1);
    mapWindow(win);
    waitUntilIdle();
    assertEquals(getAllMonitors()[0]->getBase().getArea() - getAllMonitors()[0]->getViewport().getArea(), getAllMonitors()[0]->getViewport().height);
    setDockProperties(win, 0, 2);
    waitUntilIdle();
    assertEquals(getAllMonitors()[0]->getBase().getArea() - getAllMonitors()[0]->getViewport().getArea(), getAllMonitors()[0]->getViewport().height * 2);
});

MPX_TEST("test_reparent_windows", {
    WindowID win = createNormalWindow();
    WindowID child = createNormalSubWindow(win);
    WindowID parent = createNormalWindow();
    waitUntilIdle();
    assert(getWindowInfo(win));
    assert(getWindowInfo(parent));
    assert(!getWindowInfo(child));
    assertEquals(0, catchError(xcb_reparent_window_checked(dis, child, root, 0, 0)));
    assertEquals(0, catchError(xcb_reparent_window_checked(dis, win, parent, 0, 0)));
    waitUntilIdle();
    assertEquals(root, getWindowInfo(child)->getParent());
    assertEquals(parent, getWindowInfo(win)->getParent());
    assertEquals(0, catchError(xcb_reparent_window_checked(dis, win, parent, 0, 0)));
});

MPX_TEST("test_switch_workspace_with_sticky_window", {
    switchToWorkspace(0);
    addAutoTileRules();
    addEWMHRules();
    WindowID stickyWin = mapArbitraryWindow();
    WindowID normalWin = mapArbitraryWindow();
    waitUntilIdle();
    getWindowInfo(stickyWin)->addMask(STICKY_MASK);
    focusWindow(stickyWin);
    focusWindow(normalWin);
    waitUntilIdle();
    ATOMIC(switchToWorkspace(1));
    waitUntilIdle();
    assertEquals(getActiveFocus(), stickyWin);
    ATOMIC(switchToWorkspace(0));
    waitUntilIdle();
    assertEquals(getActiveFocus(), stickyWin);
});

MPX_TEST("test_screen_configure", {
    waitForChild(spawn("xrandr --noprimary &> /dev/null"));
    waitUntilIdle(1);
    assert(!getAllMonitors()[0]->isPrimary());
    waitForChild(spawn("xrandr --output $MONITOR_NAME --primary &> /dev/null"));
    waitUntilIdle();
    assert(getAllMonitors()[0]->isPrimary());
});

MPX_TEST("test_device_event", {
    getDeviceBindings().add({0, 1, incrementCount, {}, "incrementCount"});
    assertEquals(getDeviceBindings().size(), 1);
    ATOMIC(triggerBinding(getDeviceBindings()[0]));
    flush();
    waitUntilIdle();
    assert(getCount());
});
MPX_TEST_ITER("test_master_device_add_remove", 2, {
    int numMaster = getAllMasters().size();
    createMasterDevice("test1");
    createMasterDevice("test2");
    flush();
    waitUntilIdle();
    assert(getAllMasters().size() == numMaster + 2);
    Master* m = _i ? NULL : getAllMasters()[1];
    for(Slave* slave : getAllSlaves())
        attachSlaveToMaster(slave, m);
    waitUntilIdle();
    assertEquals(getAllSlaves().size(), 2);
    if(m)
        assertEquals(m->getSlaves().size(), 2);
    lock();
    destroyAllNonDefaultMasters();
    flush();
    unlock();
    WAIT_UNTIL_TRUE(getAllMasters().size() == 1);
});
MPX_TEST("test_focus_update", {
    WindowID wins[] = { mapArbitraryWindow(), mapArbitraryWindow()};
    for(WindowID win : wins) {
        focusWindow(win);
        flush();
        waitUntilIdle();
        assert(getFocusedWindow());
        assert(getFocusedWindow()->getID() == win);
    }
});
MPX_TEST("selection_steal", {
    broadcastEWMHCompilence();
    atexit(fullCleanup);
    if(!fork()) {
        saveXSession();
        openXDisplay();
        STEAL_WM_SELECTION = 1;
        broadcastEWMHCompilence();
        quit(0);
    }
    assertEquals(waitForChild(0), 0);
    WAIT_UNTIL_TRUE(isShuttingDown());
});
MPX_TEST("steal_other_selection", {
    auto atom = ewmh->MANAGER;
    assert(catchError(xcb_set_selection_owner_checked(dis, getPrivateWindow(), atom, XCB_CURRENT_TIME)) == 0);
    if(!fork()) {
        saveXSession();
        openXDisplay();
        assert(catchError(xcb_set_selection_owner_checked(dis, getPrivateWindow(), atom, XCB_CURRENT_TIME)) == 0);
        fullCleanup();
        quit(0);
    }
    assertEquals(waitForChild(0), 0);
    waitUntilIdle();
});


static void clientSetup() {
    getEventRules(POST_REGISTER_WINDOW).add(DEFAULT_EVENT(+[](WindowInfo * winInfo) {winInfo->addMask(EXTERNAL_CONFIGURABLE_MASK);}));
    getEventRules(POST_REGISTER_WINDOW).add(DEFAULT_EVENT(+[](WindowInfo * winInfo) {winInfo->moveToWorkspace(0);}));
    getEventRules(PRE_REGISTER_WINDOW).add(DEFAULT_EVENT(+[](WindowInfo * winInfo) {return !winInfo->isInputOnly();}));
    onSimpleStartup();
    addAutoTileRules();
    startWM();
    waitUntilIdle();
    if(!fork()) {
        saveXSession();
        ROOT_EVENT_MASKS = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
        openXDisplay();
        registerForEvents();
    }
    else {
        assert(waitForChild(0) == 0);
        fullCleanup();
        exit(0);
    }
}
SET_ENV(clientSetup, fullCleanup);

MPX_TEST_ITER("test_map_request", 2, {
    mapWindow(_i ? createUnmappedWindow() : createOverrideRedirectWindow());
    flush();
    waitForNormalEvent(XCB_MAP_NOTIFY);
});
MPX_TEST("test_map_request_unregistered", {
    mapWindow(createTypelessInputOnlyWindow());
    flush();
    waitForNormalEvent(XCB_MAP_NOTIFY);
});


MPX_TEST("test_configure_request_no_error", {
    int values[] = {0, 0, 1000, 1000, 1, XCB_STACK_MODE_ABOVE};
    int mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
        XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
        XCB_CONFIG_WINDOW_BORDER_WIDTH | XCB_CONFIG_WINDOW_STACK_MODE;
    assert(!catchError(xcb_configure_window_checked(dis, createUnmappedWindow(), mask, values)));
});
MPX_TEST("test_configure_request", {
    CRASH_ON_ERRORS = 0;
    int value = 2;
    int allMasks = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
    XCB_CONFIG_WINDOW_BORDER_WIDTH;
    int masks[] = {XCB_CONFIG_WINDOW_X, XCB_CONFIG_WINDOW_Y, XCB_CONFIG_WINDOW_WIDTH, XCB_CONFIG_WINDOW_HEIGHT,  XCB_CONFIG_WINDOW_BORDER_WIDTH};
    int defaultValues[] = {10, 11, 12, 13, 14};
    WindowID win = createNormalWindow();
    msleep(1000);
    consumeEvents();
    for(int i = 0; i < LEN(masks); i++) {
        assert(!catchError(xcb_configure_window_checked(dis, win, allMasks, defaultValues)));
        waitForNormalEvent(XCB_CONFIGURE_NOTIFY);
        assert(!catchError(xcb_configure_window_checked(dis, win, masks[i], &value)));
        waitForNormalEvent(XCB_CONFIGURE_NOTIFY);
        xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, win), NULL);
        assert(reply);
        for(int n = 0; n < 5; n++)
            assertEquals((&reply->x)[n], (n == i ? value : defaultValues[n]));
        free(reply);
    }
});
