#include "../layouts.h"
#include "../wmfunctions.h"
#include "../functions.h"
#include "../devices.h"
#include "../masters.h"
#include "../window-properties.h"
#include "tester.h"
#include "test-mpx-helper.h"
#include "test-event-helper.h"
#include "test-x-helper.h"


static WindowInfo* top;
static WindowInfo* middle;
static WindowInfo* bottom;
static void functionSetup() {
    onStartup();
    int size = 3;
    for(int i = 0; i < size; i++)
        mapWindow(createNormalWindow());
    scan(root);
    top = getActiveWindowStack()[0];
    middle = getActiveWindowStack()[1];
    bottom = getActiveWindowStack().back();
    assert(top);
    assert(bottom);
    for(WindowInfo* winInfo : getAllWindows())
        getActiveMaster()->onWindowFocus(winInfo->getID());
    focusWindow(bottom);
}

SET_ENV(functionSetup, fullCleanup);

MPX_TEST("cycle_window", {
    AUTO_FOCUS_NEW_WINDOW_TIMEOUT = -1;
    startWM();
    waitUntilIdle();
    assertEquals(getFocusedWindow(), bottom);
    ATOMIC(cycleWindows(DOWN));
    assert(getActiveMaster()->isFocusStackFrozen());
    assertEquals(getFocusedWindow(), middle);
    ATOMIC(cycleWindows(DOWN));
    assert(getActiveMaster()->isFocusStackFrozen());
    assertEquals(getFocusedWindow(), top);
    ATOMIC(cycleWindows(UP));
    assert(getActiveMaster()->isFocusStackFrozen());
    assertEquals(getFocusedWindow(), middle);
    endCycleWindows();
    assert(!getActiveMaster()->isFocusStackFrozen());
});

void assertWindowIsFocused(WindowID win) {
    xcb_input_xi_get_focus_reply_t* reply = xcb_input_xi_get_focus_reply(dis, xcb_input_xi_get_focus(dis,
                                            getActiveMasterKeyboardID()), NULL);
    assert(reply->focus == win);
    free(reply);
}

MPX_TEST("find_window", {
    int (*matches)() = []()->int{return 1;};
    assertEquals(findWindow(matches, getAllWindows()), top);
    ArrayList<WindowID>ignore = {top->getID()};
    assertEquals(findWindow(matches, getAllWindows(), &ignore), middle);
    int (*noMatches)() = []()->int{return 0;};
    assert(!findWindow(noMatches, getAllWindows(), &ignore));
});
MPX_TEST_ITER("find_and_raise_basic", 4, {
    static std::string titles[] = {"a", "c", "d"};
    int i = 0;
    int (*matches)() = []()->int{return 0;};
    assert(findAndRaise(matches) == NULL);
    for(auto title : titles) {
        getAllWindows()[i++]->setTitle(title);
    }
    static std::string t;
    for(auto title : titles) {
        t = title;
        int (*matches)(WindowInfo*) = [](WindowInfo * winInfo)->int{return winInfo->matches(t);};
        assertEquals(findAndRaise(matches, (WindowAction)_i)->getTitle(), title);
    }
});
MPX_TEST_ITER("find_and_raise_cache", 2, {
    int (*matches)() = []()->int{return 1;};
    bool cache = _i;
    assert(getFocusedWindow());
    WindowInfo* winInfo = findAndRaise(matches, ACTION_RAISE, 1, cache);
    WindowInfo* winInfo2 = findAndRaise(matches, ACTION_RAISE, 1, cache);
    if(cache)
        assert(winInfo != winInfo2);
    else
        assertEquals(winInfo, winInfo2);
});
MPX_TEST_ITER("find_and_raise_local", 2, {
    WindowInfo* winInfo = getAllWindows()[1];
    getActiveMaster()->onWindowFocus(winInfo->getID());
    int (*matches)() = []()->int{return 1;};
    WindowInfo* winInfo2 = findAndRaise(matches, ACTION_RAISE, _i, 0);
    if(_i)
        assertEquals(winInfo, winInfo2);
    else
        assert(winInfo != winInfo2);
});


MPX_TEST("test_get_next_window_in_stack_fail", {
    for(WindowInfo* winInfo : getAllWindows())
        winInfo->addMask(HIDDEN_MASK);
    assert(getNextWindowInStack(0) == NULL);
    assert(getNextWindowInStack(1) == NULL);
    assert(getNextWindowInStack(-1) == NULL);
}
        );
MPX_TEST("test_get_next_window_in_stack", {
    bottom->addMask(HIDDEN_MASK);
    getActiveMaster()->onWindowFocus(top->getID());
    assert(getFocusedWindow() == top);
    assert(getNextWindowInStack(0) == top);
    assert(getNextWindowInStack(DOWN) == getActiveWindowStack()[1]);
    //bottom is marked hidden
    assert(getNextWindowInStack(UP) != bottom);
    assert(getNextWindowInStack(UP) == getActiveWindowStack()[getActiveWindowStack().size() - 2]);
    assert(getFocusedWindow() == top);
    switchToWorkspace(!getActiveWorkspaceIndex());
    assert(getNextWindowInStack(0) != getFocusedWindow());
}
        );

MPX_TEST("test_activate_top_bottom", {
    assert(focusBottom());
    assertWindowIsFocused(bottom->getID());
    assert(focusTop());
    assertWindowIsFocused(top->getID());
    assert(focusBottom());
    assertWindowIsFocused(bottom->getID());
}
        );
MPX_TEST("test_shift_top", {
    getActiveMaster()->onWindowFocus(bottom->getID());
    shiftTop();
    assert(getActiveWindowStack()[0] == bottom);
    assert(getActiveWindowStack()[1] == top);
}
        );
MPX_TEST("test_shift_focus", {
    for(WindowInfo* winInfo : getActiveWindowStack()) {
        getActiveMaster()->onWindowFocus(winInfo->getID());
        int id = shiftFocus(1);
        assert(id);
        assertWindowIsFocused(id);
        getActiveMaster()->onWindowFocus(id);
    }
}
        );
MPX_TEST("test_swap_top", {
    getActiveMaster()->onWindowFocus(bottom->getID());
    swapWithTop();
    assert(getActiveWindowStack()[0] == bottom);
    assert(getActiveWindowStack().back() == top);
}
        );


MPX_TEST("test_swap_position", {
    getActiveMaster()->onWindowFocus(bottom->getID());
    swapPosition(1);
    assert(getActiveWindowStack()[0] == bottom);
    assert(getActiveWindowStack().back() == top);
}
        );

MPX_TEST("test_activate_workspace_with_mouse", {
    assert(getAllMonitors().size() == 1);
    Rect bounds = Rect(getAllMonitors()[0]->getBase());
    bounds.width /= 2;
    getAllMonitors()[0]->setBase(bounds);
    bounds.x += bounds.width;
    Monitor* m = new Monitor(2, bounds);
    getAllMonitors().add(m);
    m->assignWorkspace();
    movePointer(getActiveMasterPointerID(), root, bounds.x, bounds.y);
    flush();
    activateWorkspaceUnderMouse();
    assertEquals(m, getActiveWorkspace()->getMonitor());
});

MPX_TEST("test_send_to_workspace_by_name", {

    WindowInfo* winInfo = getAllWindows()[0];
    for(Workspace* w : getAllWorkspaces()) {
        sendWindowToWorkspaceByName(winInfo, w->getName());
        assert(winInfo->getWorkspace() == w);
    }
    Workspace* w = winInfo->getWorkspace();
    sendWindowToWorkspaceByName(winInfo, "__bad__workspace__name__");
    assert(w);
    assert(w == winInfo->getWorkspace());
});


