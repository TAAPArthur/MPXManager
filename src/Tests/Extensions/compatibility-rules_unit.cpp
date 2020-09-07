#include "../../Extensions/compatibility-rules.h"
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
    // {DOCK_LEFT, DOCK_RIGHT, DOCK_TOP, DOCK_BOTTOM} ;
    DockTypes type = (DockTypes)(_i % 4);
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
    int properties[12] = {0};
    properties[type] = thickness;
    properties[4 + type * 2] = full ? 0 : geo[!dockHor];
    properties[4 + type * 2 + 1] = full ? 0 : geo[!dockHor] + geo[!dockHor + 2];

    lock();
    WindowID win = createWindow(root, XCB_WINDOW_CLASS_INPUT_OUTPUT, 0, NULL, geo);

    catchError(xcb_ewmh_set_wm_window_type_checked(ewmh, win, 1, &ewmh->_NET_WM_WINDOW_TYPE_DOCK));
    mapWindow(win);
    unlock();
    waitUntilIdle();
    WindowInfo* winInfo = getWindowInfo(win);
    assert(winInfo);
    assert(winInfo->isDock());
    int* p = winInfo->getDockProperties();
    assert(p);
    assert(memcmp(properties, p, sizeof(properties)) == 0);
    assert(monitor->getBase() != monitor->getViewport());
});
