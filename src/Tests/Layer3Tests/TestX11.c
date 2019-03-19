
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>

#include "../../devices.h"
#include "../../default-rules.h"
#include "../../mywm-util.h"
#include "../../logger.h"
#include "../../events.h"
#include "../../wmfunctions.h"
#include "../../functions.h"
#include "../../ewmh.h"
#include "../../globals.h"
#include "../../layouts.h"

#include "../UnitTests.h"
#include "../TestX11Helper.h"




START_TEST(test_sync_state){
    WindowID win = mapArbitraryWindow();
    int activeWorkspace = 1;
    WindowID windowWorkspace = 3;
    LOAD_SAVED_STATE = 0;
    DEFAULT_WORKSPACE_INDEX = 2;
    syncState();
    assert(getActiveWorkspaceIndex() == DEFAULT_WORKSPACE_INDEX);
    unsigned int currentWorkspace = DEFAULT_WORKSPACE_INDEX;
    assert(xcb_ewmh_get_current_desktop_reply(ewmh,
            xcb_ewmh_get_current_desktop(ewmh, defaultScreenNumber),
            &currentWorkspace, NULL));
    assert(getActiveWorkspaceIndex() == currentWorkspace);
    assert(xcb_request_check(dis, xcb_ewmh_set_current_desktop_checked(ewmh, defaultScreenNumber,
                             activeWorkspace)) == NULL);
    assert(xcb_request_check(dis, xcb_ewmh_set_wm_desktop(ewmh, win, windowWorkspace)) == NULL);
    LOAD_SAVED_STATE = 1;
    syncState();
    scan(root);
    assert(isInList(getAllWindows(), win));
    assert(getActiveWorkspaceIndex() == activeWorkspace);
    assert(!isInList(getActiveWindowStack(), win));
    assert(getSize(getWindowStack(getWorkspaceByIndex(windowWorkspace))) == 1);
}
END_TEST


START_TEST(test_focus_window){
    WindowID win = mapArbitraryWindow();
    scan(root);
    focusWindow(win);
    xcb_input_xi_get_focus_reply_t* reply = xcb_input_xi_get_focus_reply(dis,
                                            xcb_input_xi_get_focus(dis, getActiveMasterKeyboardID()), NULL);
    assert(reply->focus == win);
    free(reply);
}
END_TEST

START_TEST(test_focus_window_request){
    //TODO update when the X11 request focus methods supports multi focus
    WindowID win = mapArbitraryWindow();
    scan(root);
    focusWindowInfo(getWindowInfo(win));
    xcb_input_xi_get_focus_reply_t* reply = xcb_input_xi_get_focus_reply(dis,
                                            xcb_input_xi_get_focus(dis, getActiveMasterKeyboardID()), NULL);
    assert(reply->focus == win);
    free(reply);
}
END_TEST

START_TEST(test_activate_window){
    WindowID win = createUnmappedWindow();
    WindowID win2 = createNormalWindow();
    scan(root);
    assert(!activateWindow(getWindowInfo(win)));
    assert(activateWindow(getWindowInfo(win2)));
    removeMask(getWindowInfo(win2), ALL_MASK);
    assert(!activateWindow(getWindowInfo(win2)));
}
END_TEST
START_TEST(test_delete_window_request){
    KILL_TIMEOUT = _i == 0 ? 0 : 100;
    int fd[2];
    pipe(fd);
    if(!fork()){
        openXDisplay();
        WindowID win = mapArbitraryWindow();
        xcb_atom_t atoms[] = {WM_DELETE_WINDOW, ewmh->_NET_WM_PING};
        xcb_icccm_set_wm_protocols(dis, win, ewmh->WM_PROTOCOLS, _i == 0 ? 2 : 1, atoms);
        consumeEvents();
        write(fd[1], &win, sizeof(int));
        close(fd[1]);
        close(fd[0]);
        flush();
        close(1);
        close(2);
        if(_i){
            msleep(KILL_TIMEOUT * 10);
        }
        if(_i == 0){
            /// wait for client message
            waitForNormalEvent(XCB_CLIENT_MESSAGE);
            exit(0);
        }
        flush();
        assert(!xcb_connection_has_error(dis));
        if(checkStackingOrder(&win, 1) == 0)
            exit(0);
        WAIT_UNTIL_TRUE(0);
    }
    WindowID win;
    read(fd[0], &win, sizeof(int));
    close(fd[0]);
    close(fd[1]);
    scan(root);
    WindowInfo* winInfo = getWindowInfo(win);
    assert(hasMask(winInfo, WM_DELETE_WINDOW_MASK));
    consumeEvents();
    START_MY_WM
    lock();
    if(_i != 1){
        if(_i == 2){
            WindowInfo dummy = {.id = 1};
            addMask(&dummy, WM_DELETE_WINDOW_MASK);
            killWindowInfo(&dummy);
            msleep(KILL_TIMEOUT * 2);
        }
        killWindowInfo(winInfo);
        flush();
    }
    else {
        removeWindow(win);
        killWindow(win);
    }
    unlock();
    wait(NULL);
}
END_TEST

START_TEST(test_set_border_color){
    WindowID win = mapWindow(createNormalWindow());
    scan(root);
    WindowInfo* winInfo = isInList(getAllWindows(), win);
    unsigned int colors[] = {0, 255, 255 * 255, 255 * 255 * 255};
    for(int i = 0; i < LEN(colors); i++){
        assert(setBorderColor(win, colors[i]));
        flush();
    }
    setBorderColor(win, -1);
    assert(setBorder(winInfo));
    resetBorder(winInfo);
    //TODO check to see if border was set correctly
}
END_TEST

START_TEST(test_invalid_state){
    WindowID win = mapArbitraryWindow();
    xcb_ewmh_set_wm_desktop(ewmh, win, getNumberOfWorkspaces() + 1);
    xcb_ewmh_set_current_desktop(ewmh, defaultScreenNumber, getNumberOfWorkspaces() + 1);
    flush();
    scan(root);
    syncState();
    assert(getActiveWorkspaceIndex() < getNumberOfWorkspaces());
    assert(getWindowInfo(win)->workspaceIndex < getNumberOfWorkspaces());
}
END_TEST

START_TEST(test_withdraw_window){
    WindowID win = createNormalWindow();
    WAIT_UNTIL_TRUE(getWindowInfo(win))
    WindowInfo* winInfo = getWindowInfo(win);
    IGNORE_SEND_EVENT = 0;
    WAIT_UNTIL_TRUE(hasMask(winInfo, MAPPABLE_MASK));
    xcb_unmap_notify_event_t event = {.response_type = XCB_UNMAP_NOTIFY, .event = root, .window = win};
    catchError(xcb_send_event_checked(dis, 0, root, ROOT_EVENT_MASKS, (char*) &event));
    WAIT_UNTIL_FALSE(hasMask(winInfo, MAPPABLE_MASK));
}
END_TEST
START_TEST(test_raise_window){
    //windows are in same spot
    int bottom = mapArbitraryWindow();
    int top = mapArbitraryWindow();
    registerForWindowEvents(bottom, XCB_EVENT_MASK_VISIBILITY_CHANGE);
    registerForWindowEvents(top, XCB_EVENT_MASK_VISIBILITY_CHANGE);
    scan(root);
    WindowInfo* info = getWindowInfo(bottom);
    WindowInfo* infoTop = getWindowInfo(top);
    assert(info && infoTop);
    assert(raiseWindow(bottom));
    flush();
    WindowID stackingOrder[] = {top, bottom, top};
    assert(checkStackingOrder(stackingOrder, 2));
    assert(raiseWindowInfo(infoTop));
    assert(checkStackingOrder(stackingOrder + 1, 2));
}
END_TEST
START_TEST(test_raise_window_special){
    //windows are in same spot
    int bottom = createNormalWindow();
    int top = createNormalWindow();
    registerForWindowEvents(bottom, XCB_EVENT_MASK_VISIBILITY_CHANGE);
    registerForWindowEvents(top, XCB_EVENT_MASK_VISIBILITY_CHANGE);
    scan(root);
    WindowInfo* info = getWindowInfo(bottom);
    WindowInfo* infoTop = getWindowInfo(top);
    addMask(infoTop, ALWAYS_ON_TOP);
    assert(info && infoTop);
    assert(raiseWindowInfo(info));
    flush();
    WindowID stackingOrder[] = {bottom, top};
    assert(checkStackingOrder(stackingOrder, 2));
    assert(raiseWindowInfo(infoTop));
    assert(checkStackingOrder(stackingOrder, 2));
}
END_TEST
START_TEST(test_sticky_window_add){
    LOAD_SAVED_STATE = 0;
    mapArbitraryWindow();
    WindowID win = mapArbitraryWindow();
    scan(root);
    retile();
    WindowInfo* winInfo = getWindowInfo(win);
    moveWindowToWorkspace(winInfo, -1);
    assert(hasMask(winInfo, STICKY_MASK));
}
END_TEST

START_TEST(test_window_swap){
    WindowID win[4];
    for(int i = 0; i < LEN(win); i++)win[i] = mapArbitraryWindow();
    scan(root);
    retile();
    int nonActiveWorkspace = !getActiveWorkspaceIndex();
    WindowInfo* winInfo = getWindowInfo(win[0]);
    moveWindowToWorkspace(winInfo, nonActiveWorkspace);
    swapWindows(getElement(getActiveWindowStack(), 0), winInfo);
    assert(winInfo->workspaceIndex == getActiveWorkspaceIndex());
    assert(getHead(getActiveWindowStack()) == winInfo);
    swapWindows(getElement(getActiveWindowStack(), 1), winInfo);
    assert(getElement(getActiveWindowStack(), 1) == winInfo);
    swapWindows(getElement(getActiveWindowStack(), 0), winInfo);
    assert(getElement(getActiveWindowStack(), 0) == winInfo);
}
END_TEST

START_TEST(test_sticky_workspace_change){
    LOAD_SAVED_STATE = 0;
    mapArbitraryWindow();
    WindowID win = mapArbitraryWindow();
    scan(root);
    retile();
    WindowInfo* winInfo = getWindowInfo(win);
    addMask(winInfo, STICKY_MASK);
    for(int i = 0; i < getNumberOfWorkspaces(); i++){
        switchToWorkspace(i);
        assert(find(getActiveWindowStack(), winInfo, sizeof(WindowInfo*)));
        assert(getWorkspaceIndexOfWindow(winInfo) == i);
    }
}
END_TEST

START_TEST(test_workspace_change){
    int nonEmptyIndex = getActiveWorkspaceIndex();
    mapArbitraryWindow();
    mapArbitraryWindow();
    scan(root);
    assert(getSize(getAllWindows()) == 2);
    assert(getSize(getActiveWindowStack()) == 2);
    assert(getNumberOfWorkspaces() > 1);
    for(int i = 0; i < getNumberOfWorkspaces(); i++){
        switchToWorkspace(i);
        if(i == nonEmptyIndex)continue;
        assert(getActiveWorkspace()->id == i);
        assert(getSize(getActiveWindowStack()) == 0);
    }
    switchToWorkspace(nonEmptyIndex);
    assert(getSize(getActiveWindowStack()) == 2);
    moveWindowToWorkspace(getHead(getActiveWindowStack()), !nonEmptyIndex);
    assert(getSize(getActiveWindowStack()) == 1);
    assert(getSize(getWindowStack(getWorkspaceByIndex(!nonEmptyIndex))) == 1);
}
END_TEST

START_TEST(test_workspace_activation){
    LOAD_SAVED_STATE = 0;
    mapArbitraryWindow();
    mapArbitraryWindow();
    scan(root);
    int index1 = getActiveWorkspaceIndex();
    int index2 = !index1;
    moveWindowToWorkspace(getHead(getActiveWindowStack()), index2);
    WindowInfo* winInfo1 = getHead(getWindowStack(getWorkspaceByIndex(index1)));
    WindowInfo* winInfo2 = getHead(getWindowStack(getWorkspaceByIndex(index2)));
    int info1 = winInfo1->id;
    int info2 = winInfo2->id;
    assert(getSize(getAllMonitors()) == 1);
    switchToWorkspace(index2);
    assert(isWorkspaceVisible(index2));
    assert(!isWorkspaceVisible(index1));
    WAIT_UNTIL_TRUE(isWindowMapped(info2) && !isWindowMapped(info1) && getActiveWorkspaceIndex() == index2);
    switchToWorkspace(index1);
    assert(isWorkspaceVisible(index1));
    assert(!isWorkspaceVisible(index2));
    WAIT_UNTIL_TRUE(isWindowMapped(info1) && !isWindowMapped(info2) && getActiveWorkspaceIndex() == index1);
    activateWindow(winInfo2);
    assert(isWindowNotInInvisibleWorkspace(winInfo2));
    assert(!isWindowNotInInvisibleWorkspace(winInfo1));
    WAIT_UNTIL_TRUE(isWindowMapped(info2) && !isWindowMapped(info1) && getActiveWorkspaceIndex() == index2);
    assert(getHead(getActiveWindowStack()) == winInfo2);
    activateWindow(winInfo1);
    assert(isWindowNotInInvisibleWorkspace(winInfo1));
    assert(!isWindowNotInInvisibleWorkspace(winInfo2));
    WAIT_UNTIL_TRUE(isWindowMapped(info1) && !isWindowMapped(info2) && getActiveWorkspaceIndex() == index1);
    assert(getSize(getActiveWindowStack()) == 1);
    assert(getHead(getActiveWindowStack()) == winInfo1);
}
END_TEST



START_TEST(test_configure_windows){
    WindowInfo* winInfo[] = {createWindowInfo(mapWindow(createNormalWindow())), createWindowInfo(mapWindow(createNormalWindow()))};
    markAsDock(winInfo[0]);
    DEFAULT_WINDOW_MASKS = EXTERNAL_RESIZE_MASK | EXTERNAL_MOVE_MASK | EXTERNAL_BORDER_MASK;
    for(int n = 0; n < LEN(winInfo); n++){
        processNewWindow(winInfo[n]);
        WindowID win = winInfo[n]->id;
        short values[] = {1, 2, 3, 4, 5};
        int allMasks = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
                       XCB_CONFIG_WINDOW_BORDER_WIDTH;
        int masks[] = {XCB_CONFIG_WINDOW_X, XCB_CONFIG_WINDOW_Y, XCB_CONFIG_WINDOW_WIDTH, XCB_CONFIG_WINDOW_HEIGHT,  XCB_CONFIG_WINDOW_BORDER_WIDTH};
        int defaultValues[] = {10, 10, 10, 10, 10};
        for(int i = 0; i < LEN(masks); i++){
            xcb_configure_window(dis, win, allMasks, defaultValues);
            processConfigureRequest(win, values, 0, 0, masks[i]);
            waitForNormalEvent(XCB_CONFIGURE_NOTIFY);
            xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, win), NULL);
            for(int n = 0; n < 5; n++)
                assert((&reply->x)[n] == (n == i ? values[0] : defaultValues[0]));
            free(reply);
        }
        xcb_configure_window(dis, win, allMasks, defaultValues);
        processConfigureRequest(win, values, 0, 0, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT);
        xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, win), NULL);
        assert(reply->width == values[0]);
        assert(reply->height == values[1]);
        free(reply);
        //TODO check below
        processConfigureRequest(win, NULL, root, XCB_STACK_MODE_ABOVE,
                                XCB_CONFIG_WINDOW_STACK_MODE | XCB_CONFIG_WINDOW_SIBLING);
    }
}
END_TEST

START_TEST(test_float_sink_window){
    WindowID win = mapWindow(createNormalWindow());
    WindowInfo* winInfo;
    START_MY_WM
    WAIT_UNTIL_TRUE(winInfo = getWindowInfo(win))
    WAIT_UNTIL_TRUE(isTileable(winInfo));
    lock();
    floatWindow(winInfo);
    unlock();
    assert(!isTileable(winInfo));
    assert(hasMask(winInfo, FLOATING_MASK));
    lock();
    sinkWindow(winInfo);
    unlock();
    assert(!hasMask(winInfo, FLOATING_MASK));
    WAIT_UNTIL_TRUE(isTileable(winInfo));
}
END_TEST


START_TEST(test_apply_gravity){
    WindowID win = createNormalWindow();
    xcb_size_hints_t hints;
    xcb_icccm_size_hints_set_win_gravity(&hints, XCB_GRAVITY_CENTER);
    xcb_icccm_set_wm_size_hints(dis, win, XCB_ATOM_WM_NORMAL_HINTS, &hints);
    for(int i = 0; i <= XCB_GRAVITY_STATIC; i++){
        short values[5] = {0, 0, 10, 10, 1};
        applyGravity(win, values, i);
        assert(values[0] || values[1] || i == XCB_GRAVITY_STATIC || !i);
    }
}
END_TEST
START_TEST(test_auto_focus){
    AUTO_FOCUS_NEW_WINDOW_TIMEOUT = 1000;
    int focusHolder = mapWindow(createNormalWindow());
    focusWindow(focusHolder);
    WindowInfo* winInfo = createWindowInfo(mapWindow(createNormalWindow()));
    WindowInfo* winInfo2 = createWindowInfo(mapWindow(createNormalWindow()));
    WindowInfo* winInfo3 = createWindowInfo(mapWindow(createNormalWindow()));
    winInfo->creationTime = getTime();
    winInfo2->creationTime = getTime();
    winInfo3->creationTime = getTime();
    processNewWindow(winInfo);
    assert(focusHolder == getActiveFocus(getActiveMasterKeyboardID()));
    updateMapState(winInfo->id, 1);
    assert(winInfo->id == getActiveFocus(getActiveMasterKeyboardID()));
    focusWindow(focusHolder);
    AUTO_FOCUS_NEW_WINDOW_TIMEOUT = -1;
    processNewWindow(winInfo2);
    assert(focusHolder == getActiveFocus(getActiveMasterKeyboardID()));
    updateMapState(winInfo2->id, 1);
    assert(focusHolder == getActiveFocus(getActiveMasterKeyboardID()));
    int delay = 25;
    AUTO_FOCUS_NEW_WINDOW_TIMEOUT = delay;
    msleep(delay * 2);
    processNewWindow(winInfo3);
    updateMapState(winInfo3->id, 1);
    assert(focusHolder == getActiveFocus(getActiveMasterKeyboardID()));
}
END_TEST

START_TEST(test_auto_focus_delete){
    AUTO_FOCUS_NEW_WINDOW_TIMEOUT = 10000;
    setActiveLayout(&DEFAULT_LAYOUTS[FULL]);
    START_MY_WM
    for(int i = 0; i < 3; i++)
        mapWindow(createNormalWindow());
    WAIT_UNTIL_TRUE(getSize(getActiveMasterWindowStack()) == 3);
    assert(getSize(getAllWindows()) == 3);
    assert(getSize(getActiveWindowStack()) == 3);
    lock();
    retile();
    WindowInfo* head = getHead(getActiveWindowStack());
    WindowInfo* middle = getElement(getActiveWindowStack(), 1);
    WindowInfo* lastFocused = getHead(getActiveMasterWindowStack());
    assert(lastFocused == getLast(getActiveWindowStack()));
    WindowID stackingOrder[] = {head->id, middle->id, lastFocused->id};
    assert(checkStackingOrder(stackingOrder, 3));
    int idle = getIdleCount();
    assert(catchError(xcb_destroy_window_checked(dis, lastFocused->id)) == 0);
    unlock();
    WAIT_UNTIL_TRUE(idle != getIdleCount())
    assert(getSize(getAllWindows()) == 2);
    assert(checkStackingOrder(stackingOrder, 2));
    assert(middle->id == getActiveFocus(getActiveMasterKeyboardID()));
}
END_TEST


START_TEST(test_set_workspace_names){
    char* name[] = {"test", "test2"};
    assert((unsigned int)getIndexFromName(name[0]) >= LEN(name));
    setWorkspaceNames(name, 1);
    assert(getIndexFromName(name[0]) == 0);
    assert(strcmp(name[0], getWorkspaceName(0)) == 0);
    setWorkspaceNames(name, 2);
    assert(getIndexFromName(name[1]) == 1);
    assert(strcmp(name[1], getWorkspaceName(1)) == 0);
}
END_TEST
START_TEST(test_unkown_window){
    WindowID win = createNormalWindow();
    WindowInfo* winInfo = createWindowInfo(win);
    mapWindow(win);
    assert(focusWindow(win));
    assert(raiseWindow(win));
    setWindowStateFromAtomInfo(winInfo, NULL, 0, 0);
    registerWindow(winInfo);
    updateMapState(win, 0);
    updateMapState(win, 1);
    updateMapState(win, 0);
}
END_TEST
START_TEST(test_bad_window){
    setLogLevel(LOG_LEVEL_NONE);
    CRASH_ON_ERRORS = 0;
    WindowID win = -2;
    assert(!focusWindow(win));
    assert(!raiseWindow(win));
    short arr[5];
    applyGravity(win, arr, 0);
}
END_TEST

START_TEST(test_kill_window){
    WindowID win = createNormalWindow();
    if(!fork()){
        openXDisplay();
        killWindow(win);
        flush();
        closeConnection();
    }
    wait(NULL);
    exit(0);
}
END_TEST

static void setup(){
    onStartup();
    START_MY_WM
}
START_TEST(test_connect_to_xserver){
    connectToXserver();
    assert(getActiveMaster() != NULL);
    assert(getActiveWorkspace() != NULL);
    assert(getSize(getActiveMasterWindowStack()) == 0);
    assert(getSize(getAllWindows()) == 0);
    assert(getActiveMaster());
    Monitor* m = getHead(getAllMonitors());
    assert(m != NULL);
    assert(getMonitorFromWorkspace(getActiveWorkspace()));
}
END_TEST

START_TEST(test_activate_workspace){
    int masterStackSize = getSize(getWindowStackByMaster(getActiveMaster()));
    int idle;
    for(int i = 0; i < 2; i++){
        lock();
        activateWorkspace(i);
        assert(i == getActiveWorkspaceIndex());
        idle = getIdleCount();
        unlock();
        WAIT_UNTIL_TRUE(idle != getIdleCount());
        assert(getSize(getWindowStackByMaster(getActiveMaster())) == masterStackSize);
    }
    lock();
    activateWorkspace(2);
    assert(2 == getActiveWorkspaceIndex());
    idle = getIdleCount();
    unlock();
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    assert(getSize(getWindowStackByMaster(getActiveMaster())) == masterStackSize + 1);
}
END_TEST

START_TEST(test_swap_workspace){
    int index = getActiveWorkspaceIndex();
    swapWithWorkspace(!index);
    assert(getActiveWorkspaceIndex() == index);
}
END_TEST

void multiWorkspaceStartup(void){
    CRASH_ON_ERRORS = -1;
    AUTO_FOCUS_NEW_WINDOW_TIMEOUT = -1;
    onStartup();
    switchToWorkspace(0);
    START_MY_WM;
    createNormalWindow();
    createNormalWindow();
    WAIT_UNTIL_TRUE(getSize(getAllWindows()) == 2)
    lock();
    focusWindowInfo(getLast(getActiveWindowStack()));
    switchToWorkspace(1);
    createNormalWindow();
    createNormalWindow();
    unlock();
    WAIT_UNTIL_TRUE(getSize(getAllWindows()) == 4)
    lock();
    focusWindowInfo(getHead(getActiveWindowStack()));
    switchToWorkspace(2);
    createNormalWindow();
    unlock();
    WAIT_UNTIL_TRUE(getSize(getAllWindows()) == 5)
    assert(getSize(getWindowStackByMaster(getActiveMaster())) == 2);
}

START_TEST(test_workspaceless_window){
    CRASH_ON_ERRORS = 0;
    setLogLevel(LOG_LEVEL_NONE);
    WindowInfo* winInfo = createWindowInfo(mapWindow(createNormalWindow()));
    addWindowInfo(winInfo);
    int index = getActiveWorkspaceIndex();
    START_MY_WM;
    int size = 6;
    for(int i = 0; i < size; i++){
        if(i == size / 2){
            WAIT_UNTIL_TRUE(getSize(getActiveWindowStack()) == size / 2);
            ATOMIC(switchToWorkspace(!index));
        }
        ATOMIC(createNormalWindow());
    }
    WAIT_UNTIL_TRUE(getSize(getActiveWindowStack()) == size / 2);
    assert(getSize(getAllWindows()) == size + 1);
    ATOMIC(assert(getWorkspaceIndexOfWindow(winInfo) == NO_WORKSPACE));
    ATOMIC(assert(getWorkspaceOfWindow(winInfo) == NULL));
    activateWindow(winInfo);
    ATOMIC(assert(getWorkspaceIndexOfWindow(winInfo) == NO_WORKSPACE));
    ATOMIC(assert(getWorkspaceOfWindow(winInfo) == NULL));
    assert(getActiveWorkspaceIndex() == !index);
    ATOMIC(activateWorkspace(index));
    ATOMIC(activateWorkspace(!index));
    ATOMIC(switchToWorkspace(index));
    ATOMIC(switchToWorkspace(!index));
    ATOMIC(switchToWorkspace(index));
    ATOMIC(swapWorkspaces(index, !index));
    ATOMIC(swapWorkspaces(index, index));
    ATOMIC(swapWithWorkspace(index));
    ATOMIC(swapWithWorkspace(!index));
}
END_TEST
Suite* x11Suite(void){
    Suite* s = suite_create("Window Manager Functions");
    TCase* tc_core;
    tc_core = tcase_create("X Server");
    tcase_add_checked_fixture(tc_core, createSimpleContext, resetContext);
    tcase_add_test(tc_core, test_connect_to_xserver);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Sync_State");
    tcase_add_checked_fixture(tc_core, onStartup, destroyContextAndConnection);
    tcase_add_test(tc_core, test_sync_state);
    tcase_add_test(tc_core, test_invalid_state);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("WindowOperations");
    tcase_add_checked_fixture(tc_core, onStartup, fullCleanup);
    tcase_add_test(tc_core, test_raise_window);
    tcase_add_test(tc_core, test_raise_window_special);
    tcase_add_test(tc_core, test_focus_window);
    tcase_add_test(tc_core, test_focus_window_request);
    tcase_add_test(tc_core, test_auto_focus);
    tcase_add_test(tc_core, test_auto_focus_delete);
    tcase_add_loop_test(tc_core, test_delete_window_request, 0, 3);
    tcase_add_test(tc_core, test_activate_window);
    tcase_add_test(tc_core, test_set_border_color);
    tcase_add_test(tc_core, test_workspace_activation);
    tcase_add_test(tc_core, test_workspace_change);
    tcase_add_test(tc_core, test_sticky_workspace_change);
    tcase_add_test(tc_core, test_sticky_window_add);
    tcase_add_test(tc_core, test_window_swap);
    tcase_add_test(tc_core, test_kill_window);
    tcase_add_test(tc_core, test_configure_windows);
    tcase_add_test(tc_core, test_float_sink_window);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("WorkspaceOperations");
    tcase_add_checked_fixture(tc_core, multiWorkspaceStartup, fullCleanup);
    tcase_add_test(tc_core, test_activate_workspace);
    tcase_add_test(tc_core, test_swap_workspace);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Window Managment Operations");
    tcase_add_checked_fixture(tc_core, setup, fullCleanup);
    tcase_add_test(tc_core, test_apply_gravity);
    tcase_add_test(tc_core, test_withdraw_window);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("MISC");
    tcase_add_checked_fixture(tc_core, onStartup, fullCleanup);
    tcase_add_test(tc_core, test_set_workspace_names);
    tcase_add_test(tc_core, test_unkown_window);
    tcase_add_test(tc_core, test_bad_window);
    tcase_add_test(tc_core, test_workspaceless_window);
    suite_add_tcase(s, tc_core);
    return s;
}
