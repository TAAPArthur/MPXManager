#include "../chain.h"
#include "../state.h"
#include "../devices.h"
#include "../ewmh.h"
#include "../extra-rules.h"
#include "../functions.h"
#include "../globals.h"
#include "../layouts.h"
#include "../logger.h"
#include "../window-properties.h"
#include "../windows.h"
#include "../wm-rules.h"
#include "../wmfunctions.h"
#include "../xsession.h"
#include "test-event-helper.h"
#include "tester.h"


int _main(int argc, char* const* argv) ;

static const char* const args[] = {"" __FILE__, "--no-event-loop"};

static void initSetup() {
    addDieOnIntegrityCheckFailRule();
    _main(2, (char* const*)args);
    assert(getActiveMaster());
    assert(getNumberOfWorkspaces());
}
SET_ENV(initSetup, fullCleanup);
MPX_TEST_ITER("auto_tile_with_dock", 4, {
    assignUnusedMonitorsToWorkspaces();
    bool premap = _i % 2;
    bool consume = _i / 2;
    WindowID win = createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DOCK);
    if(premap)
        mapWindow(win);
    setDockProperties(win, DOCK_TOP, 100);
    WindowID win2 = mapArbitraryWindow();
    WindowID win3 = mapArbitraryWindow();
    scan(root);

    if(consume)
        consumeEvents();
    if(!premap)
        mapWindow(win);
    startWM();
    waitUntilIdle(1);
    Monitor* m = getAllMonitors()[0];
    assert(m->getBase() != getAllMonitors()[0]->getViewport());
    assert(m->getViewport().contains(getRealGeometry(win2)));
    assert(m->getViewport().contains(getRealGeometry(win3)));
});

static void setup() {
    initSetup();
    startWM();
    waitUntilIdle(1);
    assert(getAllMonitors()[0]->getWorkspace());
}
SET_ENV(setup, fullCleanup);
MPX_TEST("auto_window_map", {
    lock();
    WindowID win = createNormalWindow();
    scan(root);
    unlock();
    waitUntilIdle();
    WindowInfo* winInfo = getWindowInfo(win);
    assert(winInfo);
    assert(winInfo->getWorkspace());
    assert(winInfo->hasMask(MAPPED_MASK));
});
MPX_TEST_ITER("auto_focus_window_when_switching_workspace", 2, {
    WorkspaceID base = _i;
    WorkspaceID other = !_i;
    switchToWorkspace(base);

    WindowID win = mapWindow(createNormalWindow());
    waitUntilIdle();
    WindowInfo* winInfo = getWindowInfo(win);
    assert(winInfo->hasMask(MAPPED_MASK));
    assertEquals(getFocusedWindow(), winInfo);
    focusWindow(winInfo);
    AUTO_FOCUS_NEW_WINDOW_TIMEOUT = 0;
    ATOMIC(switchToWorkspace(other));
    wakeupWM();
    waitUntilIdle();
    assert(getActiveFocus() != winInfo->getID());
    assert(!winInfo->hasMask(MAPPED_MASK));
    ATOMIC(switchToWorkspace(base));
    wakeupWM();
    waitUntilIdle();
    assert(winInfo->hasMask(MAPPED_MASK));
    assertEquals(getFocusedWindow(), winInfo);
    assertEquals(getActiveFocus(), winInfo->getID());
});
MPX_TEST_ITER("maintain_focus_when_moving_window_to_another_workspace", 2, {
    MONITOR_DUPLICATION_POLICY = 0;
    addFakeMonitor({100, 0, 100, 100});
    assignUnusedMonitorsToWorkspaces();
    mapWindow(createNormalWindow());
    WindowID win = mapWindow(createNormalWindow());
    waitUntilIdle();
    assertEquals(getActiveFocus(), win);
    ATOMIC(getWindowInfo(win)->moveToWorkspace(_i));
    waitUntilIdle(true);
    assertEquals(getActiveFocus(), win);
});

MPX_TEST("auto_focus", {
    lock();
    WindowID win = mapArbitraryWindow();
    setUserTime(win, 1);
    unlock();
    waitUntilIdle();
    WindowInfo* winInfo = getWindowInfo(win);
    assert(winInfo);
    assertEquals(getFocusedWindow(), winInfo);
});
MPX_TEST("test_dock_detection", {
    WindowID win = mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DOCK));
    waitUntilIdle();
    WindowInfo* winInfo = getWindowInfo(win);
    assert(winInfo);
    assert(winInfo->isDock());
});

MPX_TEST_ITER("stacking_order_with_transients_and_full_layout", 2, {
    setActiveLayout(FULL);
    WindowID win = mapArbitraryWindow();
    WindowID win2 = mapArbitraryWindow();
    WindowID win3 = mapArbitraryWindow();
    setWindowTransientFor(win2, win);
    waitUntilIdle();
    WindowInfo* winInfo = getWindowInfo(win);
    WindowInfo* winInfo2 = getWindowInfo(win2);
    assert(winInfo2->getTransientFor() == winInfo->getID());
    WindowID stack[] = {win, win2};
    WindowID stackPossible[][3] = {{win, win2, win3}, {win3, win2, win}};
    for(WindowInfo* winInfo : getAllWindows()) {
        lock();
        raiseWindow(winInfo, 0, _i);
        retile();
        unlock();
        waitUntilIdle(1);
        assert(checkStackingOrder(stack, LEN(stack)));
        assert(checkStackingOrder(stackPossible[0], LEN(stackPossible[0])) ||
            checkStackingOrder(stackPossible[1], LEN(stackPossible[1])));
    }
});

MPX_TEST("default_active_layout", {
    for(Workspace* w : getAllWorkspaces())
        assert(w->getActiveLayout());
});
MPX_TEST_ITER("auto_fullscreen", 4, {
    getAllMonitors()[0]->setViewport({10, 10, 10, 10});
    getAllMonitors()[0]->setBase({1, 2, 1000, 2000});
    static Window win = 0;
    getEventRules(POST_REGISTER_WINDOW).add(DEFAULT_EVENT(+[](WindowInfo * winInfo) {if(winInfo->getID() == win)winInfo->addMask(FULLSCREEN_MASK);}));
    switch(_i) {
        case 0:
            mapArbitraryWindow();
            waitUntilIdle();
            win = mapArbitraryWindow();
            break;
        case 1:
            win = mapArbitraryWindow();
            break;
        case 2:
            win = mapArbitraryWindow();
            mapArbitraryWindow();
            break;
        case 3:
            getEventRules(IDLE).add(DEFAULT_EVENT(+[]() {getWindowInfo(win)->addMask(MAPPABLE_MASK);}));
            win = mapArbitraryWindow();
            break;
    }
    waitUntilIdle();
    assertEquals(getRealGeometry(win), getAllMonitors()[0]->getBase());
});
MPX_TEST("click_to_focus", {
    setActiveLayout(GRID);
    WindowID wins[] = {mapArbitraryWindow(), mapArbitraryWindow()};
    waitUntilIdle();
    for(WindowID win : wins) {
        movePointer(1, 1, win);
        clickButton(1);
        waitUntilIdle();
        assert(getFocusedWindow());
        assertEquals(getFocusedWindow()->getID(), win);
    }
});
MPX_TEST_ITER("tile_invisible", 2, {
    setActiveLayout(GRID);
    mapArbitraryWindow();
    mapArbitraryWindow();
    waitUntilIdle();
    grabPointer();
    ATOMIC(getAllWindows()[0]->moveToWorkspace(1));
    wakeupWM();
    waitUntilIdle();
    if(_i)
        sendChangeWorkspaceRequest(1);
    else
        ATOMIC(switchToWorkspace(1));
    wakeupWM();
    waitUntilIdle();
    assertEquals(getActiveWorkspaceIndex(), 1);
    assertEquals(getRealGeometry(getAllWindows()[0]->getID()), getAllMonitors()[0]->getViewport());
    assert(getRealGeometry(getAllWindows()[0]->getID()) !=  getRealGeometry(getAllWindows()[1]->getID()));
});

MPX_TEST("detect_many_masters", {
    for(int i = getAllMasters().size(); i < MAX_NUMBER_OF_MASTER_DEVICES; i++)
        createMasterDevice("test");
    waitUntilIdle();
    assertEquals(getAllMasters().size(), MAX_NUMBER_OF_MASTER_DEVICES);
});

MPX_TEST_ITER("fullscreen_over_docks", 2, {
    WindowID dock = mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DOCK));
    setDockProperties(dock, DOCK_TOP, 100);
    WindowID win = mapArbitraryWindow();
    waitUntilIdle();
    lock();
    assert(getAllMonitors()[0]->getBase() != getAllMonitors()[0]->getViewport());
    getWindowInfo(dock)->addMask(ABOVE_MASK);
    getWindowInfo(win)->addMask(_i ? ROOT_FULLSCREEN_MASK : FULLSCREEN_MASK);
    retile();
    unlock();
    waitUntilIdle();
    assert(getRealGeometry(win).contains(getRealGeometry(dock)));
    WindowID stack[] = {dock, win};
    assert(checkStackingOrder(stack, 2));
});

MPX_TEST("transfer_focus_within_workspace", {
    CRASH_ON_ERRORS = 0;
    setLogLevel(LOG_LEVEL_NONE);
    addFakeMonitor({100, 0, 100, 100});
    assignUnusedMonitorsToWorkspaces();
    WindowID win = mapArbitraryWindow();
    WindowID win2 = mapArbitraryWindow();
    WindowID win3 = mapArbitraryWindow();
    waitUntilIdle();
    lock();
    for(WindowInfo* winInfo : getAllWindows())
        isWindowMapped(*winInfo);
    getWindowInfo(win2)->moveToWorkspace(1);
    assert(focusWindow(win));
    assert(focusWindow(win2));
    assert(focusWindow(win3));
    unlock();
    waitUntilIdle();
    assertEquals(getActiveMaster()->getFocusedWindow(), getWindowInfo(win3));
    assertEquals(3, getActiveMaster()->getWindowStack().size());
    destroyWindow(win3);
    waitUntilIdle();
    assertEquals(getActiveMaster()->getFocusedWindow(), getWindowInfo(win));
    destroyWindow(win);
    waitUntilIdle();
    assertEquals(getActiveMaster()->getFocusedWindow(), getWindowInfo(win2));
});

static void multiMonitorSetup() {
    MONITOR_DUPLICATION_POLICY = 0;
    setup();
    lock();
    addFakeMonitor({100, 0, 100, 100});
    addFakeMonitor({100, 0, 100, 100});
    detectMonitors();
    assertEquals(getAllMonitors().size(), 3);
    getActiveMaster()->setWorkspaceIndex(3);
    assignUnusedMonitorsToWorkspaces();
    unlock();
    for(int i = 0; i < getNumberOfWorkspaces(); i++)
        mapArbitraryWindow();
    waitUntilIdle();
    assertEquals(getActiveWorkspace()->getWindowStack().size(), getAllWindows().size());
    lock();
    for(int i = 0; i < getNumberOfWorkspaces(); i++)
        getAllWindows()[i]->moveToWorkspace(i);
    unlock();
    waitUntilIdle();
}
SET_ENV(multiMonitorSetup, fullCleanup);
MPX_TEST("switchWorkspace", {
    int count = 0;
    for(Workspace* w : getAllWorkspaces())
        count += w->isVisible();
    assertEquals(getAllMonitors().size(), count);
    assert(getActiveWorkspace()->getWindowStack().size());
    assert(getActiveMaster()->getWindowStack().size());
    for(int i = 0; i < 2; i++)
        for(Workspace* w : getAllWorkspaces()) {
            ATOMIC(switchToWorkspace(w->getID()));
            waitUntilIdle(1);
        }
});

static void  userEnvSetup() {
    multiMonitorSetup();
    WindowID win = createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DOCK);
    setDockProperties(win, DOCK_TOP, 100);
    for(int i = 0; i < 1; i++) {
        WindowID win2 = createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DOCK);
        setDockProperties(win2, i, 100);
        mapWindow(win2);
    }
    createOverrideRedirectWindow();
    waitUntilIdle();
    getWindowInfo(win)->addMask(STICKY_MASK | BELOW_MASK);
    mapWindow(win);
    mapArbitraryWindow();
    ATOMIC(switchToWorkspace(0));
    waitUntilIdle();
}
SET_ENV(userEnvSetup, fullCleanup);
MPX_TEST_ITER("stable", 2, {
    for(WindowInfo* winInfo : getAllWindows())
        raiseWindow(winInfo->getID(), 0, _i);
    waitUntilIdle();
    assert(!consumeEvents());
});
MPX_TEST_ITER("unmapped_override_redirect_windows", 2, {
    for(WindowInfo* winInfo : getAllWindows())
        if(winInfo->isOverrideRedirectWindow() && !isWindowMapped(winInfo->getID()))
            assert(!winInfo->isActivatable() && !winInfo->hasPartOfMask(MAPPED_MASK | MAPPABLE_MASK));
});
MPX_TEST_ITER("borderless_fullscreen", 2, {
    auto mask = _i ? FULLSCREEN_MASK : ROOT_FULLSCREEN_MASK;
    getEventRules(CLIENT_MAP_ALLOW).add({toggleMask, mask, "toggleMask"});
    WindowID win = mapArbitraryWindow();
    waitUntilIdle();
    assertEquals(0, getRealGeometry(win).border);
});
MPX_TEST("focus_unfocusable_window", {
    getEventRules(CLIENT_MAP_ALLOW).add({toggleMask, INPUT_MASK, "toggleMask"});
    WindowID win = mapArbitraryWindow();
    waitUntilIdle();
    assert(!getWindowInfo(win)->isFocusAllowed());
    assertEquals(0, getRealGeometry(win).border);
});
MPX_TEST_ITER("remove_all_monitors", 2, {
    ATOMIC(getAllMonitors().deleteElements());
    if(_i)
        getActiveWorkspace()->getWindowStack()[0]->addMask(STICKY_MASK);
    for(Workspace* w : getAllWorkspaces()) {
        ATOMIC(switchToWorkspace(w->getID()));
        mapArbitraryWindow();
        waitUntilIdle();
    }
});
MPX_TEST("add_hidden_mask", {
    MASKS_TO_SYNC = -1;
    lock();
    WindowInfo* winInfo = getActiveWorkspace()->getWindowStack()[0];
    assert(winInfo->hasMask(MAPPED_MASK));
    winInfo->addMask(HIDDEN_MASK);

    wakeupWM();
    unlock();
    waitUntilIdle();
    assert(!winInfo->hasMask(MAPPED_MASK));
    lock();
    winInfo->removeMask(HIDDEN_MASK);

    wakeupWM();
    unlock();
    waitUntilIdle();
    assert(winInfo->hasMask(MAPPED_MASK));
    lock();
    loadSavedAtomState(winInfo);
    assert(!winInfo->hasMask(HIDDEN_MASK));
    unlock();
});

static void bindingsSetup() {
    ROOT_DEVICE_EVENT_MASKS = 0;
    setup();
}
SET_ENV(bindingsSetup, fullCleanup);
MPX_TEST_ITER("cycle_window", 3, {
    mapArbitraryWindow();
    waitUntilIdle();
    sendKeyPress(getKeyCode(XK_Alt_L));
    if(_i == 2) {
        sendKeyPress(getKeyCode(XK_Shift_L));
    }
    for(int i = 0; i <= _i; i++) {
        typeKey(getKeyCode(XK_Tab));
        waitUntilIdle();
        assert(getActiveChain());
        assertEquals(getNumberOfActiveChains(), 1);
    }
    assert(getActiveMaster()->isFocusStackFrozen());
    sendKeyRelease(getKeyCode(XK_Tab));
    sendKeyRelease(getKeyCode(XK_Alt_L));
    waitUntilIdle();
    assert(!getActiveMaster()->isFocusStackFrozen());
    assert(!getActiveChain());
});
MPX_TEST_ITER("move_window", 2, {
    sendKeyPress(getKeyCode(XK_Super_L));
    bool move = !_i % 2;
    WindowID win = mapArbitraryWindow();
    waitUntilIdle();
    RectWithBorder rect = getRealGeometry(win);
    if(move) {
        rect.x++;
        rect.y++;
    }
    else {
        rect.width++;
        rect.height++;
    }
    movePointer(1, 1, win);
    sendButtonPress(move ? Button1 : Button3);
    waitUntilIdle();

    movePointerRelative(1, 1);
    waitUntilIdle();
    assertEquals(rect, getRealGeometry(win));
    sendButtonRelease(move ? Button1 : Button3);
    waitUntilIdle();
    movePointerRelative(1, 1);
    waitUntilIdle(1);
    assertEquals(rect, getRealGeometry(win));
});
