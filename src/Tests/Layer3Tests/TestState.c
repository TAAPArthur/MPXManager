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
#include "../../wmfunctions.h"

static int calledMasks;
static int visited;
static void onStateChange(int i){
    int mask= 1<<i;
    calledMasks|=mask;
    visited++;
}
void setup(){
    createSimpleContext();
}
START_TEST(test_no_state_change){
    assert(updateState(NULL));
    markState();
    assert(!updateState(NULL));
}END_TEST
START_TEST(test_mask_change){
    WindowInfo*winInfo=createWindowInfo(createNormalWindow());
    addWindowInfo(winInfo);
    addWindowToWorkspace(winInfo, getActiveWorkspaceIndex());
    addMask(winInfo,MAPPED_MASK);
    assert(!hasMask(winInfo, 1));
    assert(updateState(NULL));
    addMask(winInfo, 1);
    markState();
    assert(updateState(NULL));
}END_TEST
START_TEST(test_layout_change){
    Layout l={0};
    assert(updateState(NULL));
    setActiveLayout(&l);
    markState();
    assert(updateState(NULL));
    setActiveLayout(NULL);
    markState();
    assert(updateState(NULL));
}END_TEST
START_TEST(test_on_workspace_change){
    int size=getNumberOfWorkspaces();
    assert(getNumberOfWorkspaces()>2);
    WindowInfo*win[size];
    for(int i=0;i<size;i++){
        win[i]=createWindowInfo(mapWindow(createNormalWindow()));
        addWindowInfo(win[i]);
        addWindowToWorkspace(win[i], i);
    }
    assert(updateState(NULL));
    for(int i=1;i<size;i++){
        switchToWorkspace(i);
        markState();
        assert(updateState(NULL));
    }
    for(int i=0;i<size;i++){
        switchToWorkspace(i);
        markState();
        assert(!updateState(NULL));
    }
}END_TEST
START_TEST(test_on_invisible_workspace_window_add){
    int size=3;
    assert(getNumberOfWorkspaces()>2);
    WindowInfo*win[size];
    for(int i=0;i<size;i++){
        win[i]=createWindowInfo(mapWindow(createNormalWindow()));
        addWindowInfo(win[i]);
        addWindowToWorkspace(win[i], i);
    }
    switchToWorkspace(0);
    assert(updateState(NULL));
    moveWindowToWorkspace(win[2],1);
    markState();
    assert(!updateState(onStateChange));
    switchToWorkspace(1);
    markState();
    assert(updateState(onStateChange));
    assert(visited==1);
    assert(calledMasks==2);
}END_TEST

START_TEST(test_on_focus_change){
    int size=9;
    WindowInfo*win[size];
    for(int i=0;i<size;i++){
        win[i]=createWindowInfo(createNormalWindow());
        addWindowInfo(win[i]);
        addWindowToWorkspace(win[i], getActiveWorkspaceIndex());
    }
    assert(updateState(NULL));
    for(int i=0;i<size;i++){
        updateFocusState(win[i]);
        markState();
        assert(!updateState(NULL));
    }
}END_TEST
START_TEST(test_state_change_num_windows){
    assert(updateState(NULL));

    for(int i=1;i<10;i++){
        WindowInfo*winInfo=createWindowInfo(i);
        addWindowInfo(winInfo);
        addMask(winInfo,MAPPED_MASK);
        assert(isWorkspaceVisible(0));
        addWindowToWorkspace(winInfo, 0);
        markState();
        assert(updateState(NULL));
        break;
    }
}END_TEST

START_TEST(test_on_state_change){
    updateState(NULL);
    WindowInfo*winInfo=createWindowInfo(1);
    addWindowInfo(winInfo);
    addMask(winInfo,MAPPED_MASK);
    assert(isWorkspaceVisible(0));
    addWindowToWorkspace(winInfo, 0);
    markState();
    assert(updateState(onStateChange));
    assert(calledMasks==1);
    assert(visited==1);
}END_TEST

START_TEST(test_num_workspaces_grow){
    updateState(NULL);
    markState();
    int num=getNumberOfWorkspaces();
    assert(num>2);
    destroyContext();
    createContext(_i?num*2:num/2);
    assert(num!=getNumberOfWorkspaces());
    //detect change only when growing
    assert(updateState(NULL)==_i);
}END_TEST


Suite *stateSuite() {
    Suite*s = suite_create("State");
    TCase* tc_core;

    tc_core = tcase_create("StateChange");

    tcase_add_checked_fixture(tc_core, createContextAndSimpleConnection, destroyContextAndConnection);
    tcase_add_test(tc_core, test_no_state_change);
    tcase_add_test(tc_core,test_state_change_num_windows);
    tcase_add_test(tc_core,test_on_state_change);
    tcase_add_test(tc_core,test_on_workspace_change);
    tcase_add_test(tc_core,test_on_focus_change);
    tcase_add_test(tc_core,test_on_invisible_workspace_window_add);
    tcase_add_test(tc_core,test_mask_change);
    tcase_add_test(tc_core,test_layout_change);
    tcase_add_loop_test(tc_core,test_num_workspaces_grow,0,2);
    suite_add_tcase(s, tc_core);

    return s;
}

