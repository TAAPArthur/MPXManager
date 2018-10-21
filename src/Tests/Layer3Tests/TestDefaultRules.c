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
#include "../../functions.h"
#include "../../layouts.h"
#include "../../devices.h"
#include "../../bindings.h"

WindowInfo*winInfo;
WindowInfo*winInfo2;
Master* defaultMaster;
static int completedInit=0;
static void finishedInit(void){completedInit++;}
static int count=0;
static void dummy(void){count++;}
static int getDummyCount(){return count;}
static Binding testBinding={0,1,BIND(dummy),.mask=XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS};
static Binding testKeyPressBinding={0,XK_A,BIND(dummy),.mask=XCB_INPUT_XI_EVENT_MASK_KEY_PRESS};
static void sampleStartupMethod(){
    addBinding(&testBinding);
    addBinding(&testKeyPressBinding);
}

static void deviceEventsetup(){
    LOAD_SAVED_STATE=0;
    CRASH_ON_ERRORS=1;
    DEFAULT_WINDOW_MASKS=SRC_ANY;

    if(!startUpMethod)
        startUpMethod=sampleStartupMethod;
    NUMBER_OF_DEFAULT_LAYOUTS=0;
    onStartup();

    defaultMaster=getActiveMaster();
    int win1=mapWindow(createNormalWindow());
    int win2=mapWindow(createNormalWindow());
    xcb_icccm_set_wm_protocols(dis, win2, ewmh->WM_PROTOCOLS, 1, &ewmh->_NET_WM_PING);
    flush();
    scan(root);

    winInfo=getWindowInfo(win1);
    winInfo2=getWindowInfo(win2);
    assert(winInfo);
    assert(winInfo2);

    createMasterDevice("device2");
    createMasterDevice("device3");
    initCurrentMasters();

    flush();
    static Rule postInitRule=CREATE_DEFAULT_EVENT_RULE(finishedInit);
    addRule(Idle,&postInitRule);
    START_MY_WM
    WAIT_UNTIL_TRUE(completedInit);

}

static void nonDefaultDeviceEventsetup(){


    startUpMethod=addFocusFollowsMouseRule;
    deviceEventsetup();
    completedInit=0;
    toggleLayout(&DEFAULT_LAYOUTS[GRID]);
    flush();
    WAIT_UNTIL_TRUE(completedInit);


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

START_TEST(test_print_method){
    printStatusMethod=dummy;
    applyRules(eventRules[Idle],NULL);
    assert(getDummyCount());
}END_TEST

START_TEST(test_handle_error){
    CRASH_ON_ERRORS=0;
    xcb_input_key_press_event_t event={0};
    setLastEvent(&event);
    applyRules(eventRules[0],NULL);
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

    WAIT_UNTIL_FALSE(isInList(getAllWindows(), win));

    destroyWindow(win2);
    destroyWindow(win3);
    destroyWindow(win4);

    WAIT_FOR(isInList(getAllWindows(), win2)||
                isInList(getAllWindows(), win3)||
                isInList(getAllWindows(), win4));
    assert(getSize(getAllWindows())==0);
}END_TEST

START_TEST(test_visibility_update){
    clearLayoutsOfWorkspace(getActiveWorkspaceIndex());
    START_MY_WM

    lock();
    int win=createNormalWindow();
    unlock();

    WAIT_UNTIL_TRUE(getWindowInfo(win))
    WAIT_UNTIL_TRUE(isWindowVisible(getWindowInfo(win)));
    lock();
    int win2=createNormalWindow();
    unlock();
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

START_TEST(test_ignored_windows){
    createUserIgnoredWindow();
    mapWindow(createUserIgnoredWindow());
    scan(root);
    assert(getSize(getAllWindows())==0);
    consumeEvents();
    START_MY_WM
    createUserIgnoredWindow();
    mapWindow(createUserIgnoredWindow());
    msleep(1000);
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
    WAIT_UNTIL_TRUE(getDummyCount());
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

START_TEST(test_focus_follows_mouse){

    int id1=winInfo->id;
    int id2=winInfo2->id;
    assert(focusWindow(id2));
    short geo1[LEN(winInfo->geometry)],geo2[LEN(winInfo->geometry)];
    memcpy(geo1, getGeometry(winInfo), LEN(geo1)*sizeof(short));
    memcpy(geo2, getGeometry(winInfo2), LEN(geo2)*sizeof(short));

    if(_i){
        lock();
        deleteWindow(id1);
        deleteWindow(id2);
        unlock();
    }
    movePointer(getActiveMasterPointerID(), root, 0,0);
    movePointer(getActiveMasterPointerID(), root, geo1[0]+1, geo1[1]+1);
    movePointer(getActiveMasterPointerID(), root, geo1[0]+10, geo1[1]+10);
    movePointer(getActiveMasterPointerID(), root, geo2[0]+1, geo2[1]+1);
    movePointer(getActiveMasterPointerID(), root, geo2[0]+10, geo2[1]+10);
    LOG(LOG_LEVEL_ALL,"Size: %d\n\n",getSize(getAllWindows()));
/*
    LOG(LOG_LEVEL_NONE,"waiting\n\n");
    if(_i){
        WAIT_UNTIL_TRUE(getActiveFocus()==id1);
    }
    else
        WAIT_UNTIL_TRUE(getFocusedWindow()==winInfo);
*/

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
    waitForNormalEvent(XCB_MAP_NOTIFY);
}END_TEST
START_TEST(test_configure_request){
    int win=mapWindow(_i?createUnmappedWindow():createIgnoredWindow());

    int values[]={2,1,1000,1000,1,XCB_STACK_MODE_ABOVE};
    int mask= XCB_CONFIG_WINDOW_X|XCB_CONFIG_WINDOW_Y|
        XCB_CONFIG_WINDOW_WIDTH|XCB_CONFIG_WINDOW_HEIGHT |
        XCB_CONFIG_WINDOW_BORDER_WIDTH|XCB_CONFIG_WINDOW_STACK_MODE;
    assert(!catchError(xcb_configure_window_checked(dis, win, mask, values)));
    waitForNormalEvent(XCB_CONFIGURE_NOTIFY);
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
    assert(index==0);
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
        int id=_i?createNormalWindow():createIgnoredWindow();
        flush();
        registerForWindowEvents(id, XCB_EVENT_MASK_STRUCTURE_NOTIFY);
        xcb_ewmh_request_close_window(ewmh, defaultScreenNumber, id, 0, 0);
        flush();
        waitForNormalEvent(XCB_DESTROY_NOTIFY);
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

    int count=0;
    void fakeLayout(){
        count++;
    }
    Layout layout={.layoutFunction=fakeLayout,.name="Dummy"};

    void addFakeLayout(){
        assert(isNotEmpty(eventRules[onXConnection]));
        assert(getNumberOfWorkspaces());
        for(int i=0;i<getNumberOfWorkspaces();i++){
            addLayoutsToWorkspace(i, &layout, 1);
            assert(getActiveLayoutOfWorkspace(i)->layoutFunction);
        }
    }
    void addDummyWindows(){
        static Rule rule=CREATE_DEFAULT_EVENT_RULE(createNormalWindow);
        addRule(onXConnection, &rule);
    }
    preStartUpMethod=addDummyWindows;
    startUpMethod=addFakeLayout;

    onStartup();
    assert(getActiveLayoutOfWorkspace(0)==&layout);
    assert(getActiveLayoutOfWorkspace(0)->layoutFunction);


    LOG(LOG_LEVEL_ERROR,"%d\n\n\n",count);
    assert(count);
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
    static Rule properyChangeDummyRule=CREATE_DEFAULT_EVENT_RULE(dummy);
    addRule(XCB_PROPERTY_NOTIFY, &properyChangeDummyRule);
    xcb_ewmh_request_frame_extents(ewmh, defaultScreenNumber, winInfo->id);
    flush();
    WAIT_UNTIL_TRUE(getDummyCount());
}END_TEST

START_TEST(test_client_request_restack){
    assert(raiseWindow(winInfo2->id));
    addMask(winInfo, EXTERNAL_RAISE_MASK);
    LOG(LOG_LEVEL_DEBUG,"win: %d\n",winInfo->id);
    //processConfigureRequest(winInfo->id, NULL, winInfo2->id, XCB_STACK_MODE_ABOVE,  XCB_CONFIG_WINDOW_STACK_MODE|XCB_CONFIG_WINDOW_SIBLING);
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
START_TEST(test_client_ping){
    WindowInfo*rootInfo=createWindowInfo(root);
    addWindowInfo(rootInfo);
    addMask(rootInfo,WM_PING_MASK);
    xcb_ewmh_send_wm_ping(ewmh, root, 0);
    flush();

    WAIT_UNTIL_TRUE(rootInfo->pingTimeStamp)
}END_TEST

START_TEST(test_key_repeat){
    IGNORE_KEY_REPEAT=1;
    xcb_input_key_press_event_t event={.flags=XCB_INPUT_KEY_EVENT_FLAGS_KEY_REPEAT};
    setLastEvent(&event);
    onDeviceEvent();
   assert(getDummyCount()==0);
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
    tcase_add_test(tc_core, test_print_method);
    tcase_add_test(tc_core, test_handle_error);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Normal Events");
    tcase_add_checked_fixture(tc_core, regularEventsetup, fullCleanup);
    tcase_add_test(tc_core, test_detect_new_windows);
    tcase_add_test(tc_core, test_map_windows);
    tcase_add_test(tc_core, test_delete_windows);
    tcase_add_test(tc_core, test_visibility_update);
    tcase_add_test(tc_core, test_property_update);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Extra Events");
    tcase_add_checked_fixture(tc_core, regularEventsetup, fullCleanup);
    tcase_add_test(tc_core, test_ignored_windows);
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

    tc_core = tcase_create("Non_Default_Device_Events");
    tcase_add_checked_fixture(tc_core, nonDefaultDeviceEventsetup, deviceEventTeardown);
    tcase_add_loop_test(tc_core, test_focus_follows_mouse,0,2);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("KeyEvent");
    tcase_add_checked_fixture(tc_core, deviceEventsetup, deviceEventTeardown);
    tcase_add_test(tc_core, test_key_repeat);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Client_Messages");
    tcase_add_checked_fixture(tc_core, deviceEventsetup, deviceEventTeardown);
    tcase_add_test(tc_core, test_client_activate_window);
    tcase_add_test(tc_core, test_client_change_desktop);
    tcase_add_test(tc_core, test_client_set_window_workspace);
    tcase_add_test(tc_core, test_client_set_window_state);
    tcase_add_test(tc_core, test_client_show_desktop);
    tcase_add_test(tc_core, test_client_request_frame_extents);
    tcase_add_test(tc_core, test_client_request_restack);
    tcase_add_test(tc_core, test_client_request_move_resize);
    tcase_add_test(tc_core, test_client_ping);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Special Client Messages");
    tcase_add_checked_fixture(tc_core, onStartup, deviceEventTeardown);
    tcase_add_loop_test(tc_core, test_client_close_window,0,2);
    suite_add_tcase(s, tc_core);

    return s;
}
