#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>

#include "tester.h"
#include "test-event-helper.h"
#include "../state.h"
#include "../extra-rules.h"
#include "../window-properties.h"
#include "../wm-rules.h"
#include "../logger.h"
#include "../globals.h"
#include "../wmfunctions.h"
#include "../layouts.h"
#include "../devices.h"
#include "../bindings.h"

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
    runEventLoop(NULL);
    assert(0);
});
MPX_TEST("handle_error_continue", {
    setLogLevel(LOG_LEVEL_NONE);
    CRASH_ON_ERRORS = 0;
    onSimpleStartup();
    xcb_generic_event_t event = {0};
    xcb_send_event(dis, 0, root, ROOT_EVENT_MASKS, (char*) &event);
    addShutdownOnIdleRule();
    runEventLoop(NULL);
});
MPX_TEST("auto_grab_bindings", {
    Binding b = {0, 1, requestShutdown};
    getDeviceBindings().add(b);
    onSimpleStartup();
    triggerBinding(&b);
    runEventLoop();
});

MPX_TEST("test_auto_tile", {
    getEventRules(onXConnection).add(DEFAULT_EVENT(mapArbitraryWindow));
    addBasicRules();
    getEventRules(TileWorkspace).deleteElements();
    getEventRules(TileWorkspace).add(DEFAULT_EVENT(incrementCount));
    getEventRules(PostRegisterWindow).add(DEFAULT_EVENT(autoResumeWorkspace));
    addAutoTileRules();
    createSimpleEnv();
    openXDisplay();
    applyBatchEventRules();
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
    getEventRules(PostRegisterWindow).add(DEFAULT_EVENT(+[](WindowInfo * winInfo) {markState(); winInfo->moveToWorkspace(getActiveWorkspaceIndex());}));
    startWM();
    assert(getDeviceBindings().size() == 0);
    waitUntilIdle();
    for(Workspace* w : getAllWorkspaces())
        w->setActiveLayout(NULL);
}

SET_ENV(createWMEnvWithRunningWM, fullCleanup);



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
    assertEquals(getActiveWindowStack().size(), 2);
    createIgnoredWindow();
    WindowID win3 = createUnmappedWindow();
    waitUntilIdle();
    assert(getAllWindows().find(win) &&
        getAllWindows().find(win2) &&
        getAllWindows().find(win3));
    assert(!getAllWindows().find(child));
    assert(getActiveWindowStack().size() == 3);
});
MPX_TEST("test_detect_new_override_redirect_windows", {
    assert(!addIgnoreOverrideRedirectWindowsRule(ADD_REMOVE));
    createIgnoredWindow();
    createIgnoredWindow();
    createIgnoredWindow();
    waitUntilIdle();
    assertEquals(getActiveWindowStack().size(), 3);
    for(WindowInfo* winInfo : getAllWindows())
        assert(winInfo->isOverrideRedirectWindow());
});
MPX_TEST("test_reparent_windows", {
    WindowID win = createNormalWindow();
    WindowID child = createNormalSubWindow(win);
    WindowID parent = createNormalWindow();
    waitUntilIdle();
    xcb_reparent_window_checked(dis, child, root, 0, 0);
    xcb_reparent_window_checked(dis, win, parent, 0, 0);
    waitUntilIdle();
    assertEquals(root, getWindowInfo(child)->getParent());
    assertEquals(parent, getWindowInfo(win)->getParent());
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

MPX_TEST("test_map_windows", {
    WindowID win = createUnmappedWindow();
    WindowID win2 = createUnmappedWindow();
    WindowID win3 = createUnmappedWindow();
    mapWindow(win3);
    mapWindow(win2);
    //wait for all to be in list
    WAIT_UNTIL_TRUE(getAllWindows().find(win)&&
        getAllWindows().find(win2)&&
        getAllWindows().find(win3));
    //WAIT_UNTIL_TRUE(!isWindowMapped(win)&&isWindowMapped(win2)&&isWindowMapped(win3));
    mapWindow(win);
    //wait for all to be mapped
    WAIT_UNTIL_TRUE(isWindowMapped(win)&& isWindowMapped(win2)&& isWindowMapped(win3));
});

MPX_TEST("test_unmap", {
    WindowID win = createUnmappedWindow();
    waitUntilIdle();
    xcb_unmap_notify_event_t event = {.response_type = XCB_UNMAP_NOTIFY, .window = win};
    catchError(xcb_send_event_checked(dis, 0, root, ROOT_EVENT_MASKS, (char*) &event));
    flush();
    waitUntilIdle();
    assert(!getWindowInfo(win)->hasMask(MAPPED_MASK));
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
    getEventRules(PostRegisterWindow).add(DEFAULT_EVENT(+[](WindowInfo * winInfo) {winInfo->addMask(EXTERNAL_CONFIGURABLE_MASK);}));
    getEventRules(PostRegisterWindow).add(DEFAULT_EVENT(+[](WindowInfo * winInfo) {winInfo->moveToWorkspace(0);}));
    getEventRules(PreRegisterWindow).add(DEFAULT_EVENT(+[](WindowInfo * winInfo) {return !winInfo->isInputOnly();}));
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
    mapWindow(_i ? createUnmappedWindow() : createIgnoredWindow());
    flush();
    waitForNormalEvent(XCB_MAP_NOTIFY);
});
MPX_TEST("test_map_request_unregistered", {
    mapWindow(createTypelessInputOnlyWindow());
    flush();
    waitForNormalEvent(XCB_MAP_NOTIFY);
});

MPX_TEST("test_configure_request", {
    int values[] = {2, 1, 1000, 1000, 1, XCB_STACK_MODE_ABOVE};
    int mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
        XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
        XCB_CONFIG_WINDOW_BORDER_WIDTH | XCB_CONFIG_WINDOW_STACK_MODE;
    for(int i = 0; i < 2; i++) {
        WindowID win = i ? createUnmappedWindow() : createIgnoredWindow();
        assert(!catchError(xcb_configure_window_checked(dis, win, mask, values)));
        waitForNormalEvent(XCB_CONFIGURE_NOTIFY);
    }
    consumeEvents();
    int allMasks = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
        XCB_CONFIG_WINDOW_BORDER_WIDTH;
    int masks[] = {XCB_CONFIG_WINDOW_X, XCB_CONFIG_WINDOW_Y, XCB_CONFIG_WINDOW_WIDTH, XCB_CONFIG_WINDOW_HEIGHT,  XCB_CONFIG_WINDOW_BORDER_WIDTH};
    int defaultValues[] = {10, 10, 10, 10, 10};
    WindowID win = createUnmappedWindow();
    for(int i = 0; i < LEN(masks); i++) {
        catchError(xcb_configure_window_checked(dis, win, allMasks, defaultValues));
        waitForNormalEvent(XCB_CONFIGURE_NOTIFY);
        catchError(xcb_configure_window_checked(dis, win, masks[i], values));
        waitForNormalEvent(XCB_CONFIGURE_NOTIFY);
        xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, win), NULL);
        for(int n = 0; n < 5; n++)
            assert((&reply->x)[n] == (n == i ? values[0] : defaultValues[0]));
        free(reply);
    }
});
