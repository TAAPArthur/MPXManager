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
    addCloneRules();
    addAutoTileRules();
    onStartup();
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
    markState();
    updateState();
    swapWindows(winInfo, cloneInfo);
    markState();
    assert(updateState() == WINDOW_CHANGE);
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


MPX_TEST("test_auto_update_clones", {
    startAutoUpdatingClones();
    startAutoUpdatingClones();
    startAutoUpdatingClones();
    requestShutdown();
    createNormalWindow();
    waitForAllThreadsToExit();
});


