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
#include "../../layouts.h"
#include "../../wmfunctions.h"

static int calledMasks;
static int visited;
static void onStateChange(int i){
    int mask = 1 << i;
    calledMasks |= mask;
    visited++;
}
static WindowInfo* addVisibleWindow(int i){
    WindowInfo* winInfo = createWindowInfo(createNormalWindow());
    addWindowInfo(winInfo);
    addWindowToWorkspace(winInfo, i);
    addMask(winInfo, MAPPED_MASK);
    return winInfo;
}
START_TEST(test_no_state_change){
    markState();
    assert(updateState(NULL, NULL) == WORKSPACE_MONITOR_CHANGE);
    assert(!updateState(NULL, NULL));
    markState();
    assert(!updateState(NULL, NULL));
}
END_TEST
START_TEST(test_state_change_num_windows){
    for(int i = 1; i < 10; i++){
        WindowInfo* winInfo = createWindowInfo(i);
        addWindowInfo(winInfo);
        addMask(winInfo, MAPPED_MASK);
        assert(isWorkspaceVisible(0));
        addWindowToWorkspace(winInfo, 0);
        markState();
        assert(updateState(NULL, NULL));
        break;
    }
}
END_TEST
START_TEST(test_mask_change){
    WindowInfo* winInfo = createWindowInfo(createNormalWindow());
    addWindowInfo(winInfo);
    addWindowToWorkspace(winInfo, getActiveWorkspaceIndex());
    addMask(winInfo, MAPPED_MASK);
    assert(!hasMask(winInfo, 1));
    assert(updateState(NULL, NULL));
    addMask(winInfo, 1);
    markState();
    assert(updateState(NULL, NULL));
}
END_TEST
START_TEST(test_layout_change){
    addVisibleWindow(getActiveWorkspaceIndex());
    updateState(NULL, NULL);
    Layout l = {0};
    setActiveLayout(&l);
    markState();
    assert(updateState(NULL, NULL));
    setActiveLayout(NULL);
    markState();
    assert(updateState(NULL, NULL));
}
END_TEST
START_TEST(test_on_workspace_change){
    int size = getNumberOfWorkspaces();
    assert(getNumberOfWorkspaces() > 2);
    for(int i = 0; i < size; i++)
        addVisibleWindow(i);
    assert(updateState(NULL, NULL) & WORKSPACE_MONITOR_CHANGE);
    for(int i = 1; i < size; i++){
        switchToWorkspace(i);
        markState();
        assert(updateState(NULL, NULL) & WORKSPACE_MONITOR_CHANGE);
    }
    for(int i = 0; i < size; i++){
        switchToWorkspace(i);
        markState();
        assert(updateState(NULL, NULL) == WORKSPACE_MONITOR_CHANGE);
    }
}
END_TEST
START_TEST(test_on_invisible_workspace_window_add){
    int size = 3;
    assert(getNumberOfWorkspaces() > 2);
    WindowInfo* win[size];
    for(int i = 0; i < size; i++)
        win[i] = addVisibleWindow(i);
    switchToWorkspace(0);
    assert(updateState(NULL, NULL));
    moveWindowToWorkspace(win[2], 1);
    markState();
    assert(!updateState(onStateChange, NULL));
    switchToWorkspace(1);
    markState();
    assert(updateState(onStateChange, NULL));
    assert(visited == 1);
    assert(calledMasks == 2);
}
END_TEST

START_TEST(test_on_focus_change){
    int size = 9;
    WindowInfo* win[size];
    for(int i = 0; i < size; i++)
        win[i] = addVisibleWindow(getActiveWorkspaceIndex());
    assert(updateState(NULL, NULL));
    for(int i = 0; i < size; i++){
        updateFocusState(win[i]);
        markState();
        assert(!updateState(NULL, NULL));
    }
}
END_TEST

START_TEST(test_on_state_change){
    updateState(NULL, NULL);
    WindowInfo* winInfo = createWindowInfo(1);
    addWindowInfo(winInfo);
    addMask(winInfo, MAPPED_MASK);
    assert(isWorkspaceVisible(0));
    addWindowToWorkspace(winInfo, 0);
    markState();
    assert(updateState(onStateChange, NULL));
    assert(calledMasks == 1);
    assert(visited == 1);
}
END_TEST

START_TEST(test_num_workspaces_grow){
    updateState(NULL, NULL);
    markState();
    int num = getNumberOfWorkspaces();
    assert(num > 2);
    DEFAULT_NUMBER_OF_WORKSPACES = _i ? num * 2 : num / 2;
    resetContext();
    assert(num != getNumberOfWorkspaces());
    //detect change only when growing
    if(_i)
        assert(updateState(NULL, NULL));
    else
        assert(updateState(NULL, NULL) == WORKSPACE_MONITOR_CHANGE);
}
END_TEST


Suite* stateSuite(){
    Suite* s = suite_create("State");
    TCase* tc_core;
    tc_core = tcase_create("StateChange");
    tcase_add_checked_fixture(tc_core, createContextAndSimpleConnection, destroyContextAndConnection);
    tcase_add_test(tc_core, test_no_state_change);
    tcase_add_test(tc_core, test_state_change_num_windows);
    tcase_add_test(tc_core, test_mask_change);
    tcase_add_test(tc_core, test_layout_change);
    tcase_add_test(tc_core, test_on_state_change);
    tcase_add_test(tc_core, test_on_workspace_change);
    tcase_add_test(tc_core, test_on_focus_change);
    tcase_add_test(tc_core, test_on_invisible_workspace_window_add);
    tcase_add_loop_test(tc_core, test_num_workspaces_grow, 0, 2);
    suite_add_tcase(s, tc_core);
    return s;
}

