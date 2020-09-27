#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>

#include "../../wmfunctions.h"
#include "../../layouts.h"
#include "../../Extensions/window-clone.h"
#include "../tester.h"
#include "../test-mpx-helper.h"
#include "../test-event-helper.h"
#include "../test-x-helper.h"
#include "../test-wm-helper.h"

static WindowInfo* winInfo;
static WindowInfo* cloneInfo;

static void setup() {
    POLL_COUNT = 1;
    CRASH_ON_ERRORS = -1;
    addCloneRules();
    onDefaultStartup();
    setActiveLayout(&GRID);
    WindowID win = mapWindow(createNormalWindow());
    setWindowTitle(win, "title");
    assert(isWindowMapped(win));
    scan(root);
    assert(isWindowMapped(win));
    winInfo = getWindowInfo(win);
    assert(hasMask(winInfo, MAPPED_MASK));
    cloneInfo = cloneWindow(winInfo);
    assert(hasMask(winInfo, MAPPED_MASK));
    assert(hasMask(cloneInfo, MAPPED_MASK));
    runEventLoop();
}
SCUTEST_SET_ENV(setup, cleanupXServer);
SCUTEST(test_clone_window) {
    assert(getWindowInfo(cloneInfo->id) == cloneInfo);
    assert(getAllWindows()->size == 2);
    assert(strcmp(cloneInfo->title, winInfo->title) == 0);
}
SCUTEST(test_sync_properties, .iter = 2) {
    setWindowTitle(winInfo->id, "newTitle");
    if(_i)
        updateAllClonesOfWindow(winInfo);
    else
        updateAllClones();
    assert(strcmp(cloneInfo->title, winInfo->title) == 0);
}
SCUTEST_ITER(test_simple_kill_clone, 2) {
    if(_i)
        updateAllClones();
    killAllClones(winInfo);
    flush();
    runEventLoop();
    assert(getAllWindows()->size == 1);
}

SCUTEST(focus_clone) {
    focusWindowInfo(cloneInfo, getActiveMaster());
    runEventLoop();
    assertEquals(getFocusedWindow(), winInfo);
}
SCUTEST(mouse_enter) {
    warpPointer(1, 1, winInfo->id, getActiveMasterPointerID());
    warpPointer(1, 1, cloneInfo->id, getActiveMasterPointerID());
    runEventLoop();
    short pos[2];
    getMousePosition(getActiveMasterPointerID(), root, pos) ;
    Rect mouse = {pos[0], pos[1], 1, 1};
    assert(contains(getRealGeometry(winInfo->id), mouse));
    assert(!contains(getRealGeometry(cloneInfo->id), mouse));
}
SCUTEST(swap_on_unmap) {
    POLL_COUNT = 3;
    moveToWorkspace(winInfo, 1);
    runEventLoop();
    assertEquals(getWorkspaceIndexOfWindow(winInfo), 0);
    assertEquals(getWorkspaceIndexOfWindow(cloneInfo), 1);
    assert(!isWindowMapped(cloneInfo->id));
    switchToWorkspace(1);
    runEventLoop();
    assertEquals(getWorkspaceIndexOfWindow(winInfo), 1);
    assertEquals(getWorkspaceIndexOfWindow(cloneInfo), 0);
    assert(!isWindowMapped(cloneInfo->id));
    switchToWorkspace(2);
    runEventLoop();
    assertEquals(getWorkspaceIndexOfWindow(winInfo), 1);
    assertEquals(getWorkspaceIndexOfWindow(cloneInfo), 0);
    assert(!isWindowMapped(cloneInfo->id));
    switchToWorkspace(0);
    runEventLoop();
    assertEquals(getWorkspaceIndexOfWindow(winInfo), 0);
    assertEquals(getWorkspaceIndexOfWindow(cloneInfo), 1);
    assert(!isWindowMapped(cloneInfo->id));
}
SCUTEST(delete_parent_before_clone, .iter = 2) {
    CRASH_ON_ERRORS = 0;
    WindowID clone = cloneInfo->id;
    if(_i)
        for(int i = 0; i < 10; i++)
            cloneWindow(winInfo);
    destroyWindow(winInfo->id);
    runEventLoop();
    assert(!getWindowInfo(clone));
}

