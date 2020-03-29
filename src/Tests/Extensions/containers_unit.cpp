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
#include "../../Extensions/containers.h"
#include "../../test-functions.h"
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

static void setup() {
    DEFAULT_BORDER_WIDTH = 0;
    onSimpleStartup();
    addAutoTileRules();
    addEWMHRules();
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

MPX_TEST("close", {
    destroyWindow(container);
    waitUntilIdle();
    assert(!getWindowInfo(container));
    assert(!getAllMonitors().find(container));
});

MPX_TEST("nested_windows", {
    assert(getRealGeometry(containerWindowInfo).contains(getRealGeometry(containedWindow)));
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

MPX_TEST_ITER("contain_self", 2, {
    containerWindowInfo->moveToWorkspace(containerMonitor->getWorkspace()->getID());
    waitUntilIdle();
    assert(getWindowInfo(normalWindow)->hasMask(MAPPED_MASK));
    if(_i) {
        ATOMIC(switchToWorkspace(3));
        ATOMIC(switchToWorkspace(containerMonitor->getWorkspace()->getID()));
        waitUntilIdle();
    }
});

MPX_TEST("unmapContainer", {
    assert(!getWorkspace(3)->isVisible());
    ATOMIC(switchToWorkspace(3));
    waitUntilIdle();
    assert(!getWindowInfo(normalWindow)->hasMask(MAPPED_MASK));
    assert(!containerWindowInfo->hasMask(MAPPED_MASK));
    assert(!getWindowInfo(containedWindow)->hasMask(MAPPED_MASK));
});

MPX_TEST("persist_after_monitor_refresh", {
    detectMonitors();
    assert(getAllMonitors().find(container));
});
MPX_TEST("resume_containers", {
    auto size = getAllMonitors().size();
    auto index = getAllMonitors().indexOf(container);
    addResumeContainerRules();
    destroyWindow(container);
    waitUntilIdle();
    assertEquals(size, getAllMonitors().size());
    ATOMIC(applyEventRules(X_CONNECTION));
    waitUntilIdle();
    assertEquals(size, getAllMonitors().size());
    containerMonitor = getAllMonitors()[index];
    assert(containerMonitor->isFake());
    assert(getWindowInfoForContainer(*containerMonitor));
    assertEquals(containerMonitor, getMonitorForContainer(*getWindowInfoForContainer(*containerMonitor)));
});
