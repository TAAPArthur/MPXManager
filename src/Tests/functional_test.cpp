#include "../chain.h"
#include "../devices.h"
#include "../ewmh.h"
#include "../extra-rules.h"
#include "../functions.h"
#include "../globals.h"
#include "../layouts.h"
#include "../logger.h"
#include "../window-properties.h"
#include "../wm-rules.h"
#include "../wmfunctions.h"
#include "../xsession.h"
#include "test-event-helper.h"
#include "tester.h"


int _main(int argc, char* const* argv) ;

static const char* const args[] = {"" __FILE__, "--no-event-loop"};
static void setup() {
    addDieOnIntegrityCheckFailRule();
    _main(2, (char* const*)args);
    startWM();
    waitUntilIdle();
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
    lock();
    WindowID win = createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DOCK);
    scan(root);
    unlock();
    waitUntilIdle();
    mapWindow(win);
    waitUntilIdle();
    WindowInfo* winInfo = getWindowInfo(win);
    assert(winInfo);
    assert(winInfo->isDock());
});

MPX_TEST("default_active_layout", {
    for(Workspace* w : getAllWorkspaces())
        assert(w->getActiveLayout());
});
MPX_TEST_ITER("default_layout", getRegisteredLayouts().size(), {
    DEFAULT_BORDER_WIDTH = 0;
    setActiveLayout(getRegisteredLayouts()[_i]);
    for(int i = 0; i < 3; i++) {
        mapArbitraryWindow();
    }
    waitUntilIdle();
    int area = 0;
    for(WindowInfo* winInfo : getAllWindows())
        if(winInfo->isVisible())
            area += getRealGeometry(winInfo->getID()).getArea();
    assertEquals(area, getActiveWorkspace()->getMonitor()->getViewport().getArea());
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
            win = createNormalWindow();
            waitUntilIdle();
            mapWindow(win);
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
MPX_TEST("test_always_on_top_bottom_conflicting_masks", {
    addAlwaysOnTopBottomRules();
    lock();
    WindowID wins[4] = {
        createNormalWindow(),
        createNormalWindow(),
        createNormalWindow(),
        createNormalWindow(),
    };
    scan(root);
    getWindowInfo(wins[0])->addMask(ALWAYS_ON_BOTTOM_MASK | ALWAYS_ON_TOP_MASK);
    getWindowInfo(wins[1])->addMask(ALWAYS_ON_BOTTOM_MASK | ABOVE_MASK);
    getWindowInfo(wins[3])->addMask(ALWAYS_ON_TOP_MASK | BELOW_MASK);
    for(WindowID win : wins) {
        lowerWindow(win);
    }
    unlock();
    waitUntilIdle();
    assert(checkStackingOrder(wins, LEN(wins)));
});
MPX_TEST_ITER("tile_invisible", 2, {
    setActiveLayout(GRID);
    mapArbitraryWindow();
    mapArbitraryWindow();
    waitUntilIdle();
    grabPointer();
    for(int i = 0; i < 2; i++) {
        ATOMIC(getAllWindows()[0]->moveToWorkspace(1));
        waitUntilIdle();
        if(_i)
            sendChangeWorkspaceRequest(1);
        else {
            movePointerRelative(1, 1);
            switchToWorkspace(1);
        }
        waitUntilIdle();
        assertEquals(getActiveWorkspaceIndex(), 1);
        if(i)
            assertEquals(getRealGeometry(getAllWindows()[0]->getID()), getAllMonitors()[0]->getViewport());
        else
            assert(getRealGeometry(getAllWindows()[0]->getID()) !=  getRealGeometry(getAllWindows()[1]->getID()));
        ATOMIC(getAllWindows()[0]->moveToWorkspace(0));
        waitUntilIdle();
        if(_i)
            sendChangeWorkspaceRequest(0);
        else {
            movePointerRelative(1, 1);
            switchToWorkspace(0);
        }
        waitUntilIdle();
        assertEquals(getActiveWorkspaceIndex(), 0);
    }
});

MPX_TEST("detect_many_masters", {
    for(int i = getAllMasters().size(); i < getMaxNumberOfMasterDevices(); i++)
        createMasterDevice("test");
    waitUntilIdle();
    assertEquals(getAllMasters().size(), getMaxNumberOfMasterDevices());
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
    unlock();
    for(int i = 0; i < getNumberOfWorkspaces(); i++)
        mapArbitraryWindow();
    waitUntilIdle();
    assertEquals(getActiveMaster()->getWorkspace()->getWindowStack().size(), getAllWindows().size());
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
            switchToWorkspace(w->getID());
            waitUntilIdle(1);
        }
});

static void  userEnvSetup() {
    multiMonitorSetup();
    WindowID win = createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DOCK);
    setDockProperties(win, DOCK_TOP, 100);
    for(int i = 0; i < 4; i++) {
        WindowID win2 = mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DOCK));
        setDockProperties(win2, i, 100);
    }
    createOverrideRedirectWindow();
    waitUntilIdle();
    getWindowInfo(win)->addMask(STICKY_MASK | ALWAYS_ON_BOTTOM_MASK);
    mapWindow(win);
    mapArbitraryWindow();
    switchToWorkspace(0);
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
MPX_TEST("focus_unfocusable_window", {
    getEventRules(CLIENT_MAP_ALLOW).add({toggleMask, INPUT_MASK, "toggleMask"});
    WindowID win = mapArbitraryWindow();
    waitUntilIdle();
    assert(!getWindowInfo(win)->isFocusAllowed());
    assertEquals(0, getRealGeometry(win).border);
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
