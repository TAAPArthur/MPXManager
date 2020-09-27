#include "../devices.h"
#include "../functions.h"
#include "../globals.h"
#include "../layouts.h"
#include "../windows.h"
#include "../wm-rules.h"
#include "../settings.h"
#include "../wmfunctions.h"
#include "test-event-helper.h"
#include "test-wm-helper.h"
#include "tester.h"


SCUTEST_SET_ENV(onDefaultStartup, cleanupXServer);
SCUTEST_ITER(auto_tile_with_dock, 4) {
    assignUnusedMonitorsToWorkspaces();
    bool premap = _i % 2;
    bool consume = _i / 2;
    WindowID win = createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DOCK);
    if(premap)
        mapWindow(win);
    setEWMHDockProperties(win, DOCK_TOP, 100, 0, 0, 0);
    WindowID win2 = mapArbitraryWindow();
    WindowID win3 = mapArbitraryWindow();
    scan(root);
    if(consume)
        consumeEvents();
    if(!premap)
        mapWindow(win);
    runEventLoop();
    Monitor* m = getHead(getAllMonitors());
    assert(*(long*)&m->base != *(long*)&m->view);
    assert(contains(m->view, getRealGeometry(win2)));
    assert(contains(m->view, getRealGeometry(win3)));
}
SCUTEST(auto_window_map) {
    WindowID win = mapWindow(createNormalWindow());
    scan(root);
    WindowInfo* winInfo = getWindowInfo(win);
    assert(winInfo);
    assert(getWorkspaceOfWindow(winInfo));
    assert(isWindowMapped(winInfo->id));
    assert(hasMask(winInfo, MAPPED_MASK));
}
SCUTEST_ITER(maintain_focus_when_moving_window_to_another_workspace, 2) {
    MONITOR_DUPLICATION_POLICY = 0;
    addFakeMonitor((Rect) {100, 0, 100, 100});
    assignUnusedMonitorsToWorkspaces();
    mapWindow(createNormalWindow());
    WindowID win = mapWindow(createNormalWindow());
    scan(root);
    focusWindow(win, getActiveMaster());
    runEventLoop();
    assertEquals(getActiveFocus(getActiveMasterKeyboardID()), win);
    moveToWorkspace(getWindowInfo(win), _i);
    runEventLoop();
    assertEquals(getActiveFocus(getActiveMasterKeyboardID()), win);
}

SCUTEST(click_to_focus) {
    setActiveLayout(&GRID);
    WindowID wins[] = {mapArbitraryWindow(), mapArbitraryWindow()};
    runEventLoop();
    for(int i = 0; i < LEN(wins); i++) {
        warpPointer(1, 1, wins[i], getActiveMasterPointerID());
        clickButton(1, getActiveMasterPointerID());
        runEventLoop();
        assert(getFocusedWindow());
        assertEquals(getFocusedWindow()->id, wins[i]);
    }
}

SCUTEST(transfer_focus_within_workspace) {
    CRASH_ON_ERRORS = 0;
    addFakeMonitor((Rect) {100, 0, 100, 100});
    assignUnusedMonitorsToWorkspaces();
    WindowID win = mapArbitraryWindow();
    WindowID win2 = mapArbitraryWindow();
    WindowID win3 = mapArbitraryWindow();
    runEventLoop();
    FOR_EACH(WindowInfo*, winInfo, getAllWindows()) {
        isWindowMapped(winInfo->id);
        focusWindow(winInfo->id, getActiveMaster());
    }
    moveToWorkspace(getWindowInfo(win2), 1);
    runEventLoop();
    assertEquals(getFocusedWindow(), getWindowInfo(win3));
    assertEquals(3, getActiveMasterWindowStack()->size);
    destroyWindow(win3);
    runEventLoop();
    assertEquals(getFocusedWindow(), getWindowInfo(win));
    destroyWindow(win);
    runEventLoop();
    assertEquals(getFocusedWindow(), getWindowInfo(win2));
}
SCUTEST_ITER(stable, 2) {
    FOR_EACH(WindowInfo*, winInfo, getAllWindows()) {
        raiseLowerWindow(winInfo->id, 0, _i);
    }
    runEventLoop();
    assert(!consumeEvents());
}


SCUTEST(test_dock_not_auto_in_workspace) {
    void func(WindowInfo * winInfo) {
        winInfo->dock = 1;
    }
    addEvent(CLIENT_MAP_ALLOW, DEFAULT_EVENT(func, HIGHEST_PRIORITY));
    WindowID win = mapWindow(createNormalWindow());
    runEventLoop();
    assert(!getWorkspaceOfWindow(getWindowInfo(win)));
}


static void bindingsSetup() {
    ROOT_DEVICE_EVENT_MASKS = 0;
    addShutdownOnIdleRule();
    loadSettings();
    onStartup();
}
SCUTEST_SET_ENV(bindingsSetup, cleanupXServer);
SCUTEST_ITER(cycle_window, 3) {
    mapArbitraryWindow();
    sendKeyPress(getKeyCode(XK_Alt_L), getActiveMasterKeyboardID());
    if(_i == 2) {
        sendKeyPress(getKeyCode(XK_Shift_L), getActiveMasterKeyboardID());
    }
    for(int i = 0; i <= _i; i++) {
        typeKey(getKeyCode(XK_Tab), getActiveMasterKeyboardID());
        runEventLoop();
        assertEquals(getActiveMaster()->bindings.size, 1);
    }
    assert(isFocusStackFrozen());
    sendKeyRelease(getKeyCode(XK_Tab), getActiveMasterKeyboardID());
    sendKeyRelease(getKeyCode(XK_Alt_L), getActiveMasterKeyboardID());
    runEventLoop();
    assert(!isFocusStackFrozen());
    assertEquals(getActiveMaster()->bindings.size, 0);
}
SCUTEST_ITER(move_window, 2) {
    POLL_COUNT = 1;
    sendKeyPress(getKeyCode(XK_Super_L), getActiveMasterKeyboardID());
    bool move = !_i % 2;
    WindowID win = mapArbitraryWindow();
    setWindowPosition(win, (Rect) {0, 0, 10, 10});
    runEventLoop();
    Rect rect = getRealGeometry(win);
    if(move) {
        rect.x++;
        rect.y++;
    }
    else {
        rect.width++;
        rect.height++;
    }
    warpPointer(1, 1, win, getActiveMasterPointerID());
    sendButtonPress(move ? Button1 : Button3, getActiveMasterPointerID());
    runEventLoop();
    movePointerRelative(1, 1);
    runEventLoop();
    assertEqualsRect(rect, getRealGeometry(win));
    sendButtonRelease(move ? Button1 : Button3, getActiveMasterPointerID());
    runEventLoop();
    movePointerRelative(1, 1);
    runEventLoop();
    assertEqualsRect(rect, getRealGeometry(win));
}
SCUTEST(test_destroy_master_during_chain) {
    createMasterDevice("test");
    initCurrentMasters();
    setActiveMaster(getMasterByName("test"));
    POLL_COUNT = 1;
    sendKeyPress(getKeyCode(XK_Super_L), getActiveMasterKeyboardID());
    bool move = !_i % 2;
    WindowID win = mapArbitraryWindow();
    setWindowPosition(win, (Rect) {0, 0, 10, 10});
    runEventLoop();
    warpPointer(1, 1, win, getActiveMasterPointerID());
    sendButtonPress(move ? Button1 : Button3, getActiveMasterPointerID());
    runEventLoop();
    destroyMasterDevice(getActiveMasterKeyboardID());
    runEventLoop();
}
