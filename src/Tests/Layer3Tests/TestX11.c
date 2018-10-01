
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
#include "../../ewmh.h"
#include "../../globals.h"
#include "../../layouts.h"

#include "../UnitTests.h"
#include "../TestX11Helper.h"




START_TEST(test_mask_add_remove_toggle){
    WindowInfo*winInfo=createWindowInfo(createNormalWindow());
    addWindowInfo(winInfo);
    assert(winInfo->mask==0);
    addMask(winInfo, 1);
    assert(winInfo->mask==1);
    addMask(winInfo, 6);
    assert(winInfo->mask==7);
    removeMask(winInfo, 12);
    assert(winInfo->mask==3);
    assert(!hasMask(winInfo, 6));
    toggleMask(winInfo, 6);
    assert(hasMask(winInfo, 6));
    assert(winInfo->mask==7);
    toggleMask(winInfo, 6);
    assert(!hasMask(winInfo, 6));
    assert(winInfo->mask==1);
}END_TEST

START_TEST(test_mask_reset){
    WindowInfo*winInfo=createWindowInfo(createNormalWindow());
    addWindowInfo(winInfo);
    addMask(winInfo, X_MAXIMIZED_MASK|Y_MAXIMIZED_MASK|ROOT_FULLSCREEN_MASK|FULLSCREEN_MASK);
    int nonUserMask=MAPPED_MASK|PARTIALLY_VISIBLE;
    addMask(winInfo,nonUserMask);
    resetUserMask(winInfo);
    assert(winInfo->mask==nonUserMask);
}END_TEST

START_TEST(test_get_set_geometry_config){
    WindowInfo*winInfo=createWindowInfo(1);
    addWindowInfo(winInfo);
    short arr[10]={1,2,3,4,5,6,7,8,9,10};
    assert(LEN(arr)==LEN(winInfo->geometry)+LEN(winInfo->config) && "array size needs to be increased");
    int offset=LEN(winInfo->geometry);
    setGeometry(winInfo,arr);
    short *geo=getGeometry(winInfo);
    setConfig(winInfo,&arr[offset]);
    short *config=getConfig(winInfo);
    for(int i=0;i<LEN(arr);i++)
        assert(arr[i]==(i<offset?geo[i]:config[i-offset]));
}END_TEST

START_TEST(test_connect_to_xserver){
    init();
    connectToXserver();
    assert(getActiveMaster()!=NULL);
    assert(getActiveWorkspace()!=NULL);
    assert(getSize(getMasterWindowStack())==0);
    assert(getSize(getAllWindows())==0);
    assert(getActiveMaster());
    Monitor*m=getValue(getAllMonitors());
    assert(m!=NULL);
    assert(getMonitorFromWorkspace(getActiveWorkspace()));

}END_TEST



START_TEST(test_window_property_alt_loading){
    int win=createUnmappedWindow();
    WindowInfo*winInfo=createWindowInfo(win);
    char*name="test";
    xcb_icccm_set_wm_name(dis, win, XCB_ATOM_STRING, 8, strlen(name), name);
    addWindowInfo(winInfo);
    loadWindowProperties(winInfo);
    assert(strcmp(winInfo->title,name)==0);
}END_TEST
START_TEST(test_window_property_loading){
    int win=createIgnoredWindow();
    WindowInfo*winInfo=createWindowInfo(win);
    addWindowInfo(winInfo);
    //no properties are set don't crash
    loadWindowProperties(winInfo);
    assert(winInfo->className==NULL);
    assert(winInfo->instanceName==NULL);
    assert(winInfo->title==NULL);
    assert(winInfo->typeName==NULL||winInfo->implicitType);
    assert(winInfo->type==0||winInfo->implicitType);

    setProperties(win);

    //reloaeding should break anything
    for(int i=0;i<2;i++){
        loadWindowProperties(winInfo);
        checkProperties(winInfo);
    }
}END_TEST
START_TEST(test_window_property_reloading){
    int win=createUnmappedWindow();
    int win2=createUnmappedWindow();
    WindowInfo*winInfo=createWindowInfo(win);
    addWindowInfo(winInfo);
    setProperties(win);
    loadWindowProperties(winInfo);
    //very hacky way to unset the title property of the window
    winInfo->id=win2;
    //don't double free window properties
    loadWindowProperties(winInfo);
    loadWindowProperties(winInfo);
}END_TEST
START_TEST(test_window_extra_property_loading){

    int win=createUnmappedWindow();
    int groupLead=createUnmappedWindow();
    int trasientForWindow=createUnmappedWindow();
    WindowInfo*winInfo=createWindowInfo(win);
    xcb_icccm_wm_hints_t hints;
    xcb_icccm_wm_hints_set_window_group(&hints, groupLead);
    xcb_icccm_set_wm_hints(dis, win, &hints);
    xcb_icccm_set_wm_transient_for(dis, win, trasientForWindow);
    flush();
    addWindowInfo(winInfo);
    loadWindowProperties(winInfo);
    assert(winInfo->groupId==groupLead);
    assert(winInfo->transientFor==trasientForWindow);
}END_TEST

START_TEST(test_sync_state){

    int win=mapArbitraryWindow();
    int activeWorkspace=1;
    int windowWorkspace=3;
    DEFAULT_WORKSPACE_INDEX=2;
    syncState();
    assert(getActiveWorkspaceIndex()==DEFAULT_WORKSPACE_INDEX);


    assert(xcb_request_check(dis, xcb_ewmh_set_current_desktop_checked(ewmh, defaultScreenNumber, activeWorkspace))==NULL);
    assert(xcb_request_check(dis,xcb_ewmh_set_wm_desktop(ewmh, win, windowWorkspace))==NULL);
    //setLogLevel(LOG_LEVEL_TRACE);
    syncState();
    scan(root);

    assert(isInList(getAllWindows(),win));
    assert(getActiveWorkspaceIndex()==activeWorkspace);
    assert(!isInList(getActiveWindowStack(),win));
    assert(getSize(getWindowStack(getWorkspaceByIndex(windowWorkspace)))==1);

}END_TEST

START_TEST(test_window_state){

    xcb_atom_t net_atoms[] = {WM_TAKE_FOCUS,WM_DELETE_WINDOW,SUPPORTED_STATES};
    int win=createNormalWindow();
    WindowInfo*winInfo=createWindowInfo(win);
    addWindowInfo(winInfo);
    addWindowToWorkspace(winInfo, 0);

    //Check to make sure some mask is set
    //should not error
    setWindowStateFromAtomInfo(NULL, net_atoms, LEN(net_atoms), XCB_EWMH_WM_STATE_REMOVE);
    setWindowStateFromAtomInfo(winInfo, net_atoms, LEN(net_atoms), XCB_EWMH_WM_STATE_REMOVE);
    assert(winInfo->mask==0);
    setWindowStateFromAtomInfo(winInfo, net_atoms, LEN(net_atoms), XCB_EWMH_WM_STATE_ADD);
    assert(winInfo->mask);
    setWindowStateFromAtomInfo(winInfo, net_atoms, LEN(net_atoms), XCB_EWMH_WM_STATE_TOGGLE);
    assert(winInfo->mask==0);

    catchError(xcb_ewmh_set_wm_state_checked(ewmh, winInfo->id, 1,(unsigned int[]){-1}));
    setXWindowStateFromMask(winInfo);
    xcb_ewmh_get_atoms_reply_t reply;
    int hasState=xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, winInfo->id), &reply, NULL);
    assert(hasState);
    for(int i=0;i<reply.atoms_len;i++)
        if(reply.atoms[i]==-1){
            xcb_ewmh_get_atoms_reply_wipe(&reply);
            return;
        }
    assert(0);
}END_TEST

START_TEST(test_window_state_sync){
    int win=createNormalWindow();
    scan(root);
    WindowInfo*winInfo=getWindowInfo(win);
    moveWindowToLayer(winInfo, getActiveWorkspaceIndex(), UPPER_LAYER);
    addMask(winInfo, -1);

    xcb_atom_t net_atoms[] = {SUPPORTED_STATES};
    xcb_ewmh_get_atoms_reply_t reply;
    int count=0;

    int status=xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, win), &reply, NULL);
    assert(status);
    for(int j=0;j<LEN(net_atoms);j++)
        for(int i=0;i<reply.atoms_len;i++)
            if(reply.atoms[i]==net_atoms[j]){
                count++;
                break;
            }
    assert(count+1==LEN(net_atoms));
    xcb_ewmh_get_atoms_reply_wipe(&reply);
}END_TEST

START_TEST(test_layer_move){

    int win=createUnmappedWindow();
    WindowInfo*winInfo=createWindowInfo(win);
    LOAD_SAVED_STATE=0;
    processNewWindow(winInfo);
    for(int i=0;i<NUMBER_OF_LAYERS;i++){
        moveWindowToLayer(winInfo, getActiveWorkspaceIndex(), i);
        assert(isInList(getActiveWorkspace()->windows[i], winInfo->id));
    }
}END_TEST

START_TEST(test_process_window){
    DEFAULT_WINDOW_MASKS=0;
    xcb_atom_t net_atoms[] = {SUPPORTED_STATES};
    int win=createUnmappedWindow();
    setProperties(win);

    xcb_ewmh_set_wm_state(ewmh, win, LEN(net_atoms), net_atoms);
    flush();
    assert(!isNotEmpty(getAllWindows()));
    WindowInfo*winInfo=createWindowInfo(win);
    processNewWindow(winInfo);
    assert(isNotEmpty(getAllWindows()));
    assert(getSize(getAllWindows())==1);
    checkProperties(getValue(getAllWindows()));
    xcb_get_window_attributes_reply_t *attr;
    attr=xcb_get_window_attributes_reply(dis,xcb_get_window_attributes(dis, win),NULL);
    assert(attr->your_event_mask==NON_ROOT_EVENT_MASKS);
    free(attr);
    assert(winInfo->mask);
}END_TEST
START_TEST(test_window_scan){
    assert(!isNotEmpty(getAllWindows()));
    scan(root);
    assert(!isNotEmpty(getAllWindows()));
    int win=createUnmappedWindow();
    int win2=createUnmappedWindow();
    int win3=createUnmappedWindow();
    assert(!isWindowMapped(win));
    assert(!isWindowMapped(win2));
    assert(!isWindowMapped(win3));
    mapWindow(win);
    xcb_flush(dis);
    scan(root);
    assert(isInList(getAllWindows(), win));
    assert(isInList(getAllWindows(), win2));
    assert(isInList(getAllWindows(), win3));
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
    KILL_TIMEOUT=20;
    int fd[2];
    int fd2[2];
    pipe(fd);
    pipe(fd2);
    if(!fork()){
        openXDisplay();
        int win=mapArbitraryWindow();
        xcb_atom_t atoms[]={WM_DELETE_WINDOW,ewmh->_NET_WM_PING};
        xcb_icccm_set_wm_protocols(dis, win, ewmh->WM_PROTOCOLS, _i?2:1, atoms);
        write(fd[1],&win,sizeof(int));
        close(fd[1]);
        close(fd[0]);
        flush();
        read(fd2[0],&win,sizeof(int));
        close(fd2[1]);
        close(fd2[0]);
        if(_i==1){
            msleep(KILL_TIMEOUT*10);
            mapArbitraryWindow();
            LOG(LOG_LEVEL_NONE,"Code should have terminated\n");
            msleep(KILL_TIMEOUT*10000);
        }
        waitForNormalEvent(XCB_CLIENT_MESSAGE);
        closeConnection();
        msleep(KILL_TIMEOUT*2);
        exit(0);
    }
    int win;
    read(fd[0],&win,sizeof(int));
    close(fd[0]);
    close(fd[1]);
    scan(root);
    WindowInfo*winInfo=getWindowInfo(win);
    assert(hasMask(winInfo, WM_DELETE_WINDOW_MASK));
    consumeEvents();
    lock();
    killWindowInfo(winInfo);
    flush();
    unlock();
    write(fd2[1],&win,sizeof(int));
    close(fd2[0]);
    close(fd2[1]);
    if(_i==2){
        lock();
        removeWindow(win);
        killWindow(win);
        unlock();
        msleep(KILL_TIMEOUT*2);
    }
    else if(_i==3){
        lock();
        requestShutdown();
        msleep(KILL_TIMEOUT*2);
        unlock();
        return;
    }
    wait(NULL);
}END_TEST

START_TEST(test_set_border_color){
    int win=mapArbitraryWindow();
    unsigned int colors[]={0,-1,255};
    for(int i=0;i<LEN(colors);i++){
        setBorderColor(win, colors[i]);
        flush();
    }
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

    WindowInfo*info=getValue(isInList(getAllWindows(), bottom));
    WindowInfo*infoTop=getValue(isInList(getAllWindows(), top));
    assert(info);
    assert(infoTop);
    assert(raiseWindow(bottom));
    flush();
    waitForNormalEvent(XCB_VISIBILITY_NOTIFY);
    waitForNormalEvent(XCB_VISIBILITY_NOTIFY);
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

    moveWindowToWorkspace(getValue(getActiveWindowStack()), !nonEmptyIndex);
    assert(getSize(getActiveWindowStack())==1);
    assert(getSize(getWindowStack(getWorkspaceByIndex(!nonEmptyIndex)))==1);

}END_TEST

START_TEST(test_sticky_window){
    mapArbitraryWindow();
    scan(root);
    WindowInfo*winInfo=getValue(getAllWindows());
    moveWindowToWorkspace(winInfo, -1);
    assert(hasMask(winInfo, STICKY_MASK));
    assert(hasMask(winInfo, FLOATING_MASK));
    assert(!isNotEmpty(getActiveWindowStack()));
}END_TEST
START_TEST(test_workspace_activation){
    LOAD_SAVED_STATE=0;
    mapArbitraryWindow();
    mapArbitraryWindow();
    scan(root);
    int index1=getActiveWorkspaceIndex();
    int index2=!index1;
    moveWindowToWorkspace(getValue(getActiveWindowStack()), index2);

    WindowInfo* winInfo1=getValue(getWindowStack(getWorkspaceByIndex(index1)));
    WindowInfo* winInfo2=getValue(getWindowStack(getWorkspaceByIndex(index2)));
    int info1=winInfo1->id;
    int info2=winInfo2->id;

    assert(getSize(getAllMonitors())==1);

    activateWorkspace(index2);
    assert(isWorkspaceVisible(index2));
    assert(!isWorkspaceVisible(index1));
    WAIT_UNTIL_TRUE(isWindowMapped(info2)&&!isWindowMapped(info1)&&getActiveWorkspaceIndex()==index2);

    activateWorkspace(index1);
    assert(isWorkspaceVisible(index1));
    assert(!isWorkspaceVisible(index2));
    WAIT_UNTIL_TRUE(isWindowMapped(info1)&&!isWindowMapped(info2)&&getActiveWorkspaceIndex()==index1);

    activateWindow(winInfo2);
    assert(isWindowInVisibleWorkspace(winInfo2));
    assert(!isWindowInVisibleWorkspace(winInfo1));
    WAIT_UNTIL_TRUE(isWindowMapped(info2)&&!isWindowMapped(info1)&&getActiveWorkspaceIndex()==index2);
    assert(getIntValue(getActiveWindowStack())==info2);
    activateWindow(winInfo1);
    assert(isWindowInVisibleWorkspace(winInfo1));
    assert(!isWindowInVisibleWorkspace(winInfo2));
    WAIT_UNTIL_TRUE(isWindowMapped(info1)&&!isWindowMapped(info2)&&getActiveWorkspaceIndex()==index1);
    assert(getSize(getActiveWindowStack())==1);
    assert(getIntValue(getActiveWindowStack())==info1);
}END_TEST

START_TEST(test_privileged_windows_size){
    Monitor* m=getValue(getAllMonitors());
    assert(m);
    int view[]={10,20,30,40};
    m->viewX=*view;
    WindowInfo *privilgedWindowInfo=createWindowInfo(1);
    WindowInfo *normalWindowInfo=createWindowInfo(2);
    addWindowInfo(normalWindowInfo);
    addWindowInfo(privilgedWindowInfo);
    assert(getMaxDimensionForWindow(m, normalWindowInfo, 1)==0);
    assert(getMaxDimensionForWindow(m, normalWindowInfo, 0)==0);

    addMask(privilgedWindowInfo,X_MAXIMIZED_MASK);
    assert(getMaxDimensionForWindow(m, privilgedWindowInfo, 0)==m->viewWidth);
    addMask(privilgedWindowInfo,Y_MAXIMIZED_MASK);
    assert(getMaxDimensionForWindow(m, privilgedWindowInfo, 1)==m->viewHeight);

    addMask(privilgedWindowInfo,FULLSCREEN_MASK);

    assert(getMaxDimensionForWindow(m, privilgedWindowInfo, 0)==m->width);
    assert(getMaxDimensionForWindow(m, privilgedWindowInfo, 1)==m->height);
    addMask(privilgedWindowInfo,ROOT_FULLSCREEN_MASK);
    assert(getMaxDimensionForWindow(m, privilgedWindowInfo, 0)==screen->width_in_pixels);
    assert(getMaxDimensionForWindow(m, privilgedWindowInfo, 1)==screen->height_in_pixels);
}END_TEST

START_TEST(test_tile_windows){
    LOAD_SAVED_STATE=0;
    int win[NUMBER_OF_LAYERS];
    for(int i =0;i<LEN(win);i++)
        win[i]=mapWindow(createNormalWindow());
    if(_i)
        xcb_icccm_set_wm_transient_for(dis, win[UPPER_LAYER], win[NORMAL_LAYER]);
    scan(root);
    setActiveLayout(&DEFAULT_LAYOUTS[FULL]);
    for(int i=LEN(win)-1;i>=0;i--){
        assert(raiseWindow(win[i]));
        moveWindowToLayer(getWindowInfo(win[i]), getActiveWorkspaceIndex(), i);
    }

    for(int i =0;i<LEN(win);i++)
        assert(getSize(getActiveWorkspace()->windows[i])==1);
    consumeEvents();
    tileWorkspace(getActiveWorkspaceIndex());
    for(int i=0;i<LEN(win);i++)
        waitForNormalEvent(XCB_CONFIGURE_NOTIFY);
    //TODO actually test window order

}END_TEST

START_TEST(test_empty_layout){
    LOAD_SAVED_STATE=0;
    mapWindow(createNormalWindow());
    scan(root);
    setActiveLayout(NULL);
    consumeEvents();
    tileWorkspace(getActiveWorkspaceIndex());
    assert(!xcb_poll_for_event(dis));
    Layout l={};
    setActiveLayout(&l);
    tileWorkspace(getActiveWorkspaceIndex());
    assert(!xcb_poll_for_event(dis));
    int returnFalse(){return 0;}
    void dummy(){exit(1);}
    l.layoutFunction=dummy;
    l.conditionFunction=returnFalse;
    setActiveLayout(&l);
    tileWorkspace(getActiveWorkspaceIndex());
    assert(!xcb_poll_for_event(dis));
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
    WindowInfo*winInfo=createWindowInfo(createNormalWindow());
    processNewWindow(winInfo);
    assert(isNotEmpty(getActiveWindowStack()));
    floatWindow(winInfo);
    assert(!isNotEmpty(getActiveWindowStack()));
    assert(hasMask(winInfo, FLOATING_MASK));
    sinkWindow(winInfo);
    assert(!hasMask(winInfo, FLOATING_MASK));
    assert(!hasMask(winInfo, -1));
    assert(isNotEmpty(getActiveWindowStack()));
}END_TEST

START_TEST(test_toggle_show_desktop){
    LOAD_SAVED_STATE=0;
    int win[10];
    for(int i=0;i<LEN(win);i++){
        win[i]=mapWindow(createNormalWindow());
    }
    flush();

    WAIT_UNTIL_TRUE(getSize(getAllWindows())==LEN(win));

    toggleShowDesktop();

    Node*n=getAllWindows();
    FOR_EACH(n,WAIT_UNTIL_TRUE(!hasMask(getValue(n),MAPPED_MASK)))
    toggleShowDesktop();
    n=getAllWindows();
    FOR_EACH(n,WAIT_UNTIL_TRUE(hasMask(getValue(n),MAPPED_MASK)))
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

Suite *x11Suite(void) {

    Suite*s = suite_create("Window Manager Functions");
    TCase* tc_core;

    tc_core = tcase_create("Window fields");
    tcase_add_checked_fixture(tc_core, onStartup, fullCleanup);
    tcase_add_test(tc_core, test_mask_add_remove_toggle);
    tcase_add_test(tc_core, test_mask_reset);
    tcase_add_test(tc_core, test_get_set_geometry_config);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("X Server");
    tcase_add_checked_fixture(tc_core, createSimpleContext, destroyContext);
    tcase_add_test(tc_core, test_connect_to_xserver);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Window_Detection");
    tcase_add_checked_fixture(tc_core, onStartup, destroyContextAndConnection);
    tcase_add_test(tc_core, test_window_property_loading);
    tcase_add_test(tc_core, test_window_property_reloading);
    tcase_add_test(tc_core, test_window_property_alt_loading);
    tcase_add_test(tc_core, test_window_extra_property_loading);
    tcase_add_test(tc_core, test_process_window);
    tcase_add_test(tc_core, test_window_scan);
    tcase_add_test(tc_core, test_sync_state);
    tcase_add_test(tc_core, test_window_state);
    tcase_add_test(tc_core, test_window_state_sync);
    suite_add_tcase(s, tc_core);


    tc_core = tcase_create("Layers");
    tcase_add_checked_fixture(tc_core, onStartup, destroyContextAndConnection);
    tcase_add_test(tc_core, test_layer_move);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Invalid State");
    tcase_add_checked_fixture(tc_core, onStartup, destroyContextAndConnection);
    tcase_add_test(tc_core, test_invalid_state);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("WindowOperations");
    tcase_add_checked_fixture(tc_core, onStartup, destroyContextAndConnection);

    tcase_add_test(tc_core, test_raise_window);
    tcase_add_test(tc_core, test_focus_window);
    tcase_add_test(tc_core, test_focus_window_request);
    tcase_add_loop_test(tc_core, test_delete_window_request,1,4);
    tcase_add_test(tc_core, test_activate_window);
    tcase_add_test(tc_core, test_set_border_color);
    tcase_add_test(tc_core, test_workspace_activation);
    tcase_add_test(tc_core, test_workspace_change);
    tcase_add_test(tc_core, test_sticky_window);

    tcase_add_test(tc_core, test_kill_window);
    tcase_add_test(tc_core, test_privileged_windows_size);
    tcase_add_loop_test(tc_core, test_tile_windows,0,2);
    tcase_add_test(tc_core, test_empty_layout);
    tcase_add_test(tc_core, test_configure_windows);
    tcase_add_test(tc_core, test_float_sink_window);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Window Managment Operations");
    tcase_add_checked_fixture(tc_core, setup, fullCleanup);
    tcase_add_test(tc_core, test_toggle_show_desktop);
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
