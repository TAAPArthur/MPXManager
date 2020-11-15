#include "tester.h"
#include "test-event-helper.h"
#include "../wmfunctions.h"

static void setupOwnSelection() {
    createXSimpleEnv();
    assert(!isMPXManagerRunning());
    ownSelection(MPX_WM_SELECTION_ATOM);
}
SCUTEST_SET_ENV(setupOwnSelection, cleanupXServer);
SCUTEST(test_own_selection) {
    assert(isMPXManagerRunning());
}

SCUTEST(test_steal_own_selection) {
    STEAL_WM_SELECTION = 1;
    ownSelection(MPX_WM_SELECTION_ATOM);
    assert(isMPXManagerRunning());
}
SCUTEST(test_steal_own_selection_err, .exitCode = WM_ALREADY_RUNNING) {
    ownSelection(MPX_WM_SELECTION_ATOM);
    fail();
}
SCUTEST_SET_ENV(createXSimpleEnv, cleanupXServer);
SCUTEST(test_register_unregister_window) {
    WindowID win = createUnmappedWindow();
    addEvent(POST_REGISTER_WINDOW, DEFAULT_EVENT(incrementCount));
    addEvent(UNREGISTER_WINDOW, DEFAULT_EVENT(incrementCount));
    registerWindow(win, root, NULL);
    assertEquals(1, getCount());
    unregisterWindow(getWindowInfo(win), 0);
    assertEquals(2, getCount());
}

SCUTEST(test_window_scan) {
    scan(root);
    WindowID win = createUnmappedWindow();
    WindowID win2 = createUnmappedWindow();
    WindowID win3 = createUnmappedWindow();
    WindowID win4 = createOverrideRedirectWindow();
    assert(!isWindowMapped(win));
    assert(!isWindowMapped(win2));
    assert(!isWindowMapped(win3));
    mapWindow(win);
    flush();
    scan(root);
    assert(getWindowInfo(win));
    assert(getWindowInfo(win2));
    assert(getWindowInfo(win3));
    assert(!getWindowInfo(win3)->overrideRedirect);
    assert(getWindowInfo(win4));
    assert(getWindowInfo(win4)->overrideRedirect);
    assertEquals(getAllWindows()->size, 4);
}

SCUTEST(test_activate_window) {
    addWorkspaces(1);
    WindowID win = createUnmappedWindow();
    WindowID win2 = mapWindow(createNormalWindow());
    WindowID win3 = mapWindow(createNormalWindow());
    scan(root);
    assert(isActivatable(getWindowInfo(win2)));
    assert(!activateWindow(getWindowInfo(win)));
    addMask(getWindowInfo(win), MAPPABLE_MASK);
    assert(activateWindow(getWindowInfo(win)));
    assert(activateWindow(getWindowInfo(win2)));
    removeMask(getWindowInfo(win2), ALL_MASK);
    assert(!activateWindow(getWindowInfo(win2)));
    assert(isActivatable(getWindowInfo(win3)));
    moveToWorkspace(getWindowInfo(win3), 1);
    removeMask(getWindowInfo(win3), MAPPED_MASK);
    assert(!isWorkspaceVisible(getWorkspace(1)));
    assert(isActivatable(getWindowInfo(win3)));
    assert(activateWindow(getWindowInfo(win3)));
    assertEquals(getActiveWorkspaceIndex(), 1);
}

SCUTEST(test_activate_workspace) {
    addWorkspaces(1);
    WindowID win = mapWindow(createNormalWindow());
    WindowID win2 = mapWindow(createNormalWindow());
    WindowID win3 = mapWindow(createNormalWindow());
    scan(root);
    moveToWorkspace(getWindowInfo(win), 1);
    moveToWorkspace(getWindowInfo(win3), 0);
    moveToWorkspace(getWindowInfo(win2), 0);
    activateWorkspace(1);
    assertEquals(getActiveFocus(), win);
    assertEquals(getActiveWorkspace()->id, 1);
    activateWorkspace(0);
    assertEquals(getActiveFocus(), win3);
}

SCUTEST(test_sticky_workspace_change) {
    WindowID win = mapArbitraryWindow();
    WindowInfo* winInfo = addWindow(win);
    moveToWorkspace(winInfo, 0);
    addMask(winInfo, STICKY_MASK);
    for(int i = 0; i < getNumberOfWorkspaces(); i++) {
        switchToWorkspace(i);
        assert(getWorkspaceIndexOfWindow(winInfo) == i);
    }
}

SCUTEST(test_sticky_workspace_change_multiple_masters) {
    addWorkspaces(3);
    createMasterDevice("test");
    createMasterDevice("test");
    initCurrentMasters();
    assertEquals(getAllMonitors()->size, 1);
    Master*originalMaster=getActiveMaster();
    int i=0;
    FOR_EACH(Master*,master, getAllMasters()) {
        setActiveMaster(master);
        switchToWorkspace(i++);
    }
    setActiveMaster(originalMaster);
    switchToWorkspace(i);
    assert(isWorkspaceVisible(getWorkspace(i)));
}

SCUTEST(test_workspace_change) {
    addWorkspaces(2);
    int nonEmptyIndex = 0;
    mapArbitraryWindow();
    mapArbitraryWindow();
    scan(root);
    FOR_EACH(WindowInfo*, winInfo, getAllWindows()) {
        moveToWorkspace(winInfo, 0);
    }
    assert(getAllWindows()->size == 2);
    assert(getActiveWindowStack()->size == 2);
    assert(getNumberOfWorkspaces() > 1);
    for(int i = 0; i < getNumberOfWorkspaces(); i++) {
        switchToWorkspace(i);
        if(i == nonEmptyIndex)continue;
        assert(getActiveWorkspace()->id == i);
        assert(getActiveWindowStack()->size == 0);
        assert(getActiveWorkspaceIndex() == i);
    }
};


SCUTEST(test_configure_windows, .iter = 3) {
    static int mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
    WindowID win = createNormalWindow();
    Rect original = {0, 0, 1, 1};
    Rect values = {1, 2, 3, 4};
    setWindowPosition(win, original);
    scan(root);
    WindowInfo* winInfo = getWindowInfo(win);
    bool fail = 0;
    if(_i == 0)
        addMask(winInfo, EXTERNAL_CONFIGURABLE_MASK);
    else if(_i == 1)
        removeMask(winInfo, MAPPED_MASK | MAPPABLE_MASK);
    else {
        addMask(winInfo, MAPPED_MASK | MAPPABLE_MASK);
        fail = 1;
    }
    processConfigureRequest(win, &values.x, 0, 0, mask);
    Rect rect = getRealGeometry(win);
    if(fail)
        assertEqualsRect(rect, original);
    else
        assertEqualsRect(values, rect);
}


SCUTEST_ITER(test_raise_window_ignore_unmanaged, 2) {
    WindowID bottom = mapArbitraryWindow();
    WindowID top = mapArbitraryWindow();
    WindowID stackingOrder[] = {createOverrideRedirectWindow(), getWindowDivider(0), bottom, top, getWindowDivider(1), createOverrideRedirectWindow()};
    lowerWindow(stackingOrder[0], 0);
    scan(root);
    assert(checkStackingOrder(stackingOrder, LEN(stackingOrder)));
    if(_i) {
        raiseWindowInfo(getWindowInfo(bottom), 0);
        raiseWindowInfo(getWindowInfo(top), 0);
        assert(checkStackingOrder(stackingOrder, LEN(stackingOrder)));
    }
    else {
        lowerWindowInfo(getWindowInfo(top), 0);
        lowerWindowInfo(getWindowInfo(bottom), 0);
        assert(checkStackingOrder(stackingOrder, LEN(stackingOrder)));
    }
}

SCUTEST(test_raise_window) {
    //windows are in same spot
    WindowID bottom = mapArbitraryWindow();
    WindowID top = mapArbitraryWindow();
    registerForWindowEvents(bottom, XCB_EVENT_MASK_VISIBILITY_CHANGE);
    registerForWindowEvents(top, XCB_EVENT_MASK_VISIBILITY_CHANGE);
    scan(root);
    raiseWindow(bottom, 0);
    flush();
    WindowID stackingOrder[] = {top, bottom, top};
    assert(checkStackingOrder(stackingOrder, 2));
    raiseWindow(top, 0);
    assert(checkStackingOrder(stackingOrder + 1, 2));
    lowerWindow(top, 0);
    assert(checkStackingOrder(stackingOrder, 2));
}
SCUTEST(test_raise_window_sibling) {
    //windows are in same spot
    WindowID bottom = mapArbitraryWindow();
    WindowID top = mapArbitraryWindow();
    WindowID sibling = mapArbitraryWindow();
    scan(root);
    lowerWindow(bottom, 0);
    raiseWindow(top, 0);
    raiseWindow(bottom, sibling);
    WindowID stackingOrder[] = {sibling, bottom, top};
    assert(checkStackingOrder(stackingOrder, 2));
    assert(checkStackingOrder(stackingOrder, 3));
    lowerWindow(sibling, bottom);
    assert(checkStackingOrder(stackingOrder, 3));
    lowerWindow(bottom, top);
    assert(checkStackingOrder(stackingOrder, 3));
    lowerWindow(top, sibling);
    assert(!checkStackingOrder(stackingOrder, 3));
}
SCUTEST(test_raise_window_just_above) {
    WindowID bottom = mapArbitraryWindow();
    WindowID sibling = mapArbitraryWindow();
    WindowID top = mapArbitraryWindow();
    WindowID stackingOrder[] = {bottom, sibling,  top};
    assert(checkStackingOrder(stackingOrder, 3));
    raiseWindow(top, bottom);
    WindowID stackingOrder2[] = {bottom,  top, sibling };
    assert(checkStackingOrder(stackingOrder2, 3));
}

SCUTEST(test_raise_lower_above_below_windows) {
    WindowID lower = mapArbitraryWindow();
    WindowID base = mapArbitraryWindow();
    WindowID upper = mapArbitraryWindow();
    WindowID stackingOrder[] = {lower, base, upper};
    scan(root);
    assert(checkStackingOrder(stackingOrder, 3));
    addMask(getWindowInfo(upper), ABOVE_MASK);
    addMask(getWindowInfo(lower), BELOW_MASK);
    for(int i = 0; i < LEN(stackingOrder); i++)
        raiseWindowInfo(getWindowInfo(stackingOrder[i]), 0);
    assert(checkStackingOrder(stackingOrder, 3));
    lowerWindowInfo(getWindowInfo(upper), 0);
    assert(checkStackingOrder(stackingOrder, 3));
    raiseWindowInfo(getWindowInfo(lower), 0);
    assert(checkStackingOrder(stackingOrder, 3));
}
SCUTEST(test_updateFocusForAllMasters) {
    addWorkspaces(1);
    WindowID win = mapArbitraryWindow();
    WindowID win2 = mapArbitraryWindow();
    scan(root);
    onWindowFocus(win);
    onWindowFocus(win2);
    updateFocusForAllMasters(getWindowInfo(win2));
    assertEquals(getActiveFocus(), win);
}

