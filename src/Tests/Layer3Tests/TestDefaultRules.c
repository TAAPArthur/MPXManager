#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>

#include "../UnitTests.h"
#include "../TestX11Helper.h"
#include "../../logger.h"
#include "../../globals.h"
#include "../../wmfunctions.h"
#include "../../devices.h"
#include "../../bindings.h"

WindowInfo*winInfo;
WindowInfo*winInfo2;
Master* defaultMaster;
static int count=0;
static void dummy(void){count++;}
static Binding testBinding={0,1,BIND(dummy),.mask=XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS};
static void sampleStartupMethod(){addBinding(&testBinding);};
static void deviceEventsetup(){
    LOAD_SAVED_STATE=0;
    CRASH_ON_ERRORS=1;
    DEFAULT_WINDOW_MASKS=SRC_ANY;
    startUpMethod=sampleStartupMethod;
    onStartup();

    defaultMaster=getActiveMaster();
    mapWindow(createNormalWindow());
    mapWindow(createNormalWindow());
    scan();

    winInfo=getValue(getAllWindows());
    winInfo2=getValue(getAllWindows()->next);
    assert(winInfo);
    assert(winInfo2);

    createMasterDevice("device2");
    createMasterDevice("device3");
    initCurrentMasters();
    //runEventLoop(NULL);
    START_MY_WM
}

static void deviceEventTeardown(){
    Node*n=getAllMasters();
    FOR_EACH(n,
        if(getValue(n)!=defaultMaster)
            destroyMasterDevice(getIntValue(n), getActiveMasterPointerID(), getActiveMasterKeyboardID()));
    flush();
    fullCleanup();
}
static void regularEventsetup(){
    onStartup();
}
int pid;

static void clientTeardown(){
    fullCleanup();
}

START_TEST(test_add_remove_rule){
    addDefaultRules();
    for(int n=0;n<2;n++){
        int size=0;
        for(int i=0;i<NUMBER_OF_EVENT_RULES;i++)
            size+=getSize(eventRules[i]);
        assert(n?!size:size);
        clearAllRules();
    }
}END_TEST

START_TEST(test_on_startup){

    assert(isNotEmpty(getAllMonitors()));
    assert(isNotEmpty(getAllMasters()));
    assert(dis);
    assert(!xcb_connection_has_error(dis));
    assert(isWorkspaceVisible(getActiveWorkspaceIndex()));

}END_TEST

START_TEST(test_detect_new_windows){

    int win=createUnmappedWindow();
    int win2=createUnmappedWindow();
    START_MY_WM
    int win3=createUnmappedWindow();
    WAIT_UNTIL_TRUE(isInList(getAllWindows(), win)&&
        isInList(getAllWindows(), win2)&&
        isInList(getAllWindows(), win3));
    assert(getSize(getActiveWindowStack())==3);
}END_TEST

START_TEST(test_delete_windows){

    setLogLevel(LOG_LEVEL_NONE);
    int win=createUnmappedWindow();
    int win2=createUnmappedWindow();
    int win3=createUnmappedWindow();

    destroyWindow(win);
    mapWindow(win2);
    START_MY_WM

    int win4=createUnmappedWindow();

    WAIT_UNTIL_TRUE(isInList(getAllWindows(), win4)&&
            isInList(getAllWindows(), win2)&&
            isInList(getAllWindows(), win3))

    destroyWindow(win2);
    destroyWindow(win3);
    destroyWindow(win4);

    WAIT_FOR(isInList(getAllWindows(), win)||
                isInList(getAllWindows(), win2)||
                isInList(getAllWindows(), win3)||
                isInList(getAllWindows(), win4));

}END_TEST

START_TEST(test_visibility_update){
    clearLayoutsOfWorkspace(getActiveWorkspaceIndex());
    START_MY_WM

    int win=createNormalWindow();

    WAIT_UNTIL_TRUE(getWindowInfo(win))
    WAIT_UNTIL_TRUE(isWindowVisible(getWindowInfo(win)));
    int win2=createNormalWindow();
    WAIT_UNTIL_TRUE(getWindowInfo(win2))
    WAIT_UNTIL_TRUE(isWindowVisible(getWindowInfo(win2)));
    assert(!isWindowVisible(getWindowInfo(win)));

}END_TEST

START_TEST(test_property_update){
    START_MY_WM
    int win=createNormalWindow();
    WAIT_UNTIL_TRUE(getWindowInfo(win))
    setProperties(win);
    WAIT_UNTIL_TRUE(getWindowInfo(win)->title);
    lock();
    checkProperties(getWindowInfo(win));
    unlock();

}END_TEST


START_TEST(test_map_windows){

    int win=createUnmappedWindow();
    int win2=createUnmappedWindow();
    START_MY_WM
    int win3=createUnmappedWindow();

    mapWindow(win3);
    mapWindow(win2);


    //wait for all to be in list
    WAIT_UNTIL_TRUE(isInList(getAllWindows(), win)&&
                isInList(getAllWindows(), win2)&&
                isInList(getAllWindows(), win3))
    //WAIT_UNTIL_TRUE(!isWindowMapped(win)&&isWindowMapped(win2)&&isWindowMapped(win3));
    mapWindow(win);
    //wait for all to be mapped
    WAIT_UNTIL_TRUE(isWindowMapped(win)&&isWindowMapped(win2)&&isWindowMapped(win3));


}END_TEST
START_TEST(test_device_event){
    focusWindow(root);
    flush();
    sendButtonPress(1);
    flush();
    WAIT_UNTIL_TRUE(count);
}END_TEST
START_TEST(test_master_device_add_remove){
    int numMaster=getSize(getAllMasters());
    createMasterDevice("test1");
    createMasterDevice("test2");
    flush();

    WAIT_UNTIL_TRUE(getSize(getAllMasters())==numMaster+2);
    lock();
    destroyMasterDevice(getIntValue(getAllMasters()->next), getActiveMasterPointerID(),getActiveMasterKeyboardID());
    destroyMasterDevice(getIntValue(getAllMasters()->next->next), getActiveMasterPointerID(),getActiveMasterKeyboardID());
    flush();
    unlock();
    WAIT_UNTIL_TRUE(getSize(getAllMasters())==numMaster);
}END_TEST
START_TEST(test_focus_update){
    assert(getFocusedWindow()!=winInfo);
    assert(focusWindow(winInfo->id));
    flush();
    WAIT_UNTIL_TRUE(getFocusedWindow()==winInfo);
    assert(focusWindow(winInfo2->id));
    WAIT_UNTIL_TRUE(getFocusedWindow()==winInfo2);
}END_TEST

static void clientSetup(){
    LOAD_SAVED_STATE=0;
    pid=fork();
    if(!pid){
        ROOT_EVENT_MASKS=XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
        onStartup();
    }
    else{
        DEFAULT_WINDOW_MASKS|=EXTERNAL_MOVE_MASK;
        onStartup();
        START_MY_WM
        waitForCleanExit();
        fullCleanup();
        exit(0);
    }
}

START_TEST(test_map_request){
    mapWindow(_i?createUnmappedWindow():createIgnoredWindow());
    flush();
    waitForNormalEvents(1<<XCB_MAP_NOTIFY);
}END_TEST
START_TEST(test_configure_request){
    int win=mapWindow(_i?createUnmappedWindow():createIgnoredWindow());

    int values[]={2,1,1000,1000,1,XCB_STACK_MODE_ABOVE};
    int mask= XCB_CONFIG_WINDOW_X|XCB_CONFIG_WINDOW_Y|
        XCB_CONFIG_WINDOW_WIDTH|XCB_CONFIG_WINDOW_HEIGHT |
        XCB_CONFIG_WINDOW_BORDER_WIDTH|XCB_CONFIG_WINDOW_STACK_MODE;
    assert(!catchError(xcb_configure_window_checked(dis, win, mask, values)));
    waitForNormalEvents(1<<XCB_CONFIGURE_NOTIFY);
}END_TEST

START_TEST(test_client_activate_window){

    lock();
    assert(focusWindow(winInfo2->id));
    assert(focusWindow(winInfo->id));
    Master*mainMaster=getActiveMaster();
    Master*other=getValue(getAllMasters()->next);
    assert(mainMaster!=other && "testing error");
    setActiveMaster(other);
    focusWindow(winInfo->id);
    setClientPointerForWindow(winInfo->id);
    setActiveMaster(mainMaster);
    int index=getActiveWorkspaceIndex();
    switchToWorkspace(!index);
    xcb_ewmh_request_change_active_window(ewmh, defaultScreenNumber, winInfo2->id, 1, 0, winInfo->id);
    flush();
    unlock();
    WAIT_UNTIL_TRUE(getActiveWorkspaceIndex()==index);
    WAIT_UNTIL_TRUE(getValue(getFocusedWindowByMaster(other))==winInfo2);
    assert(getValue(getFocusedWindowByMaster(mainMaster))==winInfo);
}END_TEST

START_TEST(test_client_change_desktop){
    for(int i=0;i<getNumberOfWorkspaces();i++){
        xcb_ewmh_request_change_current_desktop(ewmh, defaultScreenNumber, i, 0);
        flush();
        WAIT_UNTIL_TRUE(getActiveWorkspaceIndex()==i);
    }
}END_TEST

START_TEST(test_client_close_window){
    DEFAULT_WINDOW_MASKS|=SRC_ANY;
    if(!fork()){
        openXDisplay();
        int id=createNormalWindow();
        flush();
        registerForWindowEvents(id, XCB_EVENT_MASK_STRUCTURE_NOTIFY);
        xcb_ewmh_request_close_window(ewmh, defaultScreenNumber, id, 0, 0);
        flush();
        waitForNormalEvents(1<<XCB_DESTROY_NOTIFY);
        closeConnection();
        exit(0);
    }
    else{
        START_MY_WM
        assert(getExitStatusOfFork()==1);
    }

}END_TEST

START_TEST(test_client_set_window_workspace){
    int workspace=3;
    xcb_ewmh_request_change_wm_desktop(ewmh, defaultScreenNumber, winInfo->id, workspace, 0);
    flush();
    WAIT_UNTIL_TRUE(isInList(getWindowStack(getWorkspaceByIndex(workspace)), winInfo->id));
}END_TEST
START_TEST(test_client_set_window_state){
    int states[]={XCB_EWMH_WM_STATE_ADD,XCB_EWMH_WM_STATE_REMOVE,XCB_EWMH_WM_STATE_TOGGLE,XCB_EWMH_WM_STATE_TOGGLE};
    for(int i=0;i<LEN(states);i++){
        xcb_ewmh_request_change_wm_state(ewmh, defaultScreenNumber, winInfo->id, states[i], ewmh->_NET_WM_STATE_MAXIMIZED_HORZ, ewmh->_NET_WM_STATE_MAXIMIZED_VERT, 0);
        flush();
        if(i%2==0)
            WAIT_UNTIL_TRUE(hasMask(winInfo, X_MAXIMIZED_MASK)&&hasMask(winInfo, Y_MAXIMIZED_MASK))
        else
            WAIT_UNTIL_FALSE(hasMask(winInfo, X_MAXIMIZED_MASK)&&hasMask(winInfo, Y_MAXIMIZED_MASK));
    }

}END_TEST

START_TEST(test_auto_tile){
    Layout layout={0};
    int count=0;
    void fakeLayout(Monitor*m  __attribute__((unused)),Node*n  __attribute__((unused)),int*i  __attribute__((unused))){
        count++;
    }
    void addFakeLayout(){

        layout.layoutFunction=fakeLayout;
        for(int i=0;i<getNumberOfWorkspaces();i++){
            addLayoutsToWorkspace(i, &layout, 1);
            assert(getActiveLayoutOfWorkspace(i)->layoutFunction);
        }
    }
    startUpMethod=addFakeLayout;
    onStartup();
    assert(getActiveLayoutOfWorkspace(0)->layoutFunction);
    assert(isNotEmpty(eventRules[onXConnection]));
    assert(count==getSize(getAllMonitors()));
}END_TEST
START_TEST(test_client_show_desktop){
    xcb_ewmh_request_change_showing_desktop(ewmh, defaultScreenNumber, 1);
    flush();
    WAIT_UNTIL_TRUE(isShowingDesktop(getActiveWorkspaceIndex()));
    xcb_ewmh_request_change_showing_desktop(ewmh, defaultScreenNumber, 0);
    flush();
    WAIT_UNTIL_FALSE(isShowingDesktop(getActiveWorkspaceIndex()));
}END_TEST

START_TEST(test_client_request_frame_extents){
    consumeEvents();
    static Rule properyChangeDummyRule=CREATE_DEFAULT_EVENT_RULE(dummy);
    addRule(XCB_PROPERTY_NOTIFY, &properyChangeDummyRule);
    xcb_ewmh_request_frame_extents(ewmh, defaultScreenNumber, winInfo->id);
    flush();
    WAIT_UNTIL_TRUE(count);
}END_TEST

START_TEST(test_client_request_restack){

    raiseWindow(winInfo2->id);
    addMask(winInfo, EXTERNAL_RAISE_MASK);
    consumeEvents();
    xcb_ewmh_request_restack_window(ewmh, defaultScreenNumber, winInfo->id, winInfo2->id, XCB_STACK_MODE_ABOVE);
    flush();
    WAIT_UNTIL_TRUE(isWindowVisible(winInfo));
}END_TEST


START_TEST(test_client_request_move_resize){
    short values[]={1,2,3,4};

    addMask(winInfo, EXTERNAL_MOVE_MASK|EXTERNAL_RESIZE_MASK);
    int flags=XCB_EWMH_MOVERESIZE_WINDOW_X|XCB_EWMH_MOVERESIZE_WINDOW_Y|
            XCB_EWMH_MOVERESIZE_WINDOW_WIDTH|XCB_EWMH_MOVERESIZE_WINDOW_HEIGHT;
    xcb_ewmh_request_moveresize_window(ewmh, defaultScreenNumber, winInfo->id,
            0, 2, flags, values[0], values[1], values[2], values[3]);
    flush();

    WAIT_UNTIL_TRUE(memcmp(values, winInfo->geometry, sizeof(short)*LEN(values))==0)

}END_TEST

Suite *defaultRulesSuite() {
    Suite*s = suite_create("Default events");
    TCase* tc_core;

    tc_core = tcase_create("Rule loading Events");
    tcase_add_checked_fixture(tc_core, init, NULL);
    tcase_add_test(tc_core, test_add_remove_rule);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Startup Events");
    tcase_add_checked_fixture(tc_core, NULL, destroyContextAndConnection);
    tcase_add_test(tc_core, test_auto_tile);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Special Events");
    tcase_add_checked_fixture(tc_core, regularEventsetup, fullCleanup);
    tcase_add_test(tc_core, test_on_startup);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Normal Events");
    tcase_add_checked_fixture(tc_core, regularEventsetup, fullCleanup);
    tcase_add_test(tc_core, test_detect_new_windows);
    tcase_add_test(tc_core, test_map_windows);
    tcase_add_test(tc_core, test_delete_windows);
    tcase_add_test(tc_core, test_visibility_update);
    tcase_add_test(tc_core, test_property_update);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Requests");
    tcase_add_checked_fixture(tc_core, clientSetup, fullCleanup);
    tcase_add_loop_test(tc_core, test_map_request,0,2);
    tcase_add_loop_test(tc_core, test_configure_request,0,2);

    suite_add_tcase(s, tc_core);


    tc_core = tcase_create("Default Device Events");
    tcase_add_checked_fixture(tc_core, deviceEventsetup, deviceEventTeardown);
    tcase_add_test(tc_core, test_master_device_add_remove);
    tcase_add_test(tc_core, test_device_event);
    tcase_add_test(tc_core, test_focus_update);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Client Messages");
    tcase_add_checked_fixture(tc_core, deviceEventsetup, deviceEventTeardown);
    tcase_add_test(tc_core, test_client_activate_window);
    tcase_add_test(tc_core, test_client_change_desktop);

    tcase_add_test(tc_core, test_client_set_window_workspace);
    tcase_add_test(tc_core, test_client_set_window_state);
    tcase_add_test(tc_core, test_client_show_desktop);
    tcase_add_test(tc_core, test_client_request_frame_extents);
    tcase_add_test(tc_core, test_client_request_restack);
    tcase_add_test(tc_core, test_client_request_move_resize);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Special Client Messages");
    tcase_add_checked_fixture(tc_core, onStartup, deviceEventTeardown);
    tcase_add_test(tc_core, test_client_close_window);
    suite_add_tcase(s, tc_core);




    return s;
}

