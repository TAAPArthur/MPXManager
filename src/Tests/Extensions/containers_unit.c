#include <X11/Xlib-xcb.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>

#include "../../Extensions/containers.h"
#include "../../Extensions/session.h"
#include "../../functions.h"
#include "../../globals.h"
#include "../../layouts.h"
#include "../../wmfunctions.h"
#include "../test-event-helper.h"
#include "../test-mpx-helper.h"
#include "../test-wm-helper.h"
#include "../test-x-helper.h"
#include "../tester.h"

SCUTEST_SET_ENV(onDefaultStartup, cleanupXServer, .timeout = 2);
SCUTEST(test_create_container) {
    WindowID win = createNormalWindow();
    assert(getAllMonitors()->size == 1);
    scan(root);
    for(int i = 0; i < 2; i++) {
        assert(!i == createContainer(getWindowInfo(win)));
        assert(getMonitorForContainer(win));
        assert(getAllMonitors()->size == 2);
    }
}

SCUTEST(test_create_container_in_workspace) {
    WindowID win = mapWindow(createNormalWindow());
    WindowID win2 = mapWindow(createNormalWindow());
    scan(root);
    moveToWorkspace(getWindowInfo(win2), 2);
    createContainerInWorkspace(getWindowInfo(win), getWorkspace(2));
    assert(isWorkspaceVisible(getWorkspace(0)));
    assert(isWorkspaceVisible(getWorkspace(2)));
}

static WindowID container;
static Monitor* containerMonitor;
static WindowInfo* containerWindowInfo;
static WindowID normalWindow;
static WindowID containedWindow;

static void setup() {
    MONITOR_DUPLICATION_POLICY |= CONSIDER_ONLY_NONFAKES;
    DEFAULT_BORDER_WIDTH = 0;
    CRASH_ON_ERRORS = 0;
    addContainerRules();
    onDefaultStartup();

    container = mapArbitraryWindow();
    normalWindow = mapArbitraryWindow();
    containedWindow = mapArbitraryWindow();
    scan(root);

    containerWindowInfo = getWindowInfoForContainer(container);
    createContainer(containerWindowInfo);
    containerMonitor = getMonitorForContainer(container);
    assert(containerWindowInfo);
    assert(containerMonitor);

    setActiveLayout(&GRID);
    setWindowTitle(normalWindow, "Normal");
    setWindowTitle(containedWindow, "ContainedWindow");
    runEventLoop();

    assert(getWorkspaceOfMonitor(containerMonitor));
    moveToWorkspace(getWindowInfo(containedWindow), getWorkspaceOfMonitor(containerMonitor)->id);
    assertEqualsRect(containerMonitor->base, getRealGeometry(containerWindowInfo->id));
    runEventLoop();
}
SCUTEST_SET_ENV(setup, cleanupXServer, .timeout = 2);

SCUTEST(tile) {
    assertEquals(getArea(getRealGeometry(container)), getArea(getRealGeometry(normalWindow)));
    assertEquals(getArea(getRealGeometry(container)), getArea(containerMonitor->base));
    assertEquals(getArea(getRealGeometry(container)), getArea(((Monitor*)getHead(getAllMonitors()))->base) / 2);
}
SCUTEST_ITER(set_geometry, 4) {
    uint16_t size = _i % 2 + 1;
    Rect bounds = {0, 0, size, size};
    setWindowPosition(containerWindowInfo->id, bounds);
    runEventLoop();
    assert(contains(bounds, containerMonitor->base));
}

SCUTEST(toggle_focus) {
    focusWindowInfo(containerWindowInfo);
    runEventLoop();
    assertEquals(getActiveFocus(), container);
    assertEquals(getFocusedWindow(), containerWindowInfo);
    toggleContainer(getFocusedWindow());
    runEventLoop();
    assertEquals(getActiveFocus(), containedWindow);
    toggleContainer(getFocusedWindow());
    runEventLoop();
    assertEquals(getActiveFocus(), container);
}

SCUTEST(test_destroy_container) {
    destroyWindow(container);
    runEventLoop();
    assert(!getWindowInfo(container));
    assert(!getMonitorByID(container));
}

/*
SCUTEST_ITER(stacking_order, 2) {
    assert(containerMonitor->getStackingWindow());
    WindowID stack[] = {container, containedWindow};
    focusWindow(containedWindow);
    waitUntilIdle();

    if(_i) {
        setActiveLayout(FULL);
        raiseWindow(getWindowInfo(container));
        tileWorkspace(containerMonitor->getWorkspace());
    }
    else {
        activateWindow(getWindowInfo(container));
    }

    waitUntilIdle();
    assert(checkStackingOrder(stack, LEN(stack), 1));
}
*/

SCUTEST(mapWindowInsideContainer) {
    switchToWorkspace(getWorkspaceOfMonitor(containerMonitor)->id);
    runEventLoop();
    assert(hasMask(getWindowInfo(containedWindow), MAPPED_MASK));
    assert(hasMask(getWindowInfo(normalWindow), MAPPED_MASK));
    assert(hasMask(containerWindowInfo, MAPPED_MASK));
    switchToWorkspace(0);
    runEventLoop();
    assert(hasMask(getWindowInfo(containedWindow), MAPPED_MASK));
    assert(hasMask(getWindowInfo(normalWindow), MAPPED_MASK));
    assert(hasMask(containerWindowInfo, MAPPED_MASK));
}

SCUTEST_ITER(contain_self, 3) {
    moveToWorkspace(containerWindowInfo, getWorkspaceOfMonitor(containerMonitor)->id);
    runEventLoop();
    assert(hasMask(getWindowInfo(normalWindow), MAPPED_MASK));
    if(_i == 1) {
        switchToWorkspace(3);
        switchToWorkspace(getWorkspaceOfMonitor(containerMonitor)->id);
    }
    else if(_i) {
        focusWindow(normalWindow);
        raiseWindowInfo(containerWindowInfo, 0);
    }
    runEventLoop();
}

SCUTEST(unmapContainer) {
    switchToWorkspace(3);
    runEventLoop();
    assert(!hasMask(getWindowInfo(normalWindow), MAPPED_MASK));
    assert(!hasMask(containerWindowInfo, MAPPED_MASK));
    assert(!hasMask(getWindowInfo(containedWindow), MAPPED_MASK));
}

SCUTEST(persist_after_monitor_refresh) {
    detectMonitors();
    assert(getMonitorByID(container));
}

SCUTEST(destroy_containers) {
    assertEquals(2, getAllMonitors()->size);
    destroyWindow(container);
    runEventLoop();
    assertEquals(1, getAllMonitors()->size);
}

/*
void loadContainers();
SCUTEST(resume_containers) {
    int size = getAllMonitors()->size;
    WorkspaceID workspaceIndex = getWorkspaceIndexOfWindow(containerWindowInfo);
    Workspace* containedWorkspace = getWorkspaceOfMonitor(containerMonitor);
    destroyWindow(container);
    runEventLoop();
    switchToWorkspace(5);
    assignWorkspace(addDummyMonitor(), containedWorkspace);
    loadContainers();
    runEventLoop();
    assertEquals(size, getAllMonitors()->size);
    containerMonitor = getMonitor(containedWorkspace);
    assert(containerMonitor->fake);
    assert(getWindowInfoForContainer(containerMonitor->id));
    assertEquals(containerMonitor, getMonitorForContainer(getWindowInfoForContainer(containerMonitor->id)->id));
    assertEquals(workspaceIndex, getWorkspaceIndexOfWindow(getWindowInfoForContainer(containerMonitor->id)));
}
*/

/*
static void bare_setup() {
    addCommonRules();
    onSimpleStartup();
    setActiveLayout(&GRID);
    startWM();
    waitUntilIdle();
}
SCUTEST_SET_ENV(bare_setup, fullCleanup);

static Monitor* containerWindowHelper(const char* containedClass, Workspace* containedWorkspace, int n) {
    for(int i = 0; i < n; i++) {
        containedWindow = mapArbitraryWindow();
        setWindowClass(containedWindow, containedClass, containedClass);
    }
    runEventLoop();
    containerMonitor = containWindows(containedWorkspace, {matchesClass, containedClass, "test", PASSTHROUGH_IF_TRUE }
            assert(containerMonitor);
            assert(containerMonitor->getWorkspace());
            assert(containerMonitor->getWorkspace() != getActiveWorkspace());
            assertEquals(containerMonitor->getWorkspace(), getWindowInfo(containedWindow)->getWorkspace());
            assertEquals(containerMonitor->getWorkspace()->getWindowStack()->size, n);
            runEventLoop();
            return containerMonitor;
}
SCUTEST_ITER(contain_windows, 2) {
    normalWindow = mapArbitraryWindow();
    containerWindowHelper("test", getWorkspace(1), 1);
    containerWindowHelper("test2", getWorkspace(2), 2);
    assertEquals(getActiveWorkspace(), getWindowInfo(normalWindow)->getWorkspace());
    assertEquals(getActiveWorkspace()->getWindowStack()->size, 3);
    assertEquals(getAllMonitors()->size, 3);
    if(_i) {
        releaseWindows(getWorkspace(1)->getMonitor());
        releaseWindows(getWorkspace(2)->getMonitor());
    }
    else {
        releaseAllWindows();
    }
    runEventLoop();
    assertEquals(getActiveWorkspace()->getWindowStack()->size, getAllWindows()->size);
    assertEquals(getAllMonitors()->size, 1);
    assertEquals(getActiveWorkspace(), getWindowInfo(normalWindow)->getWorkspace());
}
*/

/*
SCUTEST(contain_windows_raise_on_tile) {

    addAutoFocusRule();
    normalWindow = mapArbitraryWindow();
    Workspace* containedWorkspace = getWorkspace(1);
    containedWorkspace->setActiveLayout(&GRID);
    containerMonitor = containerWindowHelper("test", containedWorkspace, 2);

    assertEquals(getActiveWorkspace(), getWindowInfo(normalWindow)->getWorkspace());
    assertEquals(getActiveWorkspace()->getWindowStack()->size, 2);
    assertEquals(containedWorkspace->getWindowStack()->size, 2);
    assertEquals(getAllMonitors()->size, 2);
    setActiveLayout(FULL);
    ATOMIC(activateWindow(getWindowInfo(normalWindow)));
    waitUntilIdle();

    ATOMIC(activateWindow(getWindowInfoForContainer(*containerMonitor)));
    waitUntilIdle();
    for(WindowInfo* winInfo : containedWorkspace->getWindowStack()) {
        WindowID stack[] = {*getWindowInfoForContainer(*containerMonitor), *winInfo};
        assert(checkStackingOrder(stack, LEN(stack)));
    }
}
*/
