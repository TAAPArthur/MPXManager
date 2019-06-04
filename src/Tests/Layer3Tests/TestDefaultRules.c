#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>

#include "../UnitTests.h"
#include "../TestX11Helper.h"
#include "../../state.h"
#include "../../logger.h"
#include "../../globals.h"
#include "../../wmfunctions.h"
#include "../../functions.h"
#include "../../layouts.h"
#include "../../devices.h"
#include "../../bindings.h"

WindowInfo* winInfo;
WindowInfo* winInfo2;
Master* defaultMaster;
static int count = 0;
static void dummy(void){
    count++;
}
static int getDummyCount(){
    return count;
}
static Binding testBinding = {0, 1, BIND(dummy), .mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS};
static Binding testKeyPressBinding = {0, XK_A, BIND(dummy), .mask = XCB_INPUT_XI_EVENT_MASK_KEY_PRESS};
static void sampleStartupMethod(){
    addBinding(&testBinding);
    addBinding(&testKeyPressBinding);
}
static Layout* startingLayout = &DEFAULT_LAYOUTS[FULL];
static void deviceEventsetup(){
    LOAD_SAVED_STATE = 0;
    CRASH_ON_ERRORS = -1;
    DEFAULT_WINDOW_MASKS = SRC_ANY;
    if(!startUpMethod)
        startUpMethod = sampleStartupMethod;
    NUMBER_OF_DEFAULT_LAYOUTS = 0;
    onStartup();
    setActiveLayout(startingLayout);
    defaultMaster = getActiveMaster();
    WindowID win1 = mapWindow(createNormalWindow());
    WindowID win2 = mapWindow(createNormalWindow());
    scan(root);
    xcb_icccm_set_wm_protocols(dis, win2, ewmh->WM_PROTOCOLS, 1, &ewmh->_NET_WM_PING);
    flush();
    createMasterDevice("device2");
    createMasterDevice("device3");
    flush();
    retile();
    int idle = getIdleCount();
    START_MY_WM;
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    winInfo = getWindowInfo(win1);
    winInfo2 = getWindowInfo(win2);
}

static void nonDefaultDeviceEventsetup(){
    startingLayout = &DEFAULT_LAYOUTS[GRID];
    startUpMethod = addFocusFollowsMouseRule;
    deviceEventsetup();
}

START_TEST(test_run_as_non_wm){
    RUN_AS_WM = 0;
    addSomeBasicRules((int[]){onXConnection, onWindowMove}, 2);
    onStartup();
    for(unsigned int i = 0; i < NUMBER_OF_EVENT_RULES; i++)
        if(i == onXConnection || i == onWindowMove)
            assert(isNotEmpty(getEventRules(i)));
        else
            assert(!isNotEmpty(getEventRules(i)));
}
END_TEST

START_TEST(test_on_startup){
    startUpMethod = dummy;
    onStartup();
    assert(count);
    assert(isNotEmpty(getAllMonitors()));
    assert(isNotEmpty(getAllMasters()));
    assert(dis);
    assert(!xcb_connection_has_error(dis));
    assert(isWorkspaceVisible(getActiveWorkspaceIndex()));
}
END_TEST
START_TEST(test_post_startup){
    postStartUpMethod = dummy;
    onStartup();
    assert(count);
}
END_TEST

START_TEST(test_print_method){
    printStatusMethod = dummy;
    // set to an open FD
    STATUS_FD = 1;
    applyRules(getEventRules(Idle), NULL);
    assert(getDummyCount());
}
END_TEST

START_TEST(test_detect_wm){
    if(!fork()){
        setLogLevel(LOG_LEVEL_NONE);
        onStartup();
    }
    wait(NULL);
}
END_TEST

START_TEST(test_die_on_idle){
    addDieOnIdleRule();
    createNormalWindow();
    flush();
    runEventLoop(NULL);
}
END_TEST

START_TEST(test_handle_error){
    CRASH_ON_ERRORS = 0;
    setLogLevel(LOG_LEVEL_NONE);
    xcb_input_key_press_event_t event = {0};
    setLastEvent(&event);
    applyRules(getEventRules(0), NULL);
}
END_TEST

START_TEST(test_detect_new_windows){
    WindowID win = createUnmappedWindow();
    WindowID win2 = createUnmappedWindow();
    START_MY_WM;
    WindowID win3 = createUnmappedWindow();
    WAIT_UNTIL_TRUE(isInList(getAllWindows(), win) &&
                    isInList(getAllWindows(), win2) &&
                    isInList(getAllWindows(), win3));
    assert(getSize(getActiveWindowStack()) == 3);
}
END_TEST
START_TEST(test_reparent_windows){
    IGNORE_SUBWINDOWS = 1;
    WindowID win = createNormalWindow();
    WindowID parent = createNormalWindow();
    int idle = getIdleCount();
    START_MY_WM;
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    assert(isInList(getAllWindows(), win) && isInList(getAllWindows(), parent));
    xcb_reparent_window_checked(dis, win, parent, 0, 0);
    idle = getIdleCount();
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    assert(!isInList(getAllWindows(), win) && isInList(getAllWindows(), parent));
    assert(getSize(getActiveWindowStack()) == 1);
    xcb_reparent_window_checked(dis, win, root, 0, 0);
    WAIT_UNTIL_TRUE(isInList(getAllWindows(), win) &&
                    isInList(getAllWindows(), parent));
    assert(getSize(getActiveWindowStack()) == 2);
}
END_TEST

START_TEST(test_delete_windows){
    CRASH_ON_ERRORS = 0;
    setLogLevel(LOG_LEVEL_NONE);
    WindowID win = createUnmappedWindow();
    WindowID win2 = createUnmappedWindow();
    WindowID win3 = createUnmappedWindow();
    destroyWindow(win);
    mapWindow(win2);
    assert(getSize(getAllWindows()) == 0);
    addUnknownWindowIgnoreRule();
    START_MY_WM;
    WindowID win4 = createUnmappedWindow();
    WAIT_UNTIL_TRUE(isInList(getAllWindows(), win4) &&
                    isInList(getAllWindows(), win2) &&
                    isInList(getAllWindows(), win3));
    WAIT_UNTIL_TRUE(!isInList(getAllWindows(), win));
    destroyWindow(win2);
    destroyWindow(win3);
    destroyWindow(win4);
    WAIT_UNTIL_TRUE(!(isInList(getAllWindows(), win2) ||
                      isInList(getAllWindows(), win3) ||
                      isInList(getAllWindows(), win4)));
    assert(getSize(getAllWindows()) == 0);
}
END_TEST

START_TEST(test_visibility_update){
    clearLayoutsOfWorkspace(getActiveWorkspaceIndex());
    START_MY_WM;
    lock();
    WindowID win = createNormalWindow();
    unlock();
    WAIT_UNTIL_TRUE(getWindowInfo(win));
    WAIT_UNTIL_TRUE(isWindowVisible(getWindowInfo(win)));
    lock();
    WindowID win2 = createNormalWindow();
    unlock();
    WAIT_UNTIL_TRUE(getWindowInfo(win2));
    WAIT_UNTIL_TRUE(isWindowVisible(getWindowInfo(win2)));
    assert(!isWindowVisible(getWindowInfo(win)));
    int workspace = getActiveWorkspaceIndex();
    switchToWorkspace(!workspace);
    WAIT_UNTIL_TRUE(!isWindowVisible(getWindowInfo(win2)));
    WAIT_UNTIL_TRUE(!isWindowVisible(getWindowInfo(win)));
    assert(!isWindowVisible(getWindowInfo(win2)));
    switchToWorkspace(workspace);
    WAIT_UNTIL_TRUE(isWindowVisible(getWindowInfo(win2)));
}
END_TEST

START_TEST(test_property_update){
    int idle = 0;
    START_MY_WM;
    WindowID win = mapWindow(createNormalWindow());
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    char* title = "dummy title";
    assert(!catchError(xcb_ewmh_set_wm_name_checked(ewmh, win, strlen(title), title)));
    WAIT_UNTIL_TRUE(getWindowInfo(win)->title);
    assert(strcmp(getWindowInfo(win)->title, title) == 0);
}
END_TEST

START_TEST(test_ignored_windows){
    addUnknownWindowIgnoreRule();
    createUserIgnoredWindow();
    mapWindow(createUserIgnoredWindow());
    scan(root);
    assert(getSize(getAllWindows()) == 0);
    consumeEvents();
    int idle = getIdleCount();
    START_MY_WM;
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    createUserIgnoredWindow();
    mapWindow(createUserIgnoredWindow());
    idle = getIdleCount();
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    assert(getSize(getAllWindows()) == 0);
}
END_TEST
START_TEST(test_unknown_windows_no_workspace){
    addUnknownWindowRule();
    createUserIgnoredWindow();
    mapWindow(createUserIgnoredWindow());
    scan(root);
    assert(getSize(getAllWindows()) == 2);
    assert(getSize(getActiveWindowStack()) == 0);
}
END_TEST

START_TEST(test_dock_rules){
    WindowID win = createDock(1, 1, 1);
    scan(root);
    assert(((WindowInfo*)getHead(getAllDocks()))->id == win);
    assert(hasMask(getHead(getAllDocks()), INPUT_MASK));
    removeWindow(win);
    assert(getSize(getAllDocks()) == 0);
    addNoDockFocusRule();
    scan(root);
    assert(((WindowInfo*)getHead(getAllDocks()))->id == win);
    assert(!hasMask(getHead(getAllDocks()), INPUT_MASK));
    Monitor* m = getMonitorFromWorkspace(getActiveWorkspace());
    assert(memcmp(&m->base, &m->view, sizeof(Rect)));
    consumeEvents();
    setDock(win, 0, 0, 0);
    flush();
    int idle = getIdleCount();
    START_MY_WM;
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    assert(memcmp(&m->base, &m->view, sizeof(Rect)) == 0);
}
END_TEST

START_TEST(test_desktop_rule){
    Monitor* m = getMonitorFromWorkspace(getActiveWorkspace());
    m->view.x = 10;
    m->view.y = 20;
    WindowID win = mapWindow(createNormalWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DESKTOP));
    setActiveLayout(&DEFAULT_LAYOUTS[GRID]);
    scan(root);
    WindowInfo* winInfo = getWindowInfo(win);
    retile();
    flush();
    xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, winInfo->id), NULL);
    assert(memcmp(&m->view.x, &m->base, sizeof(m->view)) != 0);
    assert(memcmp(&m->view.x, &reply->x, sizeof(m->view)) == 0);
    assert(hasMask(winInfo, STICKY_MASK | NO_TILE_MASK | BELOW_MASK));
    assert(!hasMask(winInfo, ABOVE_MASK));
    free(reply);
    setShowingDesktop(getActiveWorkspaceIndex(), 1);
    assert(!hasMask(winInfo, HIDDEN_MASK));
}
END_TEST
START_TEST(test_float_rule){
    WindowID win = createNormalWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DIALOG);
    for(int i = 0; i < 10; i++)
        createNormalWindow();
    setActiveLayout(&DEFAULT_LAYOUTS[FULL]);
    scan(root);
    WindowInfo* winInfo = getWindowInfo(win);
    assert(hasMask(winInfo, FLOATING_MASK));
}
END_TEST

START_TEST(test_always_on_top){
    //windows are in same spot
    int bottom = createNormalWindow();
    int top = createNormalWindow();
    scan(root);
    WindowInfo* info = getWindowInfo(bottom);
    WindowInfo* infoTop = getWindowInfo(top);
    addMask(infoTop, ALWAYS_ON_TOP);
    assert(info && infoTop);
    assert(raiseWindowInfo(info));
    flush();
    int idle = getIdleCount();
    START_MY_WM;
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    WindowID stackingOrder[] = {bottom, top};
    assert(checkStackingOrder(stackingOrder, 2));
}
END_TEST
START_TEST(test_map_windows){
    WindowID win = createUnmappedWindow();
    WindowID win2 = createUnmappedWindow();
    START_MY_WM;
    WindowID win3 = createUnmappedWindow();
    mapWindow(win3);
    mapWindow(win2);
    //wait for all to be in list
    WAIT_UNTIL_TRUE(isInList(getAllWindows(), win) &&
                    isInList(getAllWindows(), win2) &&
                    isInList(getAllWindows(), win3));
    //WAIT_UNTIL_TRUE(!isWindowMapped(win)&&isWindowMapped(win2)&&isWindowMapped(win3));
    mapWindow(win);
    //wait for all to be mapped
    WAIT_UNTIL_TRUE(isWindowMapped(win) && isWindowMapped(win2) && isWindowMapped(win3));
}
END_TEST
START_TEST(test_device_event){
    triggerBinding(&testBinding);
    flush();
    WAIT_UNTIL_TRUE(getDummyCount());
}
END_TEST
START_TEST(test_master_device_add_remove){
    int numMaster = getSize(getAllMasters());
    createMasterDevice("test1");
    createMasterDevice("test2");
    flush();
    WAIT_UNTIL_TRUE(getSize(getAllMasters()) == numMaster + 2);
    lock();
    destroyAllNonDefaultMasters();
    flush();
    unlock();
    WAIT_UNTIL_TRUE(getSize(getAllMasters()) == 1);
}
END_TEST
START_TEST(test_focus_update){
    assert(getFocusedWindow() != winInfo);
    assert(focusWindow(winInfo->id));
    flush();
    WAIT_UNTIL_TRUE(getFocusedWindow() == winInfo);
    assert(focusWindow(winInfo2->id));
    WAIT_UNTIL_TRUE(getFocusedWindow() == winInfo2);
}
END_TEST

START_TEST(test_focus_follows_mouse){
    int id1 = winInfo->id;
    int id2 = winInfo2->id;
    focusWindow(id1);
    movePointer(getActiveMasterPointerID(), id1, 0, 0);
    flush();
    WAIT_UNTIL_TRUE(getActiveFocus(getActiveMaster()->id) == id1);
    for(int i = 0; i < 4; i++){
        int id = (i % 2 ? id1 : id2);
        int n = 0;
        WAIT_UNTIL_TRUE(getActiveFocus(getActiveMaster()->id) == id,
                        setActiveMaster(defaultMaster);
                        movePointer(getActiveMasterPointerID(), id, n, n);
                        n = !n;
                        flush()
                       );
    }
}
END_TEST

static void clientSetup(){
    LOAD_SAVED_STATE = 0;
    CRASH_ON_ERRORS = 0;
    DEFAULT_WINDOW_MASKS = EXTERNAL_RESIZE_MASK | EXTERNAL_MOVE_MASK | EXTERNAL_BORDER_MASK;
    setLogLevel(LOG_LEVEL_NONE);
    onStartup();
    if(!fork()){
        ROOT_EVENT_MASKS = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
        openXDisplay();
        registerForEvents();
    }
    else {
        DEFAULT_WINDOW_MASKS |= EXTERNAL_MOVE_MASK;
        START_MY_WM;
        waitForCleanExit();
        fullCleanup();
        exit(0);
    }
}

START_TEST(test_map_request){
    for(int i = 0; i < 2; i++){
        mapWindow(i ? createUnmappedWindow() : createIgnoredWindow());
        flush();
        waitForNormalEvent(XCB_MAP_NOTIFY);
    }
}
END_TEST

START_TEST(test_configure_request){
    int values[] = {2, 1, 1000, 1000, 1, XCB_STACK_MODE_ABOVE};
    int mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
               XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
               XCB_CONFIG_WINDOW_BORDER_WIDTH | XCB_CONFIG_WINDOW_STACK_MODE;
    for(int i = 0; i < 2; i++){
        WindowID win = i ? createUnmappedWindow() : createIgnoredWindow();
        assert(!catchError(xcb_configure_window_checked(dis, win, mask, values)));
        waitForNormalEvent(XCB_CONFIGURE_NOTIFY);
    }
    consumeEvents();
    int allMasks = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
                   XCB_CONFIG_WINDOW_BORDER_WIDTH;
    int masks[] = {XCB_CONFIG_WINDOW_X, XCB_CONFIG_WINDOW_Y, XCB_CONFIG_WINDOW_WIDTH, XCB_CONFIG_WINDOW_HEIGHT,  XCB_CONFIG_WINDOW_BORDER_WIDTH};
    int defaultValues[] = {10, 10, 10, 10, 10};
    WindowID win = createUnmappedWindow();
    for(int i = 0; i < LEN(masks); i++){
        catchError(xcb_configure_window_checked(dis, win, allMasks, defaultValues));
        waitForNormalEvent(XCB_CONFIGURE_NOTIFY);
        catchError(xcb_configure_window_checked(dis, win, masks[i], values));
        waitForNormalEvent(XCB_CONFIGURE_NOTIFY);
        xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, win), NULL);
        for(int n = 0; n < 5; n++)
            assert((&reply->x)[n] == (n == i ? values[0] : defaultValues[0]));
        free(reply);
    }
}
END_TEST

START_TEST(test_client_activate_window){
    lock();
    assert(focusWindow(winInfo2->id));
    assert(focusWindow(winInfo->id));
    Master* mainMaster = getElement(getAllMasters(), 0);
    Master* other = getElement(getAllMasters(), 1);
    setActiveMaster(other);
    focusWindow(winInfo->id);
    setClientPointerForWindow(winInfo->id);
    setActiveMaster(mainMaster);
    int index = getActiveWorkspaceIndex();
    assert(index == 0);
    switchToWorkspace(!index);
    xcb_ewmh_request_change_active_window(ewmh, defaultScreenNumber, winInfo2->id, 1, 0, winInfo->id);
    flush();
    unlock();
    WAIT_UNTIL_TRUE(getActiveWorkspaceIndex() == index);
    WAIT_UNTIL_TRUE(getFocusedWindowByMaster(other) == winInfo2);
    assert(getFocusedWindowByMaster(mainMaster) == winInfo);
}
END_TEST

START_TEST(test_client_change_desktop){
    for(int i = 0; i < getNumberOfWorkspaces(); i++){
        xcb_ewmh_request_change_current_desktop(ewmh, defaultScreenNumber, i, 0);
        flush();
        WAIT_UNTIL_TRUE(getActiveWorkspaceIndex() == i);
    }
}
END_TEST
START_TEST(test_client_change_num_desktop){
    for(int n = 0; n < 2; n++){
        setActiveWorkspaceIndex(1);
        for(int i = 1; i < getNumberOfWorkspaces() * 2; i++){
            xcb_ewmh_request_change_number_of_desktops(ewmh, defaultScreenNumber, i);
            flush();
            WAIT_UNTIL_TRUE(getNumberOfWorkspaces() == i);
            assert(getActiveWorkspaceIndex() <= getNumberOfWorkspaces());
        }
    }
}
END_TEST

START_TEST(test_client_close_window){
    AUTO_FOCUS_NEW_WINDOW_TIMEOUT = -1;
    DEFAULT_WINDOW_MASKS |= SRC_ANY;
    setLogLevel(LOG_LEVEL_NONE);
    if(!fork()){
        openXDisplay();
        int id = _i ? createNormalWindow() : createIgnoredWindow();
        flush();
        registerForWindowEvents(id, XCB_EVENT_MASK_STRUCTURE_NOTIFY);
        xcb_ewmh_request_close_window(ewmh, defaultScreenNumber, id, 0, 0);
        flush();
        close(2);
        close(1);
        waitForNormalEvent(XCB_DESTROY_NOTIFY);
        closeConnection();
        exit(0);
    }
    else {
        START_MY_WM;
        assert(getExitStatusOfFork() == 1);
    }
}
END_TEST

START_TEST(test_client_set_sticky_window){
    mapArbitraryWindow();
    WAIT_UNTIL_TRUE(getSize(getAllWindows()));
    WindowInfo* winInfo = getHead(getAllWindows());
    xcb_ewmh_request_change_wm_desktop(ewmh, defaultScreenNumber, winInfo->id, -1, 0);
    flush();
    WAIT_UNTIL_TRUE(hasMask(winInfo, STICKY_MASK));
    assert(hasMask(winInfo, FLOATING_MASK));
}
END_TEST
START_TEST(test_client_set_window_workspace){
    int workspace = 3;
    xcb_ewmh_request_change_wm_desktop(ewmh, defaultScreenNumber, winInfo->id, workspace, 0);
    flush();
    WAIT_UNTIL_TRUE(isInList(getWindowStack(getWorkspaceByIndex(workspace)), winInfo->id));
}
END_TEST
START_TEST(test_client_set_window_state){
    SYNC_WINDOW_MASKS = 1;
    int states[] = {XCB_EWMH_WM_STATE_ADD, XCB_EWMH_WM_STATE_REMOVE, XCB_EWMH_WM_STATE_TOGGLE, XCB_EWMH_WM_STATE_TOGGLE};
    int ignoredWindow = createIgnoredWindow();
    for(int i = 0; i < LEN(states); i++){
        xcb_ewmh_request_change_wm_state(ewmh, defaultScreenNumber, ignoredWindow, states[i],
                                         ewmh->_NET_WM_STATE_MAXIMIZED_HORZ, ewmh->_NET_WM_STATE_MAXIMIZED_VERT, 0);
        xcb_ewmh_request_change_wm_state(ewmh, defaultScreenNumber, winInfo->id, states[i], ewmh->_NET_WM_STATE_MAXIMIZED_HORZ,
                                         ewmh->_NET_WM_STATE_MAXIMIZED_VERT, 0);
        flush();
        WindowInfo fakeWinInfo = {.mask = 0, .id = ignoredWindow};
        if(i % 2 == 0)
            WAIT_UNTIL_TRUE(hasMask(winInfo, X_MAXIMIZED_MASK | Y_MAXIMIZED_MASK));
        else
            WAIT_UNTIL_TRUE(!(hasMask(winInfo, X_MAXIMIZED_MASK) || hasMask(winInfo, Y_MAXIMIZED_MASK)));
        loadSavedAtomState(&fakeWinInfo);
        assert((hasMask(&fakeWinInfo, X_MAXIMIZED_MASK) && hasMask(&fakeWinInfo, Y_MAXIMIZED_MASK)) == (i % 2 == 0));
    }
}
END_TEST

START_TEST(test_auto_tile){
    Rule rule = {NULL, 0, PIPE(BIND(createNormalWindow), BIND(mapWindow))};
    volatile int count = 0;
    void fakeLayout(){
        count++;
    }
    Layout layout = {.layoutFunction = fakeLayout, .name = "Dummy"};
    void addFakeLayout(){
        assert(isNotEmpty(getEventRules(onXConnection)));
        assert(getNumberOfWorkspaces());
        for(int i = 0; i < getNumberOfWorkspaces(); i++){
            addLayoutsToWorkspace(i, &layout, 1);
            assert(getActiveLayoutOfWorkspace(i)->layoutFunction);
        }
    }
    prependToList(getEventRules(onXConnection), &rule);
    startUpMethod = addFakeLayout;
    onStartup();
    assert(getActiveLayoutOfWorkspace(0) == &layout);
    assert(getActiveLayoutOfWorkspace(0)->layoutFunction);
    assert(count);
    count = 0;
    createNormalWindow();
    START_MY_WM;
    WAIT_UNTIL_TRUE(count);
}
END_TEST
START_TEST(test_client_show_desktop){
    xcb_ewmh_request_change_showing_desktop(ewmh, defaultScreenNumber, 1);
    flush();
    WAIT_UNTIL_TRUE(isShowingDesktop(getActiveWorkspaceIndex()));
    xcb_ewmh_request_change_showing_desktop(ewmh, defaultScreenNumber, 0);
    flush();
    WAIT_UNTIL_TRUE(!isShowingDesktop(getActiveWorkspaceIndex()));
}
END_TEST

START_TEST(test_client_request_frame_extents){
    static Rule properyChangeDummyRule = CREATE_DEFAULT_EVENT_RULE(dummy);
    addToList(getEventRules(XCB_PROPERTY_NOTIFY), &properyChangeDummyRule);
    xcb_ewmh_request_frame_extents(ewmh, defaultScreenNumber, winInfo->id);
    flush();
    WAIT_UNTIL_TRUE(getDummyCount());
}
END_TEST

START_TEST(test_client_request_restack){
    WindowID stackingOrder[] = {winInfo->id, winInfo2->id, winInfo->id};
    int idle;
    lock();
    idle = getIdleCount();
    assert(raiseWindow(winInfo2->id));
    unlock();
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    assert(checkStackingOrder(stackingOrder, 2));
    addMask(winInfo, EXTERNAL_RAISE_MASK);
    //processConfigureRequest(winInfo->id, NULL, winInfo2->id, XCB_STACK_MODE_ABOVE,  XCB_CONFIG_WINDOW_STACK_MODE|XCB_CONFIG_WINDOW_SIBLING);
    lock();
    idle = getIdleCount();
    xcb_ewmh_request_restack_window(ewmh, defaultScreenNumber, winInfo->id, winInfo2->id, XCB_STACK_MODE_ABOVE);
    flush();
    unlock();
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    assert(checkStackingOrder(stackingOrder + 1, 2));
}
END_TEST


START_TEST(test_client_request_move_resize){
    short values[] = {1, 2, 3, 4};
    addMask(winInfo, EXTERNAL_MOVE_MASK | EXTERNAL_RESIZE_MASK);
    int flags = XCB_EWMH_MOVERESIZE_WINDOW_X | XCB_EWMH_MOVERESIZE_WINDOW_Y |
                XCB_EWMH_MOVERESIZE_WINDOW_WIDTH | XCB_EWMH_MOVERESIZE_WINDOW_HEIGHT;
    xcb_ewmh_request_moveresize_window(ewmh, defaultScreenNumber, winInfo->id,
                                       0, 2, flags, values[0], values[1], values[2], values[3]);
    flush();
    WAIT_UNTIL_TRUE(memcmp(values, winInfo->geometry, sizeof(short)*LEN(values)) == 0);
}
END_TEST
START_TEST(test_client_ping){
    WindowInfo* rootInfo = createWindowInfo(root);
    addWindowInfo(rootInfo);
    addMask(rootInfo, WM_PING_MASK);
    xcb_ewmh_send_wm_ping(ewmh, root, 0);
    flush();
    WAIT_UNTIL_TRUE(rootInfo->pingTimeStamp);
}
END_TEST

START_TEST(test_key_repeat){
    IGNORE_KEY_REPEAT = 1;
    xcb_input_key_press_event_t event = {.flags = XCB_INPUT_KEY_EVENT_FLAGS_KEY_REPEAT};
    setLastEvent(&event);
    onDeviceEvent();
    assert(getDummyCount() == 0);
}
END_TEST
START_TEST(test_monitor_deletion){
    POLL_COUNT = 0;
    Monitor* m = getHead(getAllMonitors());
    assert(m);
    START_MY_WM;
    createNormalWindow();
    assert(getMonitorFromWorkspace(getActiveWorkspace()) == m);
    WAIT_UNTIL_TRUE(doesWorkspaceHaveWindowsWithMask(getActiveWorkspaceIndex(), MAPPED_MASK));
    ATOMIC(removeMonitor(m->id); markState());
    assert(getMonitorFromWorkspace(getActiveWorkspace()) == NULL);
    //wake up event thread
    createNormalWindow();
    WAIT_UNTIL_TRUE(!doesWorkspaceHaveWindowsWithMask(getActiveWorkspaceIndex(), MAPPED_MASK));
}
END_TEST
START_TEST(test_workspace_deletion){
    assert(getNumberOfWorkspaces() > 3);
    activateWorkspace(1);
    for(int i = 0; i < 10; i++)
        processNewWindow(createWindowInfo(createNormalWindow()));
    flush();
    activateWorkspace(0);
    MONITOR_DUPLICATION_POLICY = 0;
    createMonitor((Rect){0, 0, 100, 100});
    detectMonitors();
    START_MY_WM;
    WAIT_UNTIL_TRUE(getWorkspaceByIndex(1)->mapped);
    lock();
    activateWorkspace(1);
    activateWorkspace(2);
    assert(isWorkspaceVisible(2));
    assert(!isWorkspaceVisible(1));
    assert(isWorkspaceVisible(0));
    xcb_ewmh_request_change_number_of_desktops(ewmh, defaultScreenNumber, 1);
    flush();
    unlock();
    WAIT_UNTIL_TRUE(getNumberOfWorkspaces() == 1);
    assert(isWorkspaceVisible(0));
    assert(getSize(getActiveWindowStack()) == getSize(getAllWindows()));
}
END_TEST
START_TEST(test_workspace_monitor_addition){
    MONITOR_DUPLICATION_POLICY = 0;
    removeWorkspaces(getNumberOfWorkspaces() - 1);
    createMonitor((Rect){0, 0, 100, 100});
    detectMonitors();
    assert(getNumberOfWorkspaces() < getSize(getAllMonitors()));
    for(int i = 0; i < getNumberOfWorkspaces(); i++)
        assert(isWorkspaceVisible(i));
    consumeEvents();
    xcb_ewmh_request_change_number_of_desktops(ewmh, defaultScreenNumber, getSize(getAllMonitors()));
    flush();
    START_MY_WM;
    WAIT_UNTIL_TRUE(getNumberOfWorkspaces() == getSize(getAllMonitors()));
    for(int i = 0; i < getNumberOfWorkspaces(); i++)
        assert(isWorkspaceVisible(i));
}
END_TEST

void fullMonitorCleanup(void){
    clearFakeMonitors();
    fullCleanup();
}

Suite* defaultRulesSuite(){
    Suite* s = suite_create("Default events");
    TCase* tc_core;
    tc_core = tcase_create("Startup Events");
    tcase_add_checked_fixture(tc_core, NULL, fullCleanup);
    tcase_add_test(tc_core, test_auto_tile);
    tcase_add_test(tc_core, test_run_as_non_wm);
    tcase_add_test(tc_core, test_on_startup);
    tcase_add_test(tc_core, test_post_startup);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Special Events");
    tcase_add_checked_fixture(tc_core, onStartup, fullCleanup);
    tcase_add_test(tc_core, test_print_method);
    tcase_add_test(tc_core, test_handle_error);
    tcase_add_test(tc_core, test_detect_wm);
    tcase_add_test(tc_core, test_die_on_idle);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("NormalEvents");
    tcase_add_checked_fixture(tc_core, onStartup, fullCleanup);
    tcase_add_test(tc_core, test_detect_new_windows);
    tcase_add_test(tc_core, test_reparent_windows);
    tcase_add_test(tc_core, test_map_windows);
    tcase_add_test(tc_core, test_delete_windows);
    tcase_add_test(tc_core, test_visibility_update);
    tcase_add_test(tc_core, test_property_update);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("DefaultRules");
    tcase_add_checked_fixture(tc_core, onStartup, fullCleanup);
    tcase_add_test(tc_core, test_ignored_windows);
    tcase_add_test(tc_core, test_unknown_windows_no_workspace);
    tcase_add_test(tc_core, test_always_on_top);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("ConvienceRules");
    tcase_add_checked_fixture(tc_core, onStartup, fullCleanup);
    tcase_add_test(tc_core, test_dock_rules);
    tcase_add_test(tc_core, test_desktop_rule);
    tcase_add_test(tc_core, test_float_rule);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Requests");
    tcase_add_checked_fixture(tc_core, clientSetup, fullCleanup);
    tcase_add_test(tc_core, test_map_request);
    tcase_add_test(tc_core, test_configure_request);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Default Device Events");
    tcase_add_checked_fixture(tc_core, deviceEventsetup, fullCleanup);
    tcase_add_test(tc_core, test_master_device_add_remove);
    tcase_add_test(tc_core, test_device_event);
    tcase_add_test(tc_core, test_focus_update);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Non_Default_Device_Events");
    tcase_add_checked_fixture(tc_core, nonDefaultDeviceEventsetup, fullCleanup);
    tcase_add_test(tc_core, test_focus_follows_mouse);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("KeyEvent");
    tcase_add_checked_fixture(tc_core, deviceEventsetup, fullCleanup);
    tcase_add_test(tc_core, test_key_repeat);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Client_Messages");
    tcase_add_checked_fixture(tc_core, deviceEventsetup, fullCleanup);
    tcase_add_test(tc_core, test_client_activate_window);
    tcase_add_test(tc_core, test_client_change_desktop);
    tcase_add_test(tc_core, test_client_change_num_desktop);
    tcase_add_test(tc_core, test_client_set_window_workspace);
    tcase_add_test(tc_core, test_client_set_sticky_window);
    tcase_add_test(tc_core, test_client_set_window_state);
    tcase_add_test(tc_core, test_client_show_desktop);
    tcase_add_test(tc_core, test_client_request_frame_extents);
    tcase_add_test(tc_core, test_client_request_restack);
    tcase_add_test(tc_core, test_client_request_move_resize);
    tcase_add_test(tc_core, test_client_ping);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Special Client Messages");
    tcase_add_checked_fixture(tc_core, onStartup, fullCleanup);
    tcase_add_loop_test(tc_core, test_client_close_window, 0, 2);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("RemoveMonitor");
    tcase_add_checked_fixture(tc_core, onStartup, fullMonitorCleanup);
    tcase_add_test(tc_core, test_monitor_deletion);
    tcase_add_test(tc_core, test_workspace_deletion);
    tcase_add_test(tc_core, test_workspace_monitor_addition);
    suite_add_tcase(s, tc_core);
    return s;
}
