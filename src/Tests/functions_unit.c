#include "../functions.h"
#include "../layouts.h"
#include "../masters.h"
#include "../wmfunctions.h"
#include "test-event-helper.h"
#include "test-mpx-helper.h"
#include "test-wm-helper.h"
#include "test-x-helper.h"
#include "tester.h"


static WindowInfo* top;
static WindowInfo* middle;
static WindowInfo* bottom;
static void functionSetup() {
    onSimpleStartup();
    addShutdownOnIdleRule();
    int size = 3;
    for(int i = 0; i < size; i++)
        mapWindow(createNormalWindow());
    scan(root);
    FOR_EACH(WindowInfo*, winInfo, getAllWindows()) {
        moveToWorkspace(winInfo, 0);
        focusWindowInfo(winInfo);
    }
    top = getElement(getActiveWindowStack(), 0);
    middle = getElement(getActiveWindowStack(), 1);
    bottom = getElement(getActiveWindowStack(), 2);
    runEventLoop();
}

SCUTEST_SET_ENV(functionSetup, simpleCleanup);

SCUTEST_ITER(cycle_window, 2) {
    if(_i) {
        int i = 0;
        FOR_EACH(WindowInfo*, winInfo, getAllWindows()) {
            moveToWorkspace(winInfo, i++);
        }
        runEventLoop();
    }
    onWindowFocus(middle->id);
    onWindowFocus(top->id);
    focusWindowInfo(top);
    assertEquals(getFocusedWindow(), top);
    WindowID stack[] = {middle->id, bottom->id, top->id};
    assertEquals(getActiveFocus(), stack[LEN(stack) - 1]);
    runEventLoop();
    setFocusStackFrozen(1);
    for(int i = 0; i < LEN(stack); i++) {
        cycleWindows(DOWN);
        assertEquals(getActiveFocus(), stack[i]);
        runEventLoop();
    }
    cycleWindows(UP);
    assertEquals(getActiveFocus(), stack[1]);
}

SCUTEST(cycle_window_many, .iter = 2) {
    IDLE_TIMEOUT = 0;
    for(int i = 0; i < 2; i++)
        registerWindow(mapWindow(createNormalWindow()), root, NULL);
    FOR_EACH(WindowInfo*, winInfo, getAllWindows()) {
        focusWindowInfo(winInfo);
        moveToWorkspace(winInfo, 0);
    }
    moveToWorkspace(getElement(getAllWindows(), 1), 1);
    moveToWorkspace(getElement(getAllWindows(), 2), 1);
    runEventLoop();
    int arr[getAllWindows()->size];
    for(int i = 0; i < LEN(arr); i++)arr[i] = 0;
    int N = 3;
    setFocusStackFrozen(1);
    WindowID lastWindowFocused = getFocusedWindow()->id;
    for(int i = 0; i < getAllWindows()->size * N - 1; i++) {
        assertEquals(lastWindowFocused, getFocusedWindow()->id);
        cycleWindows(DOWN);
        lastWindowFocused = getFocusedWindow()->id;
        int index = getIndex(getAllWindows(), getFocusedWindow(), sizeof(WindowID));
        assert(index >= 0);
        arr[index]++;
        PRINT_ARR("Arr", arr, LEN(arr));
        for(int n = 0; n < LEN(arr); n++) {
            assert(arr[index] - arr[n] <= 1);
        }
        ERROR("Waiting %d\n\n\n\n\n\n\n", i);
        if(_i)
            runEventLoop();
        ERROR("Done\n\n\n\n\n\n\n");
    }
}

void assertWindowIsFocused(WindowID win) {
    xcb_input_xi_get_focus_reply_t* reply = xcb_input_xi_get_focus_reply(dis, xcb_input_xi_get_focus(dis,
                getActiveMasterKeyboardID()), NULL);
    assert(reply->focus == win);
    free(reply);
}

SCUTEST_ITER(test_raiseOrRun, 16) {
    WindowInfo* winInfo = getHead(getAllWindows());
    const char* sAsEnvVar = "$_unique";
    const char* s = sAsEnvVar + 1;
    const char* cmd = NULL;
    const char* envVar = "var";
    bool run = !_i;
    bool env = _i & 2;
    bool title = _i & 4;
    bool role = _i & 8;
    if(!run) {
        if(role)
            setWindowRole(winInfo->id, s);
        else if(title)
            setWindowTitle(winInfo->id, s);
        else {
            setWindowClass(winInfo->id, s, s);
        }
        loadWindowProperties(winInfo);
    }
    if(env) {
        setenv(envVar, s, 1);
        s = sAsEnvVar;
    }
    int pid = raiseOrRunFunc(s, cmd,UP, role ? matchesRole : title ? matchesTitle : matchesClass);
    assertEquals(!run, !pid);
}

SCUTEST(test_raiseOrRun_diff) {
    const char* str = "role";
    FOR_EACH_R(WindowInfo*, winInfo, getAllWindows()) {
        setWindowRole(winInfo->id, str);
        loadWindowProperties(winInfo);
        focusWindowInfo(winInfo);
    }
    runEventLoop();
    assertEquals(getFocusedWindow()->id, top->id);
    assert(!raiseOrRunFunc(str, NULL, DOWN, matchesRole));
    runEventLoop();
    assertEquals(getFocusedWindow()->id, middle->id);
    assert(!raiseOrRunFunc(str, NULL, DOWN, matchesRole));
    runEventLoop();
    assertEquals(getFocusedWindow()->id, top->id);
}

SCUTEST(test_shift_top_and_focus) {
    onWindowFocus(bottom->id);
    shiftTopAndFocus();
    assertEquals(getElement(getActiveWindowStack(), 0), bottom);
    assertEquals(getElement(getActiveWindowStack(), 1), top);
    assertEquals(bottom->id, getActiveFocus());
    shiftTopAndFocus();
    assertEquals(getElement(getActiveWindowStack(), 0), top);
    assertEquals(getElement(getActiveWindowStack(), 1), bottom);
    assertEquals(top->id, getActiveFocus());
}
SCUTEST_ITER(test_shift_focus_null, 3) {
    if(_i == 0)
        clearFocusStack(getActiveMaster());
    else if(_i == 1)
        switchToWorkspace(2);
    else {
        FOR_EACH(WindowInfo*, winInfo, getAllWindows()) {
            addMask(winInfo, HIDDEN_MASK);
        }
    }
    shiftFocus(1, NULL);
}
SCUTEST(test_skip_non_focusable) {
    focusWindowInfo(top);
    focusWindowInfo(bottom);
    removeMask(middle, INPUT_MASK);
    runEventLoop();
    shiftFocus(1, NULL);
    runEventLoop();
    assertEquals(getFocusedWindow(), top);
    shiftFocus(1, NULL);
    runEventLoop();
    assertEquals(getFocusedWindow(), bottom);
}

SCUTEST(test_swap_position) {
    onWindowFocus(bottom->id);
    swapPosition(1);
    assert(getElement(getActiveWindowStack(), 0) == bottom);
    assert(pop(getActiveWindowStack()) == top);
}

SCUTEST(test_swap_position_unfocused) {
    // don't crash
    swapPosition(1);
}

SCUTEST(test_activate_workspace_with_mouse_smallest) {
    assert(getAllMonitors()->size == 1);
    Monitor* m = addFakeMonitor((Rect) {0, 0, 2, 2});
    assignUnusedMonitorsToWorkspaces();
    movePointer(1, 1);
    flush();
    activateWorkspaceUnderMouse();
    assertEquals(m, getMonitor(getActiveWorkspace()));
}

SCUTEST(test_center_mouse) {
    toggleActiveLayout(&GRID);
    retile();
    short pos[2];
    movePointer(0, 0);
    Master* master = getActiveMaster();
    FOR_EACH(WindowInfo*, winInfo, getActiveWindowStack()) {
        centerMouseInWindow(winInfo);
        assert(getMousePosition(getPointerID(master), root, pos));
        winInfo->geometry = (getRealGeometry(winInfo->id));
        assert(contains(getRealGeometry(winInfo->id), (Rect) {pos[0], pos[1], 1, 1}));
    }
}

SCUTEST(test_send_to_workspace_by_name) {
    WindowInfo* winInfo = getElement(getAllWindows(), 0);
    FOR_EACH(Workspace*, w, getAllWorkspaces()) {
        sendWindowToWorkspaceByName(winInfo, w->name);
        assert(getWorkspaceOfWindow(winInfo) == w);
    }
    Workspace* w = getWorkspaceOfWindow(winInfo);
    sendWindowToWorkspaceByName(winInfo, "__bad__workspace__name__");
    assert(w);
    assert(w == getWorkspaceOfWindow(winInfo));
}


SCUTEST(pop_hidden) {
    WindowInfo* winInfo = getHead(getAllWindows());
    addMask(winInfo, HIDDEN_MASK);
    popHiddenWindow();
    assert(!hasMask(winInfo, HIDDEN_MASK));
}
SCUTEST(activate_urgent) {
    addMask(middle, URGENT_MASK);
    activateNextUrgentWindow();
    assertEquals(getActiveFocus(), middle->id);
}
SCUTEST(test_wm_move) {
    movePointer(0, 0);
    int deltaX=10, deltaY=20;
    WindowInfo* winInfo = getElement(getAllWindows(), 0);
    WindowInfo* winInfo2 = getElement(getAllWindows(), 1);
    startWindowMoveResize(winInfo, 1, 0);
    startWindowMoveResize(winInfo2, 1, 0);
    movePointer(deltaX, deltaY);
    updateWindowMoveResize();
    assertEquals(0, getRealGeometry(winInfo2->id).x);
    assertEquals(0, getRealGeometry(winInfo2->id).y);
    assertEquals(deltaX, getRealGeometry(winInfo->id).x);
    assertEquals(deltaY, getRealGeometry(winInfo->id).y);
}
SCUTEST(test_wm_resize_invert) {
    WindowInfo* winInfo = getElement(getAllWindows(), 0);
    int N = 10;
    Rect originalPos = {N, N, N, N};
    setWindowPosition(winInfo->id, originalPos);
    movePointer(2*N, 2*N);
    startWindowMoveResize(winInfo, 0, 0);
    movePointer(0, 0);
    updateWindowMoveResize();
    Rect pos = {0, 0, N, N};
    assertEqualsRect(pos, getRealGeometry(winInfo->id));
    movePointer(2*N, 2*N);
    flush();
    updateWindowMoveResize();
    assertEqualsRect(originalPos, getRealGeometry(winInfo->id));
}

SCUTEST(test_wm_cancel) {
    WindowInfo* winInfo = getElement(getAllWindows(), 0);
    startWindowMoveResize(winInfo, 0, 0);
    Rect originalPos = getRealGeometry(winInfo->id);
    movePointer(100, 100);
    updateWindowMoveResize();
    cancelWindowMoveResize();
    assertEqualsRect(originalPos, getRealGeometry(winInfo->id));
}
SCUTEST(test_wm_commit) {
    movePointer(0, 0);
    WindowInfo* winInfo = getElement(getAllWindows(), 0);
    startWindowMoveResize(winInfo, 1, 0);
    movePointer(100, 100);
    updateWindowMoveResize();
    commitWindowMoveResize();
    assertEquals(100, getRealGeometry(winInfo->id).x);
    assertEquals(100, getRealGeometry(winInfo->id).y);
    movePointer(0, 0);
    updateWindowMoveResize();
    assertEquals(100, getRealGeometry(winInfo->id).x);
    assertEquals(100, getRealGeometry(winInfo->id).y);
}
SCUTEST_SET_ENV(onDefaultStartup, cleanupXServer);
SCUTEST(test_swap_windows, .iter=2) {
    mapWindow(createNormalWindow());
    WindowID win1 = mapWindow(createNormalWindow());
    mapWindow(createNormalWindow());
    WindowID win2 = mapWindow(createNormalWindow());
    mapWindow(createNormalWindow());
    scan(root);
    moveToWorkspace(getWindowInfo(win2), 1);
    setActiveLayout(&GRID);
    addShutdownOnIdleRule();
    runEventLoop();
    Rect pos1 = getRealGeometry(win1);
    Rect pos2 = getRealGeometry(win2);
    swapWindows(win1, win2);
    if(_i) {
        addFailOnEventRule(TILE_WORKSPACE);
    } else {
        retile();
    }
    runEventLoop();
    assertEqualsRect(pos2, getRealGeometry(win1));
    assertEqualsRect(pos1, getRealGeometry(win2));
    assertEquals(getWorkspaceOfWindow(getWindowInfo(win1))->id, 1);
    assertEquals(getWorkspaceOfWindow(getWindowInfo(win2))->id, 0);
}
