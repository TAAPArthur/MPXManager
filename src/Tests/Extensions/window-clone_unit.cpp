#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>

#include "../../logger.h"
#include "../../globals.h"
#include "../../wmfunctions.h"
#include "../../window-properties.h"
#include "../../layouts.h"
#include "../../Extensions/window-clone.h"
#include "../../test-functions.h"
#include "../../state.h"
#include "../../extra-rules.h"
#include "../tester.h"
#include "../test-mpx-helper.h"
#include "../test-event-helper.h"
#include "../test-x-helper.h"

static WindowInfo* winInfo;
static WindowInfo* cloneInfo;
static std::string defaultTitle = "test";
static std::string newTitle = "test2";

static void setup() {
    POLL_COUNT = 1;
    CRASH_ON_ERRORS = -1;
    onSimpleStartup();
    addCloneRules();
    addAutoTileRules();
    addEWMHRules();
    switchToWorkspace(0);
    setActiveLayout(GRID);
    WindowID win = mapArbitraryWindow();
    setWindowTitle(win, defaultTitle);
    scan(root);
    winInfo = getAllWindows()[0];
    assertEquals(winInfo->getTitle(), defaultTitle);
    startWM();
    waitUntilIdle();
    ATOMIC(cloneInfo = cloneWindow(winInfo));
    assert(cloneInfo->getWorkspace());
    waitUntilIdle();
}
SET_ENV(setup, fullCleanup);
MPX_TEST("test_clone_window", {
    assert(getWindowInfo(cloneInfo->getID()) == cloneInfo);
    assert(getAllWindows().size() == 2);
    assertEquals(cloneInfo->getTitle(), defaultTitle);
});
MPX_TEST("test_sync_properties", {
    ATOMIC(setWindowTitle(winInfo->getID(), newTitle));
    waitUntilIdle();
    assertEquals(winInfo->getTitle(), newTitle);
    ATOMIC(updateAllClonesOfWindow(winInfo));
    waitUntilIdle();
    assertEquals(cloneInfo->getTitle(), newTitle);
});
MPX_TEST("test_seemless_swap_with_original", {
    lock();
    winInfo->moveToWorkspace(0);
    cloneInfo->moveToWorkspace(1);

    updateState();
    swapWindows(winInfo, cloneInfo);

    assert(!updateState());
    unlock();
});

MPX_TEST("test_update_clone", {
    updateAllClonesOfWindow(winInfo);
    //TODO actually test somthing
});
MPX_TEST_ITER("test_simple_kill_clone", 2, {
    if(_i)
        updateAllClones();
    ATOMIC(killAllClones(winInfo));
    flush();
    WAIT_UNTIL_TRUE(getAllWindows().size() == 1);
    updateAllClones();
});

MPX_TEST_ITER("clone_special_windows", 2, {
    addIgnoreOverrideRedirectWindowsRule(1);

    WindowID overrideRedirectWindow = createOverrideRedirectWindow(XCB_WINDOW_CLASS_INPUT_OUTPUT);
    waitUntilIdle();
    lock();
    if(_i == 1) {
        addIgnoreOverrideRedirectWindowsRule();
        assert(cloneWindow(getWindowInfo(overrideRedirectWindow)) == NULL);
    }
    else if(_i == 2)
        assert(cloneWindow(getWindowInfo(overrideRedirectWindow))->isOverrideRedirectWindow());
    unlock();
});

MPX_TEST("test_auto_update_clones", {
    startAutoUpdatingClones();
    msleep(100);
    // TODO actually test something
    requestShutdown();
    createNormalWindow();
});


MPX_TEST("focus_clone", {
    focusWindow(cloneInfo);
    waitUntilIdle();
    assertEquals(getFocusedWindow(), winInfo);
});
MPX_TEST("mouse_enter", {
    movePointer(1, 1, winInfo->getID());
    movePointer(1, 1, cloneInfo->getID());
    waitUntilIdle();
    short pos[2];
    getMousePosition(pos);
    Rect mouse = {pos[0], pos[1], 1, 1};
    assert(getRealGeometry(winInfo).contains(mouse));
    assert(!getRealGeometry(cloneInfo).contains(mouse));
});
MPX_TEST("swap_on_unmap", {
    assertEquals(winInfo->getWorkspaceIndex(), 0);
    winInfo->moveToWorkspace(1);
    waitUntilIdle(1);
    assertEquals(winInfo->getWorkspaceIndex(), 0);
    assertEquals(cloneInfo->getWorkspaceIndex(), 1);
    ATOMIC(switchToWorkspace(1));
    waitUntilIdle(1);
    assertEquals(winInfo->getWorkspaceIndex(), 1);
    assertEquals(cloneInfo->getWorkspaceIndex(), 0);
    ATOMIC(switchToWorkspace(2));
    waitUntilIdle(1);
    assert(!winInfo->hasMask(MAPPED_MASK));
    assert(!cloneInfo->hasMask(MAPPED_MASK));
    ATOMIC(switchToWorkspace(0));
    waitUntilIdle(1);
    assertEquals(winInfo->getWorkspaceIndex(), 0);
    assertEquals(cloneInfo->getWorkspaceIndex(), 1);
});
