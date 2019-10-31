#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>

#include "../devices.h"
#include "../wm-rules.h"
#include "../logger.h"
#include "../extra-rules.h"
#include "../wmfunctions.h"
#include "../window-properties.h"
#include "../functions.h"
#include "../ewmh.h"
#include "../chain.h"
#include "../globals.h"
#include "../layouts.h"
#include "../xsession.h"


#include "tester.h"
#include "test-event-helper.h"


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
    WindowID win = createNormalWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DOCK);
    scan(root);
    unlock();
    waitUntilIdle();
    mapWindow(win);
    waitUntilIdle();
    WindowInfo* winInfo = getWindowInfo(win);
    assert(winInfo);
    assert(winInfo->isDock());
});

MPX_TEST("test_monitor_detection", {
    POLL_COUNT = 0;
    MONITOR_DUPLICATION_POLICY = 0;
    lock();
    for(int i = 0; i < getNumberOfWorkspaces(); i++)
        createFakeMonitor({100, 100, 100, 100});
    unlock();
    waitUntilIdle();
    assert(getNumberOfWorkspaces() + 1 == getAllMonitors().size());
    for(Workspace* w : getAllWorkspaces())
        assert(w->isVisible());
    lock();
    swapMonitors(0, 1);
    clearFakeMonitors();
    unlock();
    waitUntilIdle();
    for(Workspace* w : getAllWorkspaces())
        assert(w->isVisible() == (w->getID() == 1));
});
MPX_TEST("default_active_layout", {

    for(Workspace* w : getAllWorkspaces())
        assert(w->getActiveLayout());
});
MPX_TEST("click_to_focus", {
    setActiveLayout(GRID);
    WindowID wins[] = {mapArbitraryWindow(), mapArbitraryWindow()};
    waitUntilIdle();
    for(WindowID win : wins) {
        movePointer(1, 1, getActiveMasterPointerID(), win);
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
    getWindowInfo(wins[0])->addMask(ALWAYS_ON_BOTTOM | ALWAYS_ON_TOP);
    getWindowInfo(wins[1])->addMask(ALWAYS_ON_BOTTOM | ABOVE_MASK);
    getWindowInfo(wins[3])->addMask(ALWAYS_ON_TOP | BELOW_MASK);
    for(WindowID win : wins) {
        assert(lowerWindowInfo(getWindowInfo(win)));
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
    createFakeMonitor({100, 100, 100, 100});
    createFakeMonitor({100, 100, 100, 100});
    detectMonitors();
    assertEquals(getAllMonitors().size(), 3);
    getActiveMaster()->setWorkspaceIndex(3);
    unlock();
    for(int i = 0; i < 10; i++)
        setUserTime(mapArbitraryWindow(), i + 1);
    waitUntilIdle();
}
SET_ENV(multiMonitorSetup, fullCleanup);
MPX_TEST("defaultWorkspace", {
    assertEquals(getActiveMaster()->getWorkspace()->getWindowStack().size(), getAllWindows().size());
});
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
    clearFakeMonitors();
});
static void bindingsSetup() {
    ROOT_DEVICE_EVENT_MASKS = 0;
    setup();
}
SET_ENV(bindingsSetup, fullCleanup);
MPX_TEST("cycle_window", {
    mapArbitraryWindow();
    waitUntilIdle();
    sendKeyPress(getKeyCode(XK_Alt_L));
    sendKeyPress(getKeyCode(XK_Tab));
    waitUntilIdle();
    assert(getActiveChain());
    sendKeyRelease(getKeyCode(XK_Tab));
    sendKeyRelease(getKeyCode(XK_Alt_L));
    waitUntilIdle();
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
    movePointer(1, 1, getActiveMasterPointerID(), win);
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
