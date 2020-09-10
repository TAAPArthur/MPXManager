#include "../../Extensions/compatibility-rules.h"
#include "../../windows.h"
#include "../tester.h"
#include "../test-mpx-helper.h"
#include "../test-event-helper.h"
#include "../test-x-helper.h"

static void setup() {
    onSimpleStartup();
    addAutoDetectDockPosition();
    startWM();
    waitUntilIdle();
}
SET_ENV(setup, fullCleanup);
MPX_TEST_ITER("create_dock", 4 * 2, {
    Monitor* monitor = getAllMonitors()[0];
    DockType type = (DockType)(_i % 4);
    bool dockHor = type % 4 > 1;
    bool full = type / 4 ;
    const int thickness = 10;

    Rect geo = {0, 0, 0, 0};
    geo[dockHor + 2] = thickness;
    geo[!dockHor] = 1;
    geo[!dockHor + 2 ] = monitor->getBase()[!dockHor + 2] / 2;
    if(type == DOCK_BOTTOM || type == DOCK_RIGHT) {
        geo[dockHor] = monitor->getBase()[dockHor + 2] - thickness;
    }
    int start = full ? 0 : geo[!dockHor];
    int end = full ? 0 : geo[!dockHor] + geo[!dockHor + 2];

    lock();
    WindowID win = createWindow(root, XCB_WINDOW_CLASS_INPUT_OUTPUT, 0, NULL, geo);

    catchError(xcb_ewmh_set_wm_window_type_checked(ewmh, win, 1, &ewmh->_NET_WM_WINDOW_TYPE_DOCK));
    mapWindow(win);
    unlock();
    waitUntilIdle();
    WindowInfo* winInfo = getWindowInfo(win);
    assert(winInfo);
    assert(winInfo->isDock());
    auto dockProperties= winInfo->getDockProperties();
    assert(dockProperties);
    assertEquals(dockProperties.type, type);
    assertEquals(dockProperties.start, start);
    assertEquals(dockProperties.end, end);
    assert(monitor->getBase() != monitor->getViewport());
});
