
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

    int win=mapArbitraryWindow();
    int activeWorkspace=1;
    int windowWorkspace=3;
    LOAD_SAVED_STATE=0;
    DEFAULT_WORKSPACE_INDEX=2;
    syncState();
    assert(getActiveWorkspaceIndex()==DEFAULT_WORKSPACE_INDEX);
    unsigned int currentWorkspace=DEFAULT_WORKSPACE_INDEX;
    assert(xcb_ewmh_get_current_desktop_reply(ewmh,
            xcb_ewmh_get_current_desktop(ewmh, defaultScreenNumber),
            &currentWorkspace, NULL));
    assert(getActiveWorkspaceIndex()==currentWorkspace);


    assert(xcb_request_check(dis, xcb_ewmh_set_current_desktop_checked(ewmh, defaultScreenNumber, activeWorkspace))==NULL);
    assert(xcb_request_check(dis,xcb_ewmh_set_wm_desktop(ewmh, win, windowWorkspace))==NULL);
    LOAD_SAVED_STATE=1;
    syncState();
    scan(root);

    assert(isInList(getAllWindows(),win));
    assert(getActiveWorkspaceIndex()==activeWorkspace);
    assert(!isInList(getActiveWindowStack(),win));
    assert(getSize(getWindowStack(getWorkspaceByIndex(windowWorkspace)))==1);

}END_TEST


START_TEST(test_focus_window){
    int win=mapArbitraryWindow();
    scan(root);
    focusWindow(win);
    xcb_input_xi_get_focus_reply_t*reply=xcb_input_xi_get_focus_reply(dis,
            xcb_input_xi_get_focus(dis, getActiveMasterKeyboardID()), NULL);

    assert(reply->focus==win);
    free(reply);

}END_TEST

START_TEST(test_focus_window_request){
    //TODO update when the X11 request focus methods supports multi focus
    int win=mapArbitraryWindow();
    scan(root);
    focusWindowInfo(getWindowInfo(win));
    xcb_input_xi_get_focus_reply_t*reply=xcb_input_xi_get_focus_reply(dis,
            xcb_input_xi_get_focus(dis, getActiveMasterKeyboardID()), NULL);

    assert(reply->focus==win);
    free(reply);
}END_TEST

START_TEST(test_activate_window){
    int win=createUnmappedWindow();
    int win2=createNormalWindow();
    scan(root);
    assert(!activateWindow(getWindowInfo(win)));
    assert(activateWindow(getWindowInfo(win2)));
}END_TEST
START_TEST(test_delete_window_request){
    KILL_TIMEOUT=_i==0?0:100;
    int fd[2];
    pipe(fd);
    if(!fork()){
        openXDisplay();
        int win=mapArbitraryWindow();
        xcb_atom_t atoms[]={WM_DELETE_WINDOW,ewmh->_NET_WM_PING};
        xcb_icccm_set_wm_protocols(dis, win, ewmh->WM_PROTOCOLS, _i==0?2:1, atoms);
        consumeEvents();
        write(fd[1],&win,sizeof(int));
        close(fd[1]);
        close(fd[0]);
        flush();
        close(1);
        close(2);
        
        if(_i){
            msleep(KILL_TIMEOUT*10);
        }
        if(_i==0){
            /// wait for client message
            waitForNormalEvent(XCB_CLIENT_MESSAGE);
            exit(0);
        }
        flush();
        assert(!xcb_connection_has_error(dis));
        if(checkStackingOrder(&win,1)==0)
            exit(0);
        WAIT_UNTIL_TRUE(0);
    }
    int win;
    read(fd[0],&win,sizeof(int));
    close(fd[0]);
    close(fd[1]);
    scan(root);
    WindowInfo*winInfo=getWindowInfo(win);
    assert(hasMask(winInfo, WM_DELETE_WINDOW_MASK));
    consumeEvents();
    START_MY_WM
    lock();
    if(_i!=1){
        if(_i==2){
            WindowInfo dummy={.id=1};
            addMask(&dummy,WM_DELETE_WINDOW_MASK);
            killWindowInfo(&dummy);
            msleep(KILL_TIMEOUT*2);
        }
        killWindowInfo(winInfo);
        flush();
    }
    else{ 
        removeWindow(win);
        killWindow(win);
    }
    unlock();
    wait(NULL);
}END_TEST

START_TEST(test_set_border_color){
    int win=mapWindow(createNormalWindow());
    scan(root);
    WindowInfo*winInfo=isInList(getAllWindows(),win);
    unsigned int colors[]={0,255,255*255,255*255*255};
    for(int i=0;i<LEN(colors);i++){
        assert(setBorderColor(win, colors[i]));
        flush();
    }
    setBorderColor(win, -1);
    assert(setBorder(winInfo));
    resetBorder(winInfo);
    //TODO check to see if border was set correctly
}END_TEST

START_TEST(test_invalid_state){
    int win=mapArbitraryWindow();
    xcb_ewmh_set_wm_desktop(ewmh, win, getNumberOfWorkspaces()+1);
    xcb_ewmh_set_current_desktop(ewmh, defaultScreenNumber, getNumberOfWorkspaces()+1);
    flush();
    scan(root);
    syncState();
    assert(getActiveWorkspaceIndex()<getNumberOfWorkspaces());
    assert(getWindowInfo(win)->workspaceIndex<getNumberOfWorkspaces());
}END_TEST

    
START_TEST(test_raise_window){
    //windows are in same spot
    int bottom=mapArbitraryWindow();
    int top=mapArbitraryWindow();
    registerForWindowEvents(bottom, XCB_EVENT_MASK_VISIBILITY_CHANGE);
    registerForWindowEvents(top, XCB_EVENT_MASK_VISIBILITY_CHANGE);
    scan(root);

    WindowInfo*info=getWindowInfo(bottom);
    WindowInfo*infoTop=getWindowInfo(top);
    assert(info&&infoTop);
    assert(raiseWindow(bottom));
    flush();
    int stackingOrder[]={top,bottom,top};
    assert(checkStackingOrder(stackingOrder,2));
    assert(raiseWindowInfo(infoTop));
    assert(checkStackingOrder(stackingOrder+1,2));
}END_TEST

START_TEST(test_workspace_change){

    int nonEmptyIndex=getActiveWorkspaceIndex();
    mapArbitraryWindow();
    mapArbitraryWindow();
    scan(root);
    assert(getSize(getAllWindows())==2);
    assert(getSize(getActiveWindowStack())==2);
    assert(getNumberOfWorkspaces()>1);

    for(int i=0;i<getNumberOfWorkspaces();i++){
        switchToWorkspace(i);
        if(i==nonEmptyIndex)continue;
        assert(getActiveWorkspace()->id==i);
        assert(getSize(getActiveWindowStack())==0);
    }
    switchToWorkspace(nonEmptyIndex);
    assert(getSize(getActiveWindowStack())==2);

    moveWindowToWorkspace(getHead(getActiveWindowStack()), !nonEmptyIndex);
    assert(getSize(getActiveWindowStack())==1);
    assert(getSize(getWindowStack(getWorkspaceByIndex(!nonEmptyIndex)))==1);

}END_TEST

START_TEST(test_workspace_activation){
    LOAD_SAVED_STATE=0;
    mapArbitraryWindow();
    mapArbitraryWindow();
    scan(root);
    int index1=getActiveWorkspaceIndex();
    int index2=!index1;
    moveWindowToWorkspace(getHead(getActiveWindowStack()), index2);

    WindowInfo* winInfo1=getHead(getWindowStack(getWorkspaceByIndex(index1)));
    WindowInfo* winInfo2=getHead(getWindowStack(getWorkspaceByIndex(index2)));
    int info1=winInfo1->id;
    int info2=winInfo2->id;

    assert(getSize(getAllMonitors())==1);

    switchToWorkspace(index2);
    assert(isWorkspaceVisible(index2));
    assert(!isWorkspaceVisible(index1));
    WAIT_UNTIL_TRUE(isWindowMapped(info2)&&!isWindowMapped(info1)&&getActiveWorkspaceIndex()==index2);

    switchToWorkspace(index1);
    assert(isWorkspaceVisible(index1));
    assert(!isWorkspaceVisible(index2));
    WAIT_UNTIL_TRUE(isWindowMapped(info1)&&!isWindowMapped(info2)&&getActiveWorkspaceIndex()==index1);

    activateWindow(winInfo2);
    assert(isWindowInVisibleWorkspace(winInfo2));
    assert(!isWindowInVisibleWorkspace(winInfo1));
    WAIT_UNTIL_TRUE(isWindowMapped(info2)&&!isWindowMapped(info1)&&getActiveWorkspaceIndex()==index2);
    assert(getHead(getActiveWindowStack())==winInfo2);
    activateWindow(winInfo1);
    assert(isWindowInVisibleWorkspace(winInfo1));
    assert(!isWindowInVisibleWorkspace(winInfo2));
    WAIT_UNTIL_TRUE(isWindowMapped(info1)&&!isWindowMapped(info2)&&getActiveWorkspaceIndex()==index1);
    assert(getSize(getActiveWindowStack())==1);
    assert(getHead(getActiveWindowStack())==winInfo1);
}END_TEST



START_TEST(test_configure_windows){
    int win=createNormalWindow();
    WindowInfo*winInfo=createWindowInfo(win);

    DEFAULT_WINDOW_MASKS=EXTERNAL_RESIZE_MASK|EXTERNAL_MOVE_MASK|EXTERNAL_BORDER_MASK;
    processNewWindow(winInfo);
    dumpWindowInfo(winInfo);
    short values[]={1,2,3,4,5};
    int allMasks=XCB_CONFIG_WINDOW_X|XCB_CONFIG_WINDOW_Y|XCB_CONFIG_WINDOW_HEIGHT|XCB_CONFIG_WINDOW_WIDTH|XCB_CONFIG_WINDOW_BORDER_WIDTH;
    int masks[]={XCB_CONFIG_WINDOW_X,XCB_CONFIG_WINDOW_Y,XCB_CONFIG_WINDOW_HEIGHT,XCB_CONFIG_WINDOW_WIDTH,XCB_CONFIG_WINDOW_BORDER_WIDTH};
    for(int i=0;i<LEN(masks);i++){
        short defaultValues[]={10,10,10,10,10,10};
        xcb_configure_window(dis, win, allMasks, defaultValues);
        processConfigureRequest(win, values, 0, 0, masks[i]);
        waitForNormalEvent(XCB_CONFIGURE_NOTIFY);
        xcb_get_geometry_reply_t* reply=xcb_get_geometry_reply(dis, xcb_get_geometry(dis, win), NULL);
        for(int n=0;n<=5;n++)
            assert((&reply->x)[n]==n==i?values[n]:defaultValues[n]);
        free(reply);
    }
    //TODO check below
    processConfigureRequest(win, NULL, root, XCB_STACK_MODE_ABOVE, XCB_CONFIG_WINDOW_STACK_MODE|XCB_CONFIG_WINDOW_SIBLING);

}END_TEST

START_TEST(test_float_sink_window){
    int win=mapWindow(createNormalWindow());
    WindowInfo*winInfo;
    START_MY_WM
    WAIT_UNTIL_TRUE(winInfo=getWindowInfo(win))
    WAIT_UNTIL_TRUE(isTileable(winInfo));
    lock();floatWindow(winInfo);unlock();
    assert(!isTileable(winInfo));
    assert(hasMask(winInfo, FLOATING_MASK));
    lock();sinkWindow(winInfo);unlock();
    assert(!hasMask(winInfo, FLOATING_MASK));
    WAIT_UNTIL_TRUE(isTileable(winInfo));
}END_TEST


START_TEST(test_apply_gravity){
    int win=createNormalWindow();
    xcb_size_hints_t hints;
    xcb_icccm_size_hints_set_win_gravity(&hints, XCB_GRAVITY_CENTER);
    xcb_icccm_set_wm_size_hints(dis, win, XCB_ATOM_WM_NORMAL_HINTS, &hints);
    for(int i=0;i<=XCB_GRAVITY_STATIC;i++){

        short values[5]={0,0,10,10,1};
        applyGravity(win, values, i);
        assert(values[0] || values[1] || i==XCB_GRAVITY_STATIC || !i);
    }
}END_TEST



START_TEST(test_set_workspace_names){
    char*name[]={"test","test2"};
    assert((unsigned int)getIndexFromName(name[0])>=LEN(name));
    setWorkspaceNames(name, 1);
    assert(getIndexFromName(name[0])==0);
    assert(strcmp(name[0],getWorkspaceName(0))==0);
    setWorkspaceNames(name, 2);
    assert(getIndexFromName(name[1])==1);
    assert(strcmp(name[1],getWorkspaceName(1))==0);

}END_TEST
START_TEST(test_unkown_window){
    int win=createNormalWindow();
    WindowInfo*winInfo=createWindowInfo(win);
    mapWindow(win);
    assert(focusWindow(win));
    assert(raiseWindow(win));
    setWindowStateFromAtomInfo(winInfo, NULL, 0, 0);
    registerWindow(winInfo);
    updateMapState(win, 0);
    updateMapState(win, 1);
    updateMapState(win, 0);
}END_TEST
START_TEST(test_bad_window){
    setLogLevel(LOG_LEVEL_NONE);
    int win =-2;
    assert(!focusWindow(win));
    assert(!raiseWindow(win));
    short arr[5];
    applyGravity(win, arr, 0);
}END_TEST

START_TEST(test_kill_window){
    int win=createNormalWindow();
    if(!fork()){
        openXDisplay();
        killWindow(win);
        flush();
        closeConnection();
    }
    wait(NULL);
    exit(0);
}END_TEST

static void setup(){
    onStartup();
    START_MY_WM
}
START_TEST(test_connect_to_xserver){
    connectToXserver();
    assert(getActiveMaster()!=NULL);
    assert(getActiveWorkspace()!=NULL);
    assert(getSize(getActiveMasterWindowStack())==0);
    assert(getSize(getAllWindows())==0);
    assert(getActiveMaster());
    Monitor*m=getHead(getAllMonitors());
    assert(m!=NULL);
    assert(getMonitorFromWorkspace(getActiveWorkspace()));

}END_TEST

Suite *x11Suite(void) {

    Suite*s = suite_create("Window Manager Functions");
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
    tcase_add_test(tc_core, test_focus_window);
    tcase_add_test(tc_core, test_focus_window_request);

    tcase_add_loop_test(tc_core, test_delete_window_request,0,3);
    tcase_add_test(tc_core, test_activate_window);
    tcase_add_test(tc_core, test_set_border_color);
    tcase_add_test(tc_core, test_workspace_activation);
    tcase_add_test(tc_core, test_workspace_change);

    tcase_add_test(tc_core, test_kill_window);
    tcase_add_test(tc_core, test_configure_windows);
    tcase_add_test(tc_core, test_float_sink_window);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Window Managment Operations");
    tcase_add_checked_fixture(tc_core, setup, fullCleanup);
    tcase_add_test(tc_core, test_apply_gravity);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("MISC");
    tcase_add_checked_fixture(tc_core, onStartup, destroyContextAndConnection);
    tcase_add_test(tc_core, test_set_workspace_names);
    tcase_add_test(tc_core, test_unkown_window);
    tcase_add_test(tc_core, test_bad_window);
    suite_add_tcase(s, tc_core);

    return s;
}
