#include <unistd.h>

#include "../../Extensions/ewmh.h"
#include "../../Extensions/extra-rules.h"
#include "../../bindings.h"
#include "../../devices.h"
#include "../../functions.h"
#include "../../globals.h"
#include "../../layouts.h"
#include "../../wm-rules.h"
#include "../../wmfunctions.h"
#include "../test-event-helper.h"
#include "../test-wm-helper.h"
#include "../tester.h"

SCUTEST_SET_ENV(onDefaultStartup, simpleCleanup);
SCUTEST(test_print_status, .iter = 3) {
    addPrintStatusRule();
    int pid = 0;
    if(_i) {
        if(_i == 2) {
            pid = spawnPipeChild(NULL, REDIRECT_CHILD_INPUT_ONLY);
            if(!pid) {
                char buffer[255];
                assert(STATUS_FD_EXTERNAL_READ);
                int result = -1;
                while(result = read(STATUS_FD_EXTERNAL_READ, buffer, LEN(buffer))) {
                    if(result == -1)
                        perror("Failed to read");
                    write(1, buffer, result);
                }
                assert(result != -1);
                exit(0);
            }
        }
        createMasterDevice("test");
        initCurrentMasters();
        mapWindow(createNormalWindow());
        runEventLoop();
        setActiveWorkspaceIndex(1);
        focusWindow(mapWindow(createNormalWindow()));
        runEventLoop();
        setActiveWorkspaceIndex(2);
        setActiveMaster(getElement(getAllMasters(), 1));
        setActiveMode(1);
    }
    runEventLoop();
    close(STATUS_FD);
    if(pid)
        assertEquals(0, waitForChild(pid));
}

/*TODO
SCUTEST(test_desktop_rule) {
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
    assertEquals(RectWithBorder(m->getViewport()), getRealGeometry(winInfo->id));
    focusWindow(winInfo);
    assertEquals(getActiveFocus(), win);
}
*/

SCUTEST_ITER(primary_monitor_windows, 2) {
    addStickyPrimaryMonitorRule();
    addEvent(POST_REGISTER_WINDOW, DEFAULT_EVENT(addMask, HIGHER_PRIORITY, .arg = {PRIMARY_MONITOR_MASK}));
    setPrimary(addFakeMonitor((Rect) {100, 100, 200, 200})->id);
    addFakeMonitor((Rect) {300, 100, 100, 100});
    Monitor* realMonitor = getHead(getAllMonitors());
    detectMonitors();
    assignUnusedMonitorsToWorkspaces();
    WindowID win = mapWindow(createNormalWindow());
    runEventLoop();
    WindowInfo* winInfo = getWindowInfo(win);
    assert(hasMask(winInfo, PRIMARY_MONITOR_MASK));
    bool inWorkspace = _i;
    if(inWorkspace) {
        addMask(winInfo, NO_TILE_MASK);
        moveToWorkspace(winInfo, getWorkspaceOfMonitor(realMonitor)->id);
    }
    else {
        setTilingOverrideEnabled(winInfo, 31);
        setTilingOverride(winInfo, (Rect) {0, 0, 100, 100});
    }
    mapWindow(createNormalWindow());
    runEventLoop();
    if(inWorkspace)
        assertEquals(getMonitor(getWorkspaceOfWindow(winInfo)), getPrimaryMonitor());
    else {
        dumpRect(getPrimaryMonitor()->base);
        dumpRect(getRealGeometry(win));
        assert(contains(getPrimaryMonitor()->base, getRealGeometry(win)));
        Rect geo = *getTilingOverride(winInfo);
        geo.x += getPrimaryMonitor()->view.x;
        geo.y += getPrimaryMonitor()->view.y;
        assertEqualsRect(geo, getRealGeometry(winInfo->id));
    }
}

SCUTEST(test_insertWindowsAtHeadOfStack, .iter = 3) {
    Window win1 = mapArbitraryWindow();
    Window winFocused = mapArbitraryWindow();
    Window win3 = mapArbitraryWindow();
    runEventLoop();
    verifyWindowStack(getActiveWindowStack(), (WindowID[]) {win1, winFocused, win3});
    focusWindow(winFocused);
    addInsertWindowsAtPositionRule(_i);
    Window winNew = mapArbitraryWindow();
    runEventLoop();
    assertEquals(4, getActiveWindowStack()->size);
    switch(_i) {
        case HEAD_OF_STACK:
            verifyWindowStack(getActiveWindowStack(), (WindowID[]) {winNew, win1, winFocused, win3});
            break;
        case BEFORE_FOCUSED:
            verifyWindowStack(getActiveWindowStack(), (WindowID[]) {win1, winNew,  winFocused, win3});
            break;
        case AFTER_FOCUSED:
            verifyWindowStack(getActiveWindowStack(), (WindowID[]) {win1,  winFocused, winNew, win3});
            break;
    }
}

SCUTEST(test_key_repeat) {
    addEvent(XCB_INPUT_KEY_PRESS, DEFAULT_EVENT(fail));
    addIgnoreKeyRepeat();
    xcb_input_key_press_event_t event = {.flags = XCB_INPUT_KEY_EVENT_FLAGS_KEY_REPEAT};
    assert(!applyEventRules(XCB_INPUT_KEY_PRESS, &event));
}

SCUTEST(test_transient_windows_always_above) {
    addKeepTransientsOnTopRule();
    WindowID win = mapArbitraryWindow();
    WindowID win2 = mapArbitraryWindow();
    setWindowTransientFor(win2, win);
    scan(root);
    WindowInfo* winInfo = getWindowInfo(win);
    WindowInfo* winInfo2 = getWindowInfo(win2);
    loadWindowProperties(winInfo2);
    assert(winInfo2->transientFor == winInfo->id);
    moveToWorkspace(winInfo, getActiveWorkspaceIndex());
    moveToWorkspace(winInfo2, getActiveWorkspaceIndex());
    assert(isActivatable(winInfo) && isActivatable(winInfo2));
    assert(isMappable(winInfo2));
    lowerWindowInfo(winInfo, 0);
    WindowID stack[] = {win, win2};
    runEventLoop();
    for(int i = 0; i < 4; i++) {
        assert(checkStackingOrder(stack, LEN(stack)));
        if(i % 2)
            activateWindow(winInfo);
        else
            activateWindow(winInfo2);
        runEventLoop();
    }
}

/* TODO
SCUTEST_ITER(border_for_floating, 2) {
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
}
*/

SCUTEST(moveNonTileableWindowsToWorkspaceBounds) {
    addEWMHRules();
    addEvent(CLIENT_MAP_ALLOW, DEFAULT_EVENT(floatWindow));
    addMoveNonTileableWindowsToWorkspaceBounds();
    Monitor* m = getHead(getAllMonitors());
    Rect dims = m->base;
    dims.x += m->base.width;
    Monitor* m2 = addFakeMonitor(dims);
    assignWorkspace(m2, getWorkspace(1));
    WindowID win = mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DIALOG));
    runEventLoop();
    assert(contains(m->base, getRealGeometry(win)));
    setActiveWorkspaceIndex(1);
    WindowID win2 = mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DIALOG));
    runEventLoop();
    assert(contains(m2->base, getRealGeometry(win2)));
    dumpRect(dims);
    dumpRect(getRealGeometry(win2));
    assert(contains(dims, getRealGeometry(win2)));
}

SCUTEST(ignore_non_top_level_windows) {
    addIgnoreNonTopLevelWindowsRule();
    addDetectReparentEventRule();
    WindowID win = createNormalWindow();
    WindowID parent = createNormalWindow();
    runEventLoop();
    assert(getWindowInfo(win));
    assert(getWindowInfo(parent));
    xcb_reparent_window_checked(dis, win, parent, 0, 0);
    runEventLoop();
    assert(!getWindowInfo(win));
    xcb_reparent_window_checked(dis, win, root, 0, 0);
    runEventLoop();
    assert(getWindowInfo(win));
}

SCUTEST(test_float_rule) {
    addFloatRule();
    WindowID dialogWin = mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DIALOG));
    WindowID notifiWin = mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_NOTIFICATION));
    WindowID splashWin = mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_SPLASH));
    scan(root);
    assert(hasMask(getWindowInfo(dialogWin), FLOATING_MASK));
    assert(hasMask(getWindowInfo(notifiWin), FLOATING_MASK));
    assert(hasMask(getWindowInfo(splashWin), FLOATING_MASK));
}
