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
#include "../wm-rules.h"
#include "../extra-rules.h"
#include "../logger.h"
#include "../ewmh.h"
#include "../globals.h"
#include "../window-properties.h"
#include "../wmfunctions.h"
#include "../extra-rules.h"
#include "../layouts.h"
#include "../devices.h"
#include "../bindings.h"

SET_ENV(onSimpleStartup, fullCleanup);
MPX_TEST("test_print_method", {
    addPrintStatusRule();
    printStatusMethod = incrementCount;
    // set to an open FD
    STATUS_FD = 1;
    applyEventRules(Idle, NULL);
    assert(getCount());
});

MPX_TEST("test_die_on_idle", {
    addDieOnIdleRule();
    createNormalWindow();
    flush();
    atexit(fullCleanup);
    runEventLoop();
});
MPX_TEST("shutdown_on_idle", {
    addShutdownOnIdleRule();
    createNormalWindow();
    runEventLoop();
    assert(isShuttingDown());
});
MPX_TEST("test_desktop_rule", {
    addDesktopRule();
    Monitor* m = getActiveWorkspace()->getMonitor();
    WindowID win = mapWindow(createNormalWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DESKTOP));
    setActiveLayout(GRID);
    scan(root);
    WindowInfo* winInfo = getWindowInfo(win);
    winInfo->moveToWorkspace(0);
    assert(winInfo->hasMask(STICKY_MASK | NO_TILE_MASK | BELOW_MASK));
    assert(!winInfo->hasMask(ABOVE_MASK));
    m->setViewport({10, 20, 100, 100});
    retile();
    flush();
    assertEquals(m->getViewport(), getRealGeometry(winInfo->getID()));
});
MPX_TEST("test_float_rule", {
    addFloatRule();
    mapWindow(createNormalWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DIALOG));
    mapWindow(createNormalWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DESKTOP));
    mapWindow(createNormalWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_SPLASH));
    scan(root);
    for(WindowInfo* winInfo : getAllWindows())
        assert(winInfo->hasMask(FLOATING_MASK));
});
MPX_TEST_ITER("test_ignored_windows", 2, {
    addUnknownInputOnlyWindowIgnoreRule();
    if(_i)
        createTypelessInputOnlyWindow();
    else
        mapWindow(createTypelessInputOnlyWindow());
    scan(root);
    assertEquals(getAllWindows().size(), _i);
});

MPX_TEST_ITER("detect_dock", 2, {
    addAvoidDocksRule();
    if(_i)
        addNoDockFocusRule();
    WindowID win = mapWindow(createNormalWindow());
    setWindowType(win, &ewmh->_NET_WM_WINDOW_TYPE_DOCK, 1);
    assert(catchError(xcb_ewmh_set_wm_strut_checked(ewmh, win, 0, 0, 1, 0)) == 0);
    scan(root);
    assert(getWindowInfo(win)->isDock());
    assert(getWindowInfo(win)->hasMask(INPUT_MASK) == !_i);
    auto prop = getWindowInfo(win)->getDockProperties();
    assert(prop);
    for(int i = 0; i < 4; i++)
        if(i == 2)
            assert(prop[i] == 1);
        else
            assert(prop[i] == 0);
});

MPX_TEST("test_always_on_top_bottom", {
    addAlwaysOnTopBottomRules();
    //windows are in same spot
    WindowID bottom = createNormalWindow();
    WindowID bottom2 = createNormalWindow();
    WindowID normal = createNormalWindow();
    WindowID top = createNormalWindow();
    WindowID top2 = createNormalWindow();
    scan(root);
    getWindowInfo(bottom)->addMask(ALWAYS_ON_BOTTOM);
    getWindowInfo(bottom2)->addMask(ALWAYS_ON_BOTTOM);
    getWindowInfo(top)->addMask(ALWAYS_ON_TOP);
    getWindowInfo(top2)->addMask(ALWAYS_ON_TOP);
    assert(raiseWindowInfo(getWindowInfo(normal)));
    assert(raiseWindowInfo(getWindowInfo(bottom)));
    assert(lowerWindowInfo(getWindowInfo(top)));
    setActiveLayout(NULL);
    flush();
    startWM();
    waitUntilIdle();
    WindowID stackingOrder[][3] = {
        {bottom, normal, top},
        {bottom2, normal, top2},
        {bottom, normal, top2},
        {bottom2, normal, top},
    };
    for(int i = 0; i < LEN(stackingOrder); i++)
        assert(checkStackingOrder(stackingOrder[i], 3));
    msleep(POLL_COUNT* POLL_INTERVAL * 2);
    assert(consumeEvents() == 0);
});
MPX_TEST("test_focus_follows_mouse", {
    addEWMHRules();
    addFocusFollowsMouseRule();
    setActiveLayout(GRID);
    mapArbitraryWindow();
    mapArbitraryWindow();
    scan(root);
    retile();
    WindowInfo* winInfo = getAllWindows()[0];
    WindowInfo* winInfo2 = getAllWindows()[1];
    WindowID id1 = winInfo->getID();
    WindowID id2 = winInfo2->getID();
    focusWindow(id1);
    movePointer(0, 0, getActiveMasterPointerID(), id1);
    flush();
    startWM();
    WAIT_UNTIL_TRUE(getActiveFocus(getActiveMaster()->getID()) == id1);
    for(int i = 0; i < 4; i++) {
        int id = (i % 2 ? id1 : id2);
        int n = 0;
        WAIT_UNTIL_TRUE(getActiveFocus(getActiveMaster()->getID()) == id,
            movePointer(n, n, getActiveMasterPointerID(), id);
            n = !n;
            flush()
        );
    }
});

MPX_TEST_ITER("test_auto_focus", 4, {
    addAutoFocusRule();
    WindowID focusHolder = mapArbitraryWindow();

    focusWindow(focusHolder);
    Window win = mapArbitraryWindow();
    setUserTime(win, 1);
    registerWindow(win, root);
    assert(focusHolder == getActiveFocus(getActiveMasterKeyboardID()));
    xcb_map_notify_event_t event = {0};
    event.window = win;
    setLastEvent(&event);

    auto autoFocus = []() {
        getAllWindows().back()->addMask(INPUT_MASK | MAPPABLE_MASK | MAPPED_MASK);
        loadWindowProperties(getAllWindows().back());
        applyEventRules(XCB_MAP_NOTIFY, NULL);
    };
    bool autoFocused = 1;
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
    }
    autoFocus();
    assert(autoFocused == (getAllWindows().back()->getID() == getActiveFocus(getActiveMasterKeyboardID())));
});
MPX_TEST_ITER("ignore_small_window", 3, {
    addIgnoreSmallWindowRule();
    WindowID win = mapArbitraryWindow();
    xcb_size_hints_t hints = {0};
    if(_i)
        xcb_icccm_size_hints_set_size(&hints, _i - 1, 1, 1);
    else
        xcb_icccm_size_hints_set_base_size(&hints, 1, 1);
    assert(!catchError(xcb_icccm_set_wm_size_hints_checked(dis, win, XCB_ATOM_WM_NORMAL_HINTS, &hints)));
    flush();
    scan(root);
    if(_i != 1)
        assert(getWindowInfo(win));
    else
        assert(getWindowInfo(win) == NULL);
});
MPX_TEST("test_detect_sub_windows", {
    NON_ROOT_EVENT_MASKS |= XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
    addScanChildrenRule();
    WindowID win = createUnmappedWindow();
    WindowID win2 = createNormalSubWindow(win);
    flush();
    startWM();
    waitUntilIdle();
    WindowID win3 = createNormalSubWindow(win2);
    waitUntilIdle();
    assert(getAllWindows().find(win));
    assert(getAllWindows().find(win2));
    assert(getAllWindows().find(win3));
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
    assert(winInfo2->isInteractable());
    WindowID stack[] = {win, win2};
    startWM();
    for(int i = 0; i < 2; i++) {
        assert(checkStackingOrder(stack, 2));
        activateWindow(winInfo);
        waitUntilIdle(1);
    }
});
MPX_TEST_ITER("border_for_transients", 2, {
    DEFAULT_BORDER_WIDTH = 1;
    addEWMHRules();
    addDefaultBorderRule();
    WindowID win = createNormalWindow();
    removeBorder(win);
    mapWindow(win);
    registerWindow(win, root);
    flush();
    if(_i)
        assertEquals(getRealGeometry(win).border, DEFAULT_BORDER_WIDTH);
    else {
        floatWindow(getWindowInfo(win));
        retile();
        assertEquals(getRealGeometry(win).border, DEFAULT_BORDER_WIDTH);
    }
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
