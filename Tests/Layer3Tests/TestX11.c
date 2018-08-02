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

#include "../../defaults.h"
#include "../../devices.h"
#include "../../default-rules.h"
#include "../../window-properties.h"
#include "../../mywm-util.h"
#include "../../logger.h"
#include "../../event-loop.h"
#include "../../wmfunctions.h"

#include "../UnitTests.h"
#include "../TestX11Helper.h"

int isWindowMapped(DisplayConnection* dc,int win){
    xcb_get_window_attributes_reply_t *reply;

    reply=xcb_get_window_attributes_reply(dc->dis, xcb_get_window_attributes(dc->dis, win),NULL);


    return reply->map_state!=XCB_MAP_STATE_UNMAPPED;
}

START_TEST(test_window_property_loading){
    INIT
    createContext(1);
    onStartup();
    connectToXserver();
    int win=createArbitraryWindow(dc);


    char*classInstance="Instance\0Class";
    char*clazz="Class";
    char*instace="Instance";
    char*title="Title";
    char*typeName="_NET_WM_WINDOW_TYPE_DOCK";
    int s=strlen(classInstance)+1;
    s+=strlen(&classInstance[s]);
    xcb_atom_t atom[]={dc->ewmh->_NET_WM_WINDOW_TYPE_DOCK};
    setProperties(dc,win,classInstance,title,atom);

    WindowInfo*winInfo=createWindowInfo(win);
    loadWindowProperties(winInfo);
    assert(winInfo->className);
    assert(winInfo->instanceName);
    assert(winInfo->title);
    assert(winInfo->typeName);
    assert(winInfo->type);

    assert(strcmp(title, winInfo->title)==0);


    assert(strcmp(typeName, winInfo->typeName)==0);
    assert(atom[0]== winInfo->type);
    assert(strcmp(instace, winInfo->instanceName)==0);
    assert(strcmp(clazz, winInfo->className)==0);

    END
}END_TEST

START_TEST(test_focus_window){
    INIT
    createContext(1);
    int win=mapArbitraryWindow(dc);
    connectToXserver();
    focusWindow(win);
    xcb_input_xi_get_focus_reply_t*reply=xcb_input_xi_get_focus_reply(dis,
            xcb_input_xi_get_focus(dis, getActiveMasterKeyboardID()), NULL);
    assert(reply->focus==win);

}END_TEST

START_TEST(test_raise_window){
    INIT
    //windows are in same spot
    createContext(1);
    NON_ROOT_EVENT_MASKS|=XCB_EVENT_MASK_VISIBILITY_CHANGE;
    assert(isNotEmpty(eventRules[XCB_VISIBILITY_NOTIFY]));
    int bottom=mapArbitraryWindow(dc);
    int top=mapArbitraryWindow(dc);
    connectToXserver();

    WindowInfo*info=getValue(isInList(getAllWindows(), bottom));
    WindowInfo*infoTop=getValue(isInList(getAllWindows(), top));
    assert(info);
    assert(infoTop);
    assert(raiseWindow(bottom));
    START_MY_WM
    WAIT_UNTIL_TRUE(isWindowVisible(info));
    WAIT_UNTIL(isWindowVisible(infoTop));
    END
}END_TEST
START_TEST(test_workspace_change){
    INIT
    int size=2;
    createContext(size);
    mapArbitraryWindow(dc);
    mapArbitraryWindow(dc);
    connectToXserver();
    assert(getSize(getAllWindows())==2);
    assert(getSize(getActiveWindowStack())==2);

    for(int i=0;i<size;i++){
        switchToWorkspace(i);
        if(!i)continue;
        assert(getActiveWorkspace()->id==i);
        assert(getSize(getActiveWindowStack())==0);
    }
    switchToWorkspace(0);
    moveWindowToWorkspace(getValue(getActiveWindowStack()), 1);
    assert(getSize(getActiveWindowStack())==1);
    assert(getSize(getWindowStack(getWorkspaceByIndex(1)))==1);

    END
}END_TEST
START_TEST(test_workspace_activation){
    INIT
    createContext(4);
    mapArbitraryWindow(dc);
    mapArbitraryWindow(dc);
    connectToXserver();

    int index1=getActiveWorkspaceIndex();
    int index2=!index1;
    moveWindowToWorkspace(getValue(getActiveWindowStack()), index2);

    WindowInfo* winInfo1=getValue(getWindowStack(getWorkspaceByIndex(index1)));
    WindowInfo* winInfo2=getValue(getWindowStack(getWorkspaceByIndex(index2)));
    int info1=winInfo1->id;
    int info2=winInfo2->id;


    activateWorkspace(index2);
    WAIT_UNTIL_TRUE(isWindowMapped(dc,info2)&&!isWindowMapped(dc,info1)&&getActiveWorkspaceIndex()==index2);
    activateWorkspace(index1);
    WAIT_UNTIL_TRUE(isWindowMapped(dc,info1)&&!isWindowMapped(dc,info2)&&getActiveWorkspaceIndex()==index1);
    activateWindow(winInfo2);
    WAIT_UNTIL_TRUE(isWindowMapped(dc,info2)&&!isWindowMapped(dc,info1)&&getActiveWorkspaceIndex()==index2);
    assert(getIntValue(getActiveWindowStack())==info2);
    activateWindow(winInfo1);
    WAIT_UNTIL_TRUE(isWindowMapped(dc,info1)&&!isWindowMapped(dc,info2)&&getActiveWorkspaceIndex()==index1);
    assert(getSize(getActiveWindowStack())==1);
    assert(getIntValue(getActiveWindowStack())==info1);
    END
}END_TEST
START_TEST(test_workspace_resume){
    INIT

    createContext(4);
    int win=mapArbitraryWindow(dc);
    int activeWorkspaceIndex=2;
    int otherIndex=3;
    //no wm is running we we set properties directly
    assert(xcb_request_check(dc->dis, xcb_ewmh_set_current_desktop_checked(dc->ewmh, defaultScreenNumber, activeWorkspaceIndex))==NULL);
    assert(xcb_request_check(dc->dis,xcb_ewmh_set_wm_desktop(dc->ewmh, win, otherIndex))==NULL);
    //setLogLevel(LOG_LEVEL_TRACE);
    connectToXserver();
    assert(isInList(getAllWindows(),win));
    assert(getActiveWorkspaceIndex()==activeWorkspaceIndex);
    assert(!isNotEmpty(getActiveWindowStack()));
    assert(getSize(getWindowStack(getWorkspaceByIndex(otherIndex)))==1);
    assert(((WindowInfo*)getValue(isInList(getAllWindows(), win)))->workspaceIndex==otherIndex);

    activateWindow(getValue(isInList(getAllWindows(), win)));
    assert(getActiveWorkspaceIndex()==otherIndex);
    END
}END_TEST

START_TEST(test_no_windows){
    INIT
    createContext(1);
    connectToXserver();
    assert(getActiveMaster()!=NULL);
    assert(getActiveWorkspace()!=NULL);
    assert(getSize(getMasterWindowStack())==0);
    assert(getSize(getAllWindows())==0);
    Monitor*m=getValue(getAllMonitors());
    assert(m!=NULL);

    END
}END_TEST

START_TEST(test_detect_existing_windows){
    INIT
    int win=createArbitraryWindow(dc);
    int win2=createArbitraryWindow(dc);
    int win3=createArbitraryWindow(dc);
    assert(!isWindowMapped(dc,win));
    assert(!isWindowMapped(dc,win2));
    assert(!isWindowMapped(dc,win3));
    mapWindow(dc, win);

    createContext(1);
    connectToXserver();
    assert(isInList(getAllWindows(), win));
    assert(isInList(getAllWindows(), win2));
    assert(isInList(getAllWindows(), win3));
    assert(isWindowMapped(dc,win));
    assert(!isWindowMapped(dc,win2));
    assert(!isWindowMapped(dc,win3));

    END
}END_TEST
START_TEST(test_detect_new_windows){
    INIT

    createContext(1);

    connectToXserver();
    int win=createArbitraryWindow(dc);
    int win2=createArbitraryWindow(dc);
    START_MY_WM
    int win3=createArbitraryWindow(dc);
    mapWindow(dc, win);
    WAIT_FOR(!isInList(getAllWindows(), win)||
        !isInList(getAllWindows(), win2)||
        !isInList(getAllWindows(), win3))
    END
}END_TEST

START_TEST(test_delete_windows){
    INIT
    int win=createArbitraryWindow(dc);
    int win2=createArbitraryWindow(dc);
    int win3=createArbitraryWindow(dc);
    createContext(1);
    connectToXserver();


    destroyWindow(dc->dis,win);
    mapWindow(dc, win2);
    START_MY_WM

    int win4=createArbitraryWindow(dc);

    WAIT_FOR(!isInList(getAllWindows(), win4)||
            !isInList(getAllWindows(), win2)||
            !isInList(getAllWindows(), win3))

    destroyWindow(dc->dis,win2);
    destroyWindow(dc->dis,win3);
    destroyWindow(dc->dis,win4);

    WAIT_FOR(isInList(getAllWindows(), win)||
                isInList(getAllWindows(), win2)||
                isInList(getAllWindows(), win3)||
                isInList(getAllWindows(), win4))

    END
}END_TEST

START_TEST(test_map_windows){
    INIT

    int win=createArbitraryWindow(dc);
    createContext(1);
    connectToXserver();
    int win2=createArbitraryWindow(dc);
    START_MY_WM
    int win3=createArbitraryWindow(dc);

    mapWindow(dc,win3);
    mapWindow(dc,win2);


    //wait for all to be in list
    WAIT_UNTIL_TRUE(isInList(getAllWindows(), win)&&
                isInList(getAllWindows(), win2)&&
                isInList(getAllWindows(), win3))
    //WAIT_UNTIL_TRUE(!isWindowMapped(win)&&isWindowMapped(win2)&&isWindowMapped(win3));
    mapWindow(dc,win);
    //wait for all to be mapped
    WAIT_UNTIL_TRUE(isWindowMapped(dc,win)&&isWindowMapped(dc,win2)&&isWindowMapped(dc,win3));

    END
}END_TEST



Window createDock(DisplayConnection* dc,int i,int size,int full){
    Window win=createArbitraryWindow(dc);
    assert(i>=0);
    assert(i<4);
    int strut[12]={0};
    strut[i]=size;
    //strut[i*2+4+1]=0;

    xcb_void_cookie_t cookie=!full?
            xcb_ewmh_set_wm_strut_partial_checked(dc->ewmh, win, *((xcb_ewmh_wm_strut_partial_t*)strut)):
            xcb_ewmh_set_wm_strut_checked(ewmh, win, strut[0], strut[1], strut[2], strut[3]);

    assert(xcb_request_check(dc->dis, cookie)==NULL);

    xcb_atom_t atom[]={dc->ewmh->_NET_WM_WINDOW_TYPE_DOCK};
    assert(!xcb_request_check(
            dc->dis,xcb_ewmh_set_wm_window_type_checked(dc->ewmh, win, 1, atom)));

    return win;
}



START_TEST(test_dock_detection){
    INIT
    createContext(1);
    assert(getSize(eventRules[ProcessingWindow])==0);
    ADD_AVOID_STRUCTS_RULE;
    int size=10;
    int win[8];
    int num;
    for(int n=0;n<2;n++){
        int num=(n+1)*4;
        if(n){
            START_MY_WM
        }
        for(int i=num-4;i<num+4;i++)
            win[i]=createDock(dc,i%4,size,_i);
        if(n==0)
            connectToXserver();

        for(int i=0;i<num;i++){
            WAIT_UNTIL_TRUE(isInList(getAllDocks(), win[i]));
            WindowInfo*info=getValue(isInList(getAllDocks(), win[i]));
            int type=i%4;
            assert(info);
            assert(info->properties[type]);
            for(int j=0;j<4;j++)
                if(j!=type)
                    assert(info->properties[j]==0);
        }
    }
    exit(0);
    END
}END_TEST

START_TEST(test_avoid_docks){
    INIT

    createContext(1);
    assert(getSize(eventRules[ProcessingWindow])==0);
    ADD_AVOID_STRUCTS_RULE;
    Rules *r=getValue(eventRules[ProcessingWindow]);
    assert(r->ruleTarget & LITERAL);
    int size=10;
    for(int i=0;i<4;i++)
        createDock(dc,i,size,0);
    //should detect dock
    connectToXserver();
    assert(getSize(getAllDocks())==4);
    Monitor*m=getValue(getAllMonitors());

    short arr[4][4]={
            {0,0,size,m->height},//left
            {m->width-size,0,size,m->height},
            {0,0,m->width,size},//top
            {0,m->height-size,m->width,size}
    };

    for(int i=0;i<4;i++)
        assert(intersects(&m->viewX,arr[i])==0);

    START_MY_WM
    assert(getSize(eventRules[ProcessingWindow])>0);
    for(int i=0;i<4;i++)
        assert(intersects(&m->viewX,arr[i])==0);


    WAIT_FOR(getSize(getAllDocks())<2)
    END
}END_TEST

START_TEST(test_auto_tile){
    INIT
    int size=2;
    Window win[size];
    int count=0;
    void fakeLayout(Monitor*m,Node*n,int*i){
        count++;
    }
    Layout layout;
    layout.layoutFunction=fakeLayout;
    for(int i=0;i<size;i++){
        win[i]=createArbitraryWindow(dc);
        mapWindow(dc, win[i]);
    }
    createContext(1);
    addLayoutsToWorkspace(0, &layout, 1);
    setLogLevel(0);
    connectToXserver();
    assert(isNotEmpty(eventRules[onXConnection]));
    printf("%d\n",count);
    assert(count==1);
    END
}END_TEST



START_TEST(test_privileged_windows_size){
    createContext(1);
    connectToXserver();
    Monitor* m=getValue(getAllMonitors());
    int view[]={10,20,30,40};
    m->viewX=*view;

    WindowInfo *privilgedWindowInfo=createWindowInfo(1);
    WindowInfo *normalWindowInfo=createWindowInfo(1);
    addState(privilgedWindowInfo,FULLSCREEN_MASK);
    assert(getMaxDimensionForWindow(m, normalWindowInfo, 1)==0);
    assert(getMaxDimensionForWindow(m, normalWindowInfo, 0)==0);
    assert(getMaxDimensionForWindow(m, privilgedWindowInfo, 0)==m->width);
    assert(getMaxDimensionForWindow(m, privilgedWindowInfo, 1)==m->height);
    addState(privilgedWindowInfo,ROOT_FULLSCREEN_MASK);
    assert(getMaxDimensionForWindow(m, privilgedWindowInfo, 0)==screen->width_in_pixels);
    assert(getMaxDimensionForWindow(m, privilgedWindowInfo, 1)==screen->height_in_pixels);


}END_TEST



Suite *x11Suite(void) {
    setLogLevel(LOG_LEVEL_WARN);
    onStartup();
    Suite*s = suite_create("My Window Manager");
    TCase* tc_core = tcase_create("X");
    tc_core = tcase_create("Window properties");
    tcase_add_test(tc_core, test_window_property_loading);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Window detection");
    //tcase_set_timeout(tc_core, 10);

    tcase_add_test(tc_core, test_no_windows);
    tcase_add_test(tc_core, test_detect_existing_windows);
    tcase_add_test(tc_core, test_detect_new_windows);
    tcase_add_test(tc_core, test_map_windows);
    tcase_add_test(tc_core, test_delete_windows);
    tcase_add_test(tc_core, test_workspace_resume);
    tcase_add_test(tc_core, test_dock_detection);
    tcase_add_test(tc_core, test_avoid_docks);
    suite_add_tcase(s, tc_core);


    tc_core = tcase_create("Window Operations");
    tcase_add_test(tc_core, test_raise_window);
    tcase_add_test(tc_core, test_focus_window);
    tcase_add_test(tc_core, test_workspace_change);
    tcase_add_test(tc_core, test_workspace_activation);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Tiling");
    //tcase_set_timeout(tc_core, 10);
    tcase_add_test(tc_core, test_privileged_windows_size);
    tcase_add_test(tc_core, test_auto_tile);
    suite_add_tcase(s, tc_core);

    return s;
}
