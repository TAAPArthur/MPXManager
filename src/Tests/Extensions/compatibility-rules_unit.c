#include "../../Extensions/compatibility-rules.h"
#include "../../windows.h"
#include "../test-event-helper.h"
#include "../test-mpx-helper.h"
#include "../test-wm-helper.h"
#include "../test-x-helper.h"
#include "../tester.h"

static void setup() {
    onSimpleStartup();
    addAutoDetectDockPosition();
    addShutdownOnIdleRule();
    runEventLoop();
}
SCUTEST_SET_ENV(setup, cleanupXServer);
SCUTEST_ITER(create_dock, 4 * 2) {
    Monitor* monitor = getHead(getAllMonitors());
    DockType type = (DockType)(_i % 4);
    bool dockHor = type % 4 > 1;
    bool full = type / 4 ;
    const int thickness = 10;
    short geo[] = {0, 0, 0, 0};
    geo[dockHor + 2] = thickness;
    geo[!dockHor] = 1;
    geo[!dockHor + 2 ] = (&monitor->base.x)[!dockHor + 2] / 2;
    if(type == DOCK_BOTTOM || type == DOCK_RIGHT) {
        geo[dockHor] = (&monitor->base.x)[dockHor + 2] - thickness;
    }
    int start = full ? 0 : geo[!dockHor];
    int end = full ? 0 : geo[!dockHor] + geo[!dockHor + 2];
    WindowID win = createWindow(root, XCB_WINDOW_CLASS_INPUT_OUTPUT, 0, NULL, *(Rect*)geo);
    catchError(xcb_ewmh_set_wm_window_type_checked(ewmh, win, 1, &ewmh->_NET_WM_WINDOW_TYPE_DOCK));
    mapWindow(win);
    runEventLoop();
    WindowInfo* winInfo = getWindowInfo(win);
    assert(winInfo);
    assert(hasMask(winInfo, DOCK_MASK));
    const DockProperties* dockProperties = getDockProperties(winInfo);
    assert(dockProperties);
    assertEquals(dockProperties->type, type);
    assertEquals(dockProperties->start, start);
    assertEquals(dockProperties->end, end);
}
