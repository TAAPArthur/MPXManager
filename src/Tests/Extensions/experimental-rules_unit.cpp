#include "../../bindings.h"
#include "../../devices.h"
#include "../../ewmh.h"
#include "../../extra-rules.h"
#include "../../Extensions/experimental-rules.h"
#include "../../globals.h"
#include "../../layouts.h"
#include "../../logger.h"
#include "../../state.h"
#include "../../window-properties.h"
#include "../../wm-rules.h"
#include "../../wmfunctions.h"
#include "../test-event-helper.h"
#include "../tester.h"

SET_ENV(onSimpleStartup, fullCleanup);
MPX_TEST("test_die_on_idle", {
    addDieOnIdleRule();
    createNormalWindow();
    flush();
    atexit(fullCleanup);
    runEventLoop();
});

MPX_TEST("test_float_rule", {
    addFloatRule();
    mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DIALOG));
    mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_NOTIFICATION));
    mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_SPLASH));
    scan(root);
    for(WindowInfo* winInfo : getAllWindows())
        assert(winInfo->hasMask(FLOATING_MASK));
});
MPX_TEST_ITER("test_ignored_windows", 2, {
    addUnknownInputOnlyWindowIgnoreRule();
    if(_i)
        createTypelessInputOnlyWindow();
    else
        mapWindow(createTypelessInputOnlyWindow());
    startWM();
    waitUntilIdle();
    assertEquals(getAllWindows().size(), _i);
});

MPX_TEST("test_focus_follows_mouse", {
    addEWMHRules();
    addFocusFollowsMouseRule();
    setActiveLayout(GRID);
    mapArbitraryWindow();
    mapArbitraryWindow();
    scan(root);
    retile();
    WindowInfo* winInfo = getAllWindows()[0];
    WindowInfo* winInfo2 = getAllWindows()[1];
    WindowID id1 = winInfo->getID();
    WindowID id2 = winInfo2->getID();
    focusWindow(id1);
    movePointer(0, 0, id1);
    flush();
    startWM();
    WAIT_UNTIL_TRUE(getActiveFocus(getActiveMaster()->getID()) == id1);
    for(int i = 0; i < 4; i++) {
        int id = (i % 2 ? id1 : id2);
        int n = 0;
        WAIT_UNTIL_TRUE(getActiveFocus(getActiveMaster()->getID()) == id,
            movePointer(n, n, id);
            n = !n;
            flush()
        );
    }
});

MPX_TEST("test_detect_sub_windows", {
    NON_ROOT_EVENT_MASKS |= XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
    addScanChildrenRule();
    WindowID win = createUnmappedWindow();
    WindowID win2 = createNormalSubWindow(win);
    flush();
    startWM();
    waitUntilIdle();
    WindowID win3 = createNormalSubWindow(win2);
    waitUntilIdle();
    assert(getAllWindows().find(win));
    assert(getAllWindows().find(win2));
    assert(getAllWindows().find(win3));
});
