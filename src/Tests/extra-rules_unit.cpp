#include "../bindings.h"
#include "../devices.h"
#include "../ewmh.h"
#include "../extra-rules.h"
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

SET_ENV(onSimpleStartup, fullCleanup);
MPX_TEST("test_print_method", {
    addPrintStatusRule();
    printStatusMethod = incrementCount;
    // set to an open FD
    STATUS_FD = 1;
    applyEventRules(IDLE, NULL);
    assert(getCount());
});

MPX_TEST("test_desktop_rule", {
    addDesktopRule();
    Monitor* m = getActiveWorkspace()->getMonitor();
    WindowID stackingOrder[3] = {0, mapArbitraryWindow(), mapArbitraryWindow()};
    WindowID win = mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DESKTOP));
    stackingOrder[0] = win;

    setActiveLayout(GRID);
    scan(root);
    WindowInfo* winInfo = getWindowInfo(win);
    winInfo->moveToWorkspace(0);
    assert(winInfo->hasMask(STICKY_MASK | NO_TILE_MASK | BELOW_MASK));
    assert(!winInfo->hasMask(ABOVE_MASK));
    m->setViewport({10, 20, 100, 100});
    retile();
    flush();
    assert(checkStackingOrder(stackingOrder, LEN(stackingOrder)));
    assertEquals(RectWithBorder(m->getViewport()), getRealGeometry(winInfo->getID()));
    focusWindow(winInfo);
    assertEquals(getActiveFocus(), win);
});
MPX_TEST_ITER("desktop_focus_transfer", 2, {
    assert(getAllMonitors().size() == 1);
    addEWMHRules();
    addDesktopRule();
    startWM();
    WindowID normalWin;
    if(_i)
        normalWin = mapArbitraryWindow();
    WindowID win = mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DESKTOP));
    if(!_i)
        normalWin = mapArbitraryWindow();
    waitUntilIdle();
    assert(focusWindow(win));
    assert(focusWindow(normalWin));
    waitUntilIdle();
    Workspace* emptyWorkspace = getActiveWorkspace()->getNextWorkspace(1, EMPTY);
    assert(emptyWorkspace);
    auto index = getActiveWorkspaceIndex();
    ATOMIC(switchToWorkspace(*emptyWorkspace));
    waitUntilIdle();
    assertEquals(getActiveFocus(), win);
    assertEquals(getFocusedWindow(), getWindowInfo(win));
    ATOMIC(switchToWorkspace(index));
    waitUntilIdle();
    assertEquals(*getFocusedWindow(), normalWin);
});

MPX_TEST_ITER("detect_dock", 2, {
    addAvoidDocksRule();
    WindowID win = mapWindow(createNormalWindow());
    setWindowType(win, &ewmh->_NET_WM_WINDOW_TYPE_DOCK, 1);
    assert(catchError(xcb_ewmh_set_wm_strut_checked(ewmh, win, 0, 0, 1, 0)) == 0);
    scan(root);
    assert(getWindowInfo(win)->isDock());
    auto prop = getWindowInfo(win)->getDockProperties();
    assert(prop);
    for(int i = 0; i < 4; i++)
        if(i == 2)
            assert(prop[i] == 1);
        else
            assert(prop[i] == 0);
});

MPX_TEST_ITER("primary_monitor_windows", 2, {
    addAutoTileRules();
    getEventRules(PRE_REGISTER_WINDOW).add(DEFAULT_EVENT(+[](WindowInfo * winInfo) {winInfo->addMask(PRIMARY_MONITOR_MASK);}));
    addStickyPrimaryMonitorRule();
    addFakeMonitor({100, 100, 200, 200});
    addFakeMonitor({300, 100, 100, 100});
    Monitor* realMonitor = getAllMonitors()[0];
    getAllMonitors()[1]->setPrimary();
    detectMonitors();
    assignUnusedMonitorsToWorkspaces();
    assert(getPrimaryMonitor()->getWorkspace());
    WindowID win = mapWindow(createNormalWindow());
    startWM();
    waitUntilIdle();
    WindowInfo* winInfo = getWindowInfo(win);
    bool inWorkspace = _i;
    if(inWorkspace) {
        winInfo->addMask(NO_TILE_MASK);
        winInfo->moveToWorkspace(realMonitor->getWorkspace()->getID());
    }
    else {
        winInfo->setTilingOverrideEnabled(31);
        winInfo->setTilingOverride({0, 0, 100, 100});
    }

    mapWindow(createNormalWindow());
    waitUntilIdle();
    if(inWorkspace)
        assertEquals(*winInfo->getWorkspace()->getMonitor(), *getPrimaryMonitor());
    else {
        assert(getPrimaryMonitor()->getBase().contains(getRealGeometry(win)));
        Rect geo = winInfo->getTilingOverride();
        geo.x += getPrimaryMonitor()->getViewport().x;
        geo.y += getPrimaryMonitor()->getViewport().y;
        assertEquals(getRealGeometry(winInfo), geo);
    }
});

MPX_TEST("test_insertWindowsAtHeadOfStack", {
    addEWMHRules();
    addInsertWindowsAtHeadOfStackRule();
    Window win = mapArbitraryWindow();
    Window win2 = mapArbitraryWindow();
    addShutdownOnIdleRule();
    runEventLoop();
    assertEquals(getActiveWindowStack()[0]->getID(), win2);
    assertEquals(getActiveWindowStack()[1]->getID(), win);
});

MPX_TEST_ITER("test_auto_focus", 5, {
    addAutoFocusRule();
    WindowID focusHolder = mapArbitraryWindow();

    focusWindow(focusHolder);
    Window win = mapArbitraryWindow();
    registerWindow(win, root);
    assert(focusHolder == getActiveFocus(getActiveMasterKeyboardID()));
    xcb_map_notify_event_t event = {0};
    event.window = win;
    setLastEvent(&event);

    auto autoFocus = []() {
        getAllWindows().back()->addMask(INPUT_MASK | MAPPABLE_MASK | MAPPED_MASK);
        applyEventRules(XCB_MAP_NOTIFY, NULL);
    };
    bool autoFocused = 1;
    Master* master = getActiveMaster();
    switch(_i) {
        case 0:
            AUTO_FOCUS_NEW_WINDOW_TIMEOUT = 1000;
            break;
        case 1:
            AUTO_FOCUS_NEW_WINDOW_TIMEOUT = -1;
            break;
        case 2:
            AUTO_FOCUS_NEW_WINDOW_TIMEOUT = 0;
            autoFocused = 0;
            break;
        case 3:
            AUTO_FOCUS_NEW_WINDOW_TIMEOUT = -1;
            autoFocused = 0;
            event.window = 0;
            break;
        case 4:
            AUTO_FOCUS_NEW_WINDOW_TIMEOUT = 1000;
            createMasterDevice("test");
            initCurrentMasters();
            master = getAllMasters()[1];
            setClientPointerForWindow(win, master->getID());
            break;
    }
    autoFocus();
    assert(autoFocused == (win == getActiveFocus(master->getID())));
});
MPX_TEST("test_key_repeat", {
    getEventRules(XCB_INPUT_KEY_PRESS).add(DEFAULT_EVENT(+[]() {exit(10);}));
    addIgnoreKeyRepeat();
    xcb_input_key_press_event_t event = {.flags = XCB_INPUT_KEY_EVENT_FLAGS_KEY_REPEAT};
    setLastEvent(&event);
    assert(!applyEventRules(XCB_INPUT_KEY_PRESS));
});

MPX_TEST("test_transient_windows_always_above", {
    addKeepTransientsOnTopRule();
    WindowID win = mapArbitraryWindow();
    WindowID win2 = mapArbitraryWindow();
    WindowID win3 = mapArbitraryWindow();
    setWindowTransientFor(win2, win);
    registerWindow(win, root);
    registerWindow(win2, root);
    WindowInfo* winInfo = getWindowInfo(win);
    WindowInfo* winInfo2 = getWindowInfo(win2);
    winInfo->moveToWorkspace(getActiveWorkspaceIndex());
    winInfo2->moveToWorkspace(getActiveWorkspaceIndex());
    loadWindowProperties(winInfo2);
    assert(winInfo2->getTransientFor() == winInfo->getID());
    winInfo2->moveToWorkspace(getActiveWorkspaceIndex());
    assert(winInfo->isActivatable()&& winInfo2->isActivatable());
    assert(winInfo2->isMappable());
    WindowID stack[] = {win, win2, win3};
    startWM();
    waitUntilIdle();
    for(int i = 0; i < 4; i++) {
        assert(checkStackingOrder(stack, 2));
        if(i % 2)
            activateWindow(winInfo);
        else
            activateWindow(winInfo2);
        waitUntilIdle(1);
    }
});
MPX_TEST_ITER("border_for_floating", 2, {
    DEFAULT_BORDER_WIDTH = 1;
    addAutoTileRules();
    getEventRules(POST_REGISTER_WINDOW).add(DEFAULT_EVENT(floatWindow));
    addEWMHRules();
    WindowID win = mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DIALOG));
    startWM();
    waitUntilIdle();
    if(_i)
        assertEquals(getRealGeometry(win).border, DEFAULT_BORDER_WIDTH);
    else {
        floatWindow(getWindowInfo(win));
        mapArbitraryWindow();
        startWM();
        waitUntilIdle();
        retile();
        assertEquals(getRealGeometry(win).border, DEFAULT_BORDER_WIDTH);
    }
});
MPX_TEST_ITER("unmanaged_windows_above", 2, {
    MASKS_TO_SYNC |= ABOVE_MASK | BELOW_MASK;
    bool above = _i;
    addEWMHRules();
    addIgnoreOverrideRedirectWindowsRule(1);
    startWM();
    waitUntilIdle();
    WindowID win = createOverrideRedirectWindow(XCB_WINDOW_CLASS_INPUT_OUTPUT);
    sendChangeWindowStateRequest(win, XCB_EWMH_WM_STATE_ADD, above ? ewmh->_NET_WM_STATE_ABOVE : ewmh->_NET_WM_STATE_BELOW);
    waitUntilIdle();
    mapWindow(win);
    waitUntilIdle();
    assert(getWindowInfo(win)->hasMask(above ? ABOVE_MASK : BELOW_MASK));
});
MPX_TEST("shutdown_on_idle", {
    addShutdownOnIdleRule();
    createNormalWindow();
    runEventLoop();
    assert(isShuttingDown());
});
MPX_TEST("moveNonTileableWindowsToWorkspaceBounds", {
    addEWMHRules();
    getEventRules(CLIENT_MAP_ALLOW).add(DEFAULT_EVENT(floatWindow));
    addMoveNonTileableWindowsToWorkspaceBounds();
    getActiveMaster()->setWorkspaceIndex(0);
    Monitor* m = getAllMonitors()[0];
    Rect dims = m->getBase();
    dims.x += m->getBase().width;
    Monitor* m2 = addFakeMonitor(dims, "fake");
    m2->assignWorkspace(getWorkspace(1));
    startWM();
    WindowID win = mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DIALOG));
    waitUntilIdle();
    assert(m->getBase().contains(getRealGeometry(win)));
    getActiveMaster()->setWorkspaceIndex(1);
    WindowID win2 = mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DIALOG));
    waitUntilIdle();
    assert(m2->getBase().contains(getRealGeometry(win2)));
    assert(getRealGeometry(win) != getRealGeometry(win2));
    assert(dims.intersects(getRealGeometry(win2)) || dims.contains(getRealGeometry(win2)));
});

MPX_TEST("ignore_non_top_level_windows", {
    addIgnoreNonTopLevelWindowsRule();
    WindowID win = createNormalWindow();
    WindowID parent = createNormalWindow();
    startWM();
    waitUntilIdle();
    assert(getWindowInfo(win));
    assert(getWindowInfo(parent));
    xcb_reparent_window_checked(dis, win, parent, 0, 0);
    waitUntilIdle();
    assert(!getWindowInfo(win));
    xcb_reparent_window_checked(dis, win, root, 0, 0);
    waitUntilIdle();
    assert(getWindowInfo(win));
});

MPX_TEST("test_float_rule", {
    addFloatRule();
    mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DIALOG));
    mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_NOTIFICATION));
    mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_SPLASH));
    scan(root);
    for(WindowInfo* winInfo : getAllWindows())
        assert(winInfo->hasMask(FLOATING_MASK));
});
