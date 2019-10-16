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
    addEWMHRules();
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
    WindowID stack[] = {middle->getID(), top->getID(), bottom->getID()};
    for(WindowID active : stack) {
        ATOMIC(cycleWindows(DOWN));
        assertEquals(getActiveFocus(), active);
        assert(getActiveMaster()->isFocusStackFrozen());
        waitUntilIdle();
    }

    ATOMIC(cycleWindows(UP));
    assertEquals(getActiveFocus(), top->getID());
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
    int (*matches)() = []() {return 1;};
    assertEquals(findWindow(matches, getAllWindows()), top);
    ArrayList<WindowID>ignore = {top->getID()};
    assertEquals(findWindow(matches, getAllWindows(), &ignore), middle);
    int (*noMatches)() = []() {return 0;};
    assert(!findWindow(noMatches, getAllWindows(), &ignore));
    assert(!findWindow(noMatches, getAllWindows(), NULL));
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
MPX_TEST_ITER("raiseOrRun", 8, {
    WindowInfo* winInfo = getAllWindows()[1];
    std::string s = "_unique";
    std::string cmd = "exit 1";
    std::string envVar = "var";
    bool run = _i ? 1 : 0;
    bool env = _i % 2;
    bool title = _i & 4;
    if(!run)
        if(title)
            winInfo->setTitle(s);
        else
            winInfo->setClassName(s);
    if(env) {
        setenv(envVar.c_str(), s.c_str(), 1);
        s = "$" + envVar;
    }
    assertEquals(!run, raiseOrRun(s, cmd, !title));
    if(run)
        assertEquals(waitForChild(0), 1);
});
MPX_TEST("auto_clear_cache", {
    assert(getFocusedWindow());
    WindowInfo* winInfo = getFocusedWindow();
    assert(getAllWindows().size() == 3);
    startWM();

    waitUntilIdle();
    for(int i = 0; i < getAllWindows().size() - 1; i++) {
        assert(winInfo != findAndRaise({}, ACTION_ACTIVATE));
        waitUntilIdle();
    }
    assertEquals(*winInfo, *findAndRaise({}, ACTION_ACTIVATE));
});



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
});
MPX_TEST("test_shift_focus", {
    for(WindowInfo* winInfo : getActiveWindowStack()) {
        getActiveMaster()->onWindowFocus(winInfo->getID());
        int id = shiftFocus(1);
        assert(id);
        assertWindowIsFocused(id);
        getActiveMaster()->onWindowFocus(id);
    }
});
MPX_TEST_ITER("test_shift_focus_null", 3, {
    if(_i == 0)
        getActiveMaster()->getWindowStack().clear();
    else if(_i == 1)
        switchToWorkspace(2);
    else
        for(WindowInfo* winInfo : getAllWindows())
            winInfo->addMask(HIDDEN_MASK);
    shiftFocus(1);
    swapWithTop();
});
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


