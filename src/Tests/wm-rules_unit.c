#include <unistd.h>
#include <signal.h>

#include "tester.h"
#include "test-event-helper.h"
#include "test-wm-helper.h"
#include "../wm-rules.h"
#include "../layouts.h"
#include "../wmfunctions.h"
#include "../layouts.h"

static void setupEnvWithBasicRules() {
    registerDefaultLayouts();
    addBasicRules();
    addWorkspaces(1);
    addShutdownOnIdleRule();
    openXDisplay();
}
SCUTEST_SET_ENV(setupEnvWithBasicRules, cleanupXServer);

SCUTEST(test_init_state) {
    assert(isMPXManagerRunning());
    assert(getAllWorkspaces()->size);
    assert(getAllMonitors()->size);
    assert(getAllMasters()->size);
    FOR_EACH(Workspace*, workspace, getAllWorkspaces()) {
        assert(getLayout(workspace));
    }
}

SCUTEST(test_create_window_destroy) {
    WindowID win = mapWindow(createNormalWindow());
    runEventLoop();
    assert(getWindowInfo(win));
    destroyWindow(win);
    runEventLoop();
    assert(!getWindowInfo(win));
}

SCUTEST(test_get_private_window) {
    getPrivateWindow();
    runEventLoop();
    assert(!getWindowInfo(getPrivateWindow()));
}


SCUTEST(test_unmap) {
    WindowID win = mapWindow(createNormalWindow());
    registerWindow(win, root, NULL);
    xcb_unmap_notify_event_t event = {.response_type = XCB_UNMAP_NOTIFY, .window = win};
    catchError(xcb_send_event_checked(dis, 0, root, ROOT_EVENT_MASKS, (char*) &event));
    runEventLoop();
    assert(!hasPartOfMask(getWindowInfo(win), MAPPED_MASK | MAPPABLE_MASK));
    unmapWindow(win);
    waitUntilIdle();
    assert(!hasPartOfMask(getWindowInfo(win), MAPPED_MASK | MAPPABLE_MASK));
}

SCUTEST(test_focus_window) {
    createMasterDevice("test");
    initCurrentMasters();
    setActiveMaster(getElement(getAllMasters(), 1));
    WindowID win = mapWindow(createNormalWindow());
    WindowID win2 = mapWindow(createNormalWindow());
    runEventLoop();
    for(int i = 0; i < 2; i++) {
        focusWindow(win, getActiveMaster());
        runEventLoop();
        assertEquals(getFocusedWindow(), getWindowInfo(win));
        assertEquals(getActiveFocus(getActiveMasterKeyboardID()), win);
        setActiveMaster(getElement(getAllMasters(), 0));
    }
    focusWindow(win2, getActiveMaster());
    runEventLoop();
    assertEquals(getFocusedWindow(), getWindowInfo(win2));
    assertEquals(getActiveFocus(getActiveMasterKeyboardID()), win2);
    setActiveMaster(getElement(getAllMasters(), 1));
    assertEquals(getFocusedWindow(), getWindowInfo(win));
    assertEquals(getActiveFocus(getActiveMasterKeyboardID()), win);
}

SCUTEST(test_focus_transfer_on_destroy) {
    createMasterDevice("test");
    createMasterDevice("test2");
    initCurrentMasters();
    WindowID win = mapWindow(createNormalWindow());
    WindowID win2 = mapWindow(createNormalWindow());
    runEventLoop();
    FOR_EACH(Master*, master, getAllMasters()) {
        focusWindow(win, master);
    }
    focusWindow(win2, getActiveMaster());
    focusWindow(win2, getMasterByName("test"));
    focusWindow(win, getMasterByName("test"));
    destroyWindow(win2);
    runEventLoop();
    FOR_EACH(Master*, master, getAllMasters()) {
        assertEquals(win, getActiveFocus(getKeyboardID(master)));
        assertEquals(win, getFocusedWindowOfMaster(master)->id);
    }
}

SCUTEST(test_switching_workspaces) {
    addWorkspaces(1);
    WindowID win = mapWindow(createNormalWindow());
    WindowID win2 = mapWindow(createNormalWindow());
    registerWindow(win, root, NULL);
    registerWindow(win2, root, NULL);
    moveToWorkspace(getWindowInfo(win), 0);
    moveToWorkspace(getWindowInfo(win2), 1);
    switchToWorkspace(1);
    runEventLoop();
    assert(!isWindowMapped(win));
    assert(isWindowMapped(win2));
    switchToWorkspace(0);
    runEventLoop();
    assert(isWindowMapped(win));
    assert(!isWindowMapped(win2));
}
static void setupEnvWithAutoTileRules() {
    addAutoTileRules();
    setupEnvWithBasicRules();
    addEvent(TILE_WORKSPACE, DEFAULT_EVENT(incrementCount));
}
SCUTEST_SET_ENV(setupEnvWithAutoTileRules, cleanupXServer);
SCUTEST(test_auto_tile, .iter = 4) {
    WindowID win = mapWindow(createNormalWindow());
    WindowID win2 = createNormalWindow();
    runEventLoop();
    assertEquals(0, getCount());
    moveToWorkspace(getWindowInfo(win), 0);
    runEventLoop();
    assertEquals(1, getCount());
    mapWindow(createNormalWindow());
    runEventLoop();
    assertEquals(1, getCount());
    switch(_i) {
        case 0:
            moveToWorkspace(getWindowInfo(win2), 0);
            break;
        case 1:
            addMask(getWindowInfo(win), FLOATING_MASK);
            break;
        case 2:
            toggleActiveLayout(&GRID);
            break;
        case 3:
            getMonitor(getActiveWorkspace())->view = (Rect) {0, 0, 1, 1};
            break;
    }
    runEventLoop();
    assertEquals(2, getCount());
}
static Binding bindings[] = {
    {0, 1, incrementCount},
    {0, XK_A, incrementCount}
};
static void setupEnvWithBasicRulesAndBindings() {
    addBindings(bindings, LEN(bindings));
    setupEnvWithBasicRules();
}
SCUTEST_SET_ENV(setupEnvWithBasicRulesAndBindings, cleanupXServer);
SCUTEST(test_trigger_mouse_binding) {
    sendButtonPress(1, getActiveMasterPointerID());
    runEventLoop();
    assertEquals(1, getCount());
}
SCUTEST(test_trigger_key_binding) {
    sendKeyPress(getKeyCode(XK_A), getActiveMasterKeyboardID());
    runEventLoop();
    assertEquals(1, getCount());
}

static void setupClientEnvWithBasicRules() {
    createSigAction(SIGCHLD, requestShutdown);
    int pid = fork();
    if(pid) {
        addBasicRules();
        addWorkspaces(1);
        openXDisplay();
        kill(pid, SIGCHLD);
        runEventLoop();
        assert(getAllWindows()->size);
        assertEquals(0, waitForChild(pid));
        cleanupXServer();
        exit(0);
    }
    openXDisplay();
    registerForWindowEvents(root, XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY);
    pause();
    consumeEvents();
}
SCUTEST_SET_ENV(setupClientEnvWithBasicRules, cleanupXServer);

SCUTEST(test_map_request) {
    WindowID win = mapWindow(createNormalWindow());
    waitForNormalEvent(XCB_MAP_NOTIFY);
    assert(isWindowMapped(win));
}

static int masks[] = {XCB_CONFIG_WINDOW_X, XCB_CONFIG_WINDOW_Y, XCB_CONFIG_WINDOW_WIDTH, XCB_CONFIG_WINDOW_HEIGHT,  XCB_CONFIG_WINDOW_BORDER_WIDTH};
SCUTEST(test_configure_request, .iter = LEN(masks)) {
    CRASH_ON_ERRORS = 0;
    int value = 2;
    int allMasks = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
        XCB_CONFIG_WINDOW_BORDER_WIDTH;
    int defaultValues[] = {10, 11, 12, 13, 14};
    WindowID win = createNormalWindow();
    waitForNormalEvent(XCB_CONFIGURE_NOTIFY);
    assert(!catchError(xcb_configure_window_checked(dis, win, allMasks, defaultValues)));
    waitForNormalEvent(XCB_CONFIGURE_NOTIFY);
    assert(!catchError(xcb_configure_window_checked(dis, win, masks[_i], &value)));
    waitForNormalEvent(XCB_CONFIGURE_NOTIFY);
    xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, win), NULL);
    assert(reply);
    for(int n = 0; n < 5; n++)
        assertEquals((&reply->x)[n], (n == _i ? value : defaultValues[n]));
    free(reply);
}
