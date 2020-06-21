#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>

#include "../../Extensions/containers.h"
#include "../../Extensions/session.h"
#include "../../extra-rules.h"
#include "../../functions.h"
#include "../../globals.h"
#include "../../layouts.h"
#include "../../logger.h"
#include "../../test-functions.h"
#include "../../window-properties.h"
#include "../../wmfunctions.h"
#include "../tester.h"
#include "../test-mpx-helper.h"
#include "../test-event-helper.h"
#include "../test-x-helper.h"

SET_ENV(createXSimpleEnv, fullCleanup);
MPX_TEST("create_container", {
    auto id = createContainer();
    assert(getWindowInfo(id));
    assert(getAllMonitors().find(id));
});


MPX_TEST("ignore", {
    getEventRules(PRE_REGISTER_WINDOW).add(DEFAULT_EVENT([]{return false;}));
    auto id = createContainer();
    assert(!getWindowInfo(id));

});

WindowID container;
Monitor* containerMonitor;
WindowInfo* containerWindowInfo;
WindowID normalWindow;
WindowID containedWindow;


static void addCommonRules() {
    addResumeContainerRules();
    addAutoTileRules();
    addEWMHRules();
    addAutoFocusRule();
}

static void setup() {
    DEFAULT_BORDER_WIDTH = 0;
    CRASH_ON_ERRORS = 0;
    addCommonRules();
    onSimpleStartup();
    container = createContainer();
    setActiveLayout(GRID);
    normalWindow = mapArbitraryWindow();
    containedWindow = mapArbitraryWindow();
    startWM();
    waitUntilIdle();
    containerMonitor = getMonitorForContainer(container);
    containerWindowInfo = getWindowInfoForContainer(container);
    assert(containerWindowInfo);
    assert(containerMonitor);
    assert(containerMonitor->getWorkspace());
    assertEquals(containerMonitor->getBase(), getRealGeometry(containerWindowInfo));
    getWindowInfo(containedWindow)->moveToWorkspace(containerMonitor->getWorkspace()->getID());
    waitUntilIdle();
}
SET_ENV(setup, fullCleanup);

MPX_TEST("tile", {
    assertEquals(getRealGeometry(container).getArea(), getRealGeometry(normalWindow).getArea());
    assertEquals(getRealGeometry(container).getArea(), containerMonitor->getBase().getArea());
    assertEquals(getRealGeometry(container).getArea(), getAllMonitors()[0]->getBase().getArea() / 2);
});

MPX_TEST("toggle_focus", {
    focusWindow(containerWindowInfo);
    waitUntilIdle(1);
    assertEquals(getActiveFocus(), container);
    assertEquals(getFocusedWindow(), containerWindowInfo);
    toggleContainer(getFocusedWindow());
    waitUntilIdle();
    assertEquals(getActiveFocus(), containedWindow);
    toggleContainer(getFocusedWindow());
    waitUntilIdle();
    assertEquals(getActiveFocus(), container);
});

MPX_TEST_ITER("release_container", 2, {
    lock();
    if(_i)
        switchToWorkspace(containerMonitor->getWorkspace()->getID());
    releaseWindows(containerMonitor);
    unlock();
    waitUntilIdle();
    assert(!getWindowInfo(container));
    assert(!getAllMonitors().find(container));
});

MPX_TEST("close", {
    destroyWindow(container);
    waitUntilIdle();
    assert(!getWindowInfo(container));
    assert(!getAllMonitors().find(container));
});

MPX_TEST("create_container_id_taken", {
    for(int i = 0; i < 10; i++) {
        WindowID win = createWindow();
        lock();
        getAllMonitors().add(new Monitor(win + 1, {0, 0, 10, 10}, 0, "", 1));
        WindowID container = createContainer();
        unlock();
        validate();
        if(container == win + 2)
            return;
    }
    assert(0);
});
MPX_TEST("nested_windows", {
    assert(getRealGeometry(containerWindowInfo).contains(getRealGeometry(containedWindow)));
});

MPX_TEST_ITER("stacking_order", 2, {
    assert(containerMonitor->getStackingWindow());
    WindowID stack[] = {container, containedWindow};
    focusWindow(containedWindow);
    waitUntilIdle();
    lock();
    if(_i) {
        setActiveLayout(FULL);
        raiseWindow(getWindowInfo(container));
        tileWorkspace(containerMonitor->getWorkspace());
    }
    else {
        activateWindow(getWindowInfo(container));
    }
    unlock();
    waitUntilIdle();
    assert(checkStackingOrder(stack, LEN(stack), 1));
});

MPX_TEST("mapWindowInsideContainer", {
    switchToWorkspace(containerMonitor->getWorkspace()->getID());
    waitUntilIdle(1);
    assert(getWindowInfo(containedWindow)->hasMask(MAPPED_MASK));
    assert(getWindowInfo(normalWindow)->hasMask(MAPPED_MASK));
    assert(containerWindowInfo->hasMask(MAPPED_MASK));
    switchToWorkspace(0);
    waitUntilIdle(1);
    assert(getWindowInfo(containedWindow)->hasMask(MAPPED_MASK));
    assert(getWindowInfo(normalWindow)->hasMask(MAPPED_MASK));
    assert(containerWindowInfo->hasMask(MAPPED_MASK));
});

MPX_TEST_ITER("contain_self", 3, {
    containerWindowInfo->moveToWorkspace(containerMonitor->getWorkspace()->getID());
    wakeupWM();
    waitUntilIdle();
    assert(getWindowInfo(normalWindow)->hasMask(MAPPED_MASK));
    assertEquals(containerWindowInfo->getWorkspace(), containerMonitor->getWorkspace());
    assertEquals(containerWindowInfo->getWorkspace()->getWindowStack().size(), 2);
    if(_i == 1) {
        ATOMIC(switchToWorkspace(3));
        ATOMIC(switchToWorkspace(containerMonitor->getWorkspace()->getID()));
        wakeupWM();
        waitUntilIdle();
    }
    else if(_i) {
        focusWindow(normalWindow);
        raiseWindow(containerWindowInfo);
        waitUntilIdle();
    }
});

MPX_TEST("unmapContainer", {
    assert(!getWorkspace(3)->isVisible());
    ATOMIC(switchToWorkspace(3));
    wakeupWM();
    waitUntilIdle();
    assert(!getWindowInfo(normalWindow)->hasMask(MAPPED_MASK));
    assert(!containerWindowInfo->hasMask(MAPPED_MASK));
    assert(!getWindowInfo(containedWindow)->hasMask(MAPPED_MASK));
});

MPX_TEST("persist_after_monitor_refresh", {
    detectMonitors();
    assert(getAllMonitors().find(container));
});

MPX_TEST("destroy_containers", {
    assertEquals(2, getAllMonitors().size());
    destroyWindow(container);
    waitUntilIdle();
    assertEquals(1, getAllMonitors().size());
});

void loadContainers();
MPX_TEST("resume_containers", {
    auto size = getAllMonitors().size();
    auto monitorID = *containerMonitor;
    auto workspaceIndex = containerWindowInfo->getWorkspaceIndex();
    Workspace* containedWorkspace = containerMonitor->getWorkspace();
    lock();
    destroyWindow(container);
    delete getAllWindows().removeElement(container);
    switchToWorkspace(5);
    getAllMonitors().add(new Monitor(monitorID, {0, 0, 1, 1}, 0, "", 1));
    getAllMonitors()[1]->assignWorkspace(containedWorkspace);
    loadContainers();
    assertEquals(size, getAllMonitors().size());
    containerMonitor = getAllMonitors()[1];
    assert(containerMonitor->isFake());
    assertEquals(*containedWorkspace, *containerMonitor->getWorkspace());
    assert(getWindowInfoForContainer(*containerMonitor));
    assertEquals(containerMonitor, getMonitorForContainer(*getWindowInfoForContainer(*containerMonitor)));
    assertEquals(workspaceIndex, getWindowInfoForContainer(*containerMonitor)->getWorkspaceIndex());
    unlock();
});

static void bare_setup() {
    addCommonRules();
    onSimpleStartup();
    setActiveLayout(GRID);
    startWM();
    waitUntilIdle();
}
SET_ENV(bare_setup, fullCleanup);

static Monitor* containerWindowHelper(std::string containedClass, Workspace* containedWorkspace, int n = 1) {
    lock();
    for(int i = 0; i < n; i++) {
        containedWindow = mapArbitraryWindow();
        setWindowClass(containedWindow, containedClass, containedClass);
    }
    unlock();
    waitUntilIdle();
    lock();
    containerMonitor = containWindows(containedWorkspace, {matchesClass, containedClass, "test", PASSTHROUGH_IF_TRUE });
    assert(containerMonitor);
    assert(containerMonitor->getWorkspace());
    assert(containerMonitor->getWorkspace() != getActiveWorkspace());
    assertEquals(containerMonitor->getWorkspace(), getWindowInfo(containedWindow)->getWorkspace());
    assertEquals(containerMonitor->getWorkspace()->getWindowStack().size(), n);
    unlock();
    waitUntilIdle();
    return containerMonitor;
}
MPX_TEST_ITER("contain_windows", 2, {
    normalWindow = mapArbitraryWindow();
    containerWindowHelper("test", getWorkspace(1));
    containerWindowHelper("test2", getWorkspace(2), 2);
    assertEquals(getActiveWorkspace(), getWindowInfo(normalWindow)->getWorkspace());
    assertEquals(getActiveWorkspace()->getWindowStack().size(), 3);
    assertEquals(getAllMonitors().size(), 3);

    lock();
    if(_i) {
        releaseWindows(getWorkspace(1)->getMonitor());
        releaseWindows(getWorkspace(2)->getMonitor());
    }
    else {
        releaseAllWindows();
    }
    unlock();
    waitUntilIdle();

    assertEquals(getActiveWorkspace()->getWindowStack().size(), getAllWindows().size());
    assertEquals(getAllMonitors().size(), 1);
    assertEquals(getActiveWorkspace(), getWindowInfo(normalWindow)->getWorkspace());

});

MPX_TEST("contain_window_named", {
    lock();
    assert(containWindows(getWorkspace(1), {}, "test"));
    assertEquals(getAllMonitors().size(), 2);
    assert(containWindows(getWorkspace(2), {}, "test"));
    assertEquals(getAllMonitors().size(), 2);
    unlock();
});
MPX_TEST("contain_window_failed_registration", {
    getEventRules(PRE_REGISTER_WINDOW).add(PASSTHROUGH_EVENT(incrementCount, NO_PASSTHROUGH));
    lock();
    Monitor* monitor = containWindows(getWorkspace(1), {}, "test");
    assert(!monitor);
    unlock();
});

MPX_TEST("contain_window_named_taken", {
    lock();
    addFakeMonitor({0, 0, 1, 1}, "test");
    Monitor* monitor = containWindows(getWorkspace(1), {}, "test");
    WindowInfo* winInfo = getWindowInfoForContainer(*monitor);
    assert(!winInfo);
    unlock();
});

MPX_TEST("contain_windows_to_self", {
    mapArbitraryWindow();
    assert(!containWindows(getActiveWorkspace(), {}));
});

MPX_TEST("many_containers", {
    for(int i = 0; i < DEFAULT_NUMBER_OF_WORKSPACES; i++) {
        lock();
        createContainer();
        retile();
        unlock();
        validate();
    }
});
