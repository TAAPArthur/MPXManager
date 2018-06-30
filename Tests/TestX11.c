/*
 * TestX11.c
 *
 *  Created on: Jun 30, 2018
 *      Author: arthur
 */

#include <check.h>
#include <err.h>

#include <stdio.h>
#include <check.h>
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <unistd.h>
#include <regex.h>

#include "../util.h"
#include "../config.h"
#include "../event-loop.h"
#include "../mywm-structs.h"
#include "../extra-functions.h"
#include "../functions.h"

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

#include "../defaults.h"
#include "../mywm-util.h"

#include "TestHelper.c"

const int size=10;

int isWindowMapped(int win){
    return isMapped(((WindowInfo*)getValue(isInList(getAllWindows(), win))));
}

START_TEST(test_window_property_loading){
    INIT
    createContext(1);
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
    printf("%s %s\n",instace, winInfo->instanceName);
    printf("%s %s\n",clazz, winInfo->className);
    assert(strcmp(instace, winInfo->instanceName)==0);
    assert(strcmp(clazz, winInfo->className)==0);

    END
}END_TEST
START_TEST(test_run_or_raise){
    INIT

    char*titles[]={"a","c","d","e"};
    int size=LEN(titles);
    Window win[size];
    for(int i=0;i<size;i++){
        win[i]=mapArbitraryWindow(dc);


        assert(xcb_request_check(dc->dis,
                xcb_ewmh_set_wm_name_checked(dc->ewmh,
                        win[i], 1, titles[i]))==NULL);
    }
    createContext(1);

    connectToXserver();
    setLayout(&DEFAULT_LAYOUTS[FULL]);
    assert(getSize(getAllWindows())==size);
    Rules fail={NULL,0,BIND_TO_INT_FUNC(exit,127)};
    for(int i=0;i<size;i++){
        Rules r[]={CREATE_RULE(titles[i],LITERAL|TITLE,NULL),{},fail,{}};
        assert(r[0].ruleTarget&LITERAL);
        int raisedWindow=runOrRaise(r);
        assert(raisedWindow);
        assert(raisedWindow==win[i]);
    }

    END
}END_TEST
START_TEST(test_workspace_change){
    INIT
    createContext(4);
    connectToXserver();
    assert(getSize(getAllWindows())==0);
    assert(getSize(getActiveWindowStack())==0);
    START_MY_WM
    assert(getSize(getActiveWindowStack())==0);
    assert(getActiveWorkspace()->id==0);
    for(int i=0;i<size*2;i++){
        mapArbitraryWindow(dc);
        if(i+1==size){
            xcb_flush(dc->dis);
            WAIT_FOR(getSize(getActiveWindowStack())<size)
            assert(getSize(getActiveWindowStack())==getSize(getAllWindows()));
            switchToWorkspace(1);
            assert(getActiveWorkspace()->id==1);
        }
    }
    assert(getActiveWorkspace()->id==1);
    xcb_flush(dc->dis);
    WAIT_FOR(getSize(getActiveWindowStack())!=size)
    Node*n=getWindowStack(getWorkspaceByIndex(0)),*n2;
    //verify all windows are distict
    FOR_EACH(n,n2=getWindowStack(getWorkspaceByIndex(1));FOR_EACH(n2,assert(getIntValue(n)!=getIntValue(n2))));

    Node*head=getActiveWindowStack();
    FOR_EACH(head,assert(isMapped(getValue(head))))
    head=getWindowStack(getWorkspaceByIndex(0));
    FOR_EACH(head,assert(!isMapped(getValue(head))))

    assert(getSize(getAllWindows())==size*2);
    assert(getSize(getWindowStack(getWorkspaceByIndex(0)))==size);
    assert(getSize(getWindowStack(getWorkspaceByIndex(1)))==size);

    assert(activateWindow(getWindowInfo(getIntValue(getWindowStack(getWorkspaceByIndex(0))))));
    assert(getActiveWorkspace()->id==0);
    assert(activateWindow(getWindowInfo(getIntValue(getWindowStack(getWorkspaceByIndex(0))->next))));
    assert(getActiveWorkspace()->id==0);
    END
}END_TEST
START_TEST(test_workspace_activation){
    INIT
    createContext(4);
    mapArbitraryWindow(dc);
    mapArbitraryWindow(dc);
    connectToXserver();
    START_MY_WM
    int index1=getActiveWorkspaceIndex();
    int index2=!index1;
    moveWindowToWorkspace(getValue(getActiveWindowStack()), index2);

    WindowInfo *info1=getValue(getWindowStack(getWorkspaceByIndex(index1)));
    WindowInfo *info2=getValue(getWindowStack(getWorkspaceByIndex(index2)));

    WAIT_FOR(isMapped(info2))
    xcb_ewmh_request_change_current_desktop(ewmh, defaultScreenNumber, index2, 0);
    WAIT_UNTIL_TRUE(isMapped(info2)&&!isMapped(info1));
    xcb_ewmh_request_change_current_desktop(ewmh, defaultScreenNumber, index1, 0);
    WAIT_UNTIL_TRUE(isMapped(info1)&&!isMapped(info2));
    xcb_ewmh_request_change_active_window(ewmh, defaultScreenNumber, info2->id, 0, 0, info1->id);

    WAIT_UNTIL_TRUE(isMapped(info2)&&!isMapped(info1)&&getActiveWorkspaceIndex()==index2);
    xcb_ewmh_request_change_active_window(ewmh, defaultScreenNumber, info1->id, 0, 0, info2->id);
    WAIT_UNTIL_TRUE(isMapped(info1)&&!isMapped(info2)&&getActiveWorkspaceIndex()==index1);
    END
}END_TEST
START_TEST(test_grab){
    INIT

    char target[]={1,1,1,1};
    char count[]={0,0,0,0};
    void KP(){count[0]++;}
    void KR(){count[1]++;}
    void MP(){count[2]++;}
    void MR(){count[3]++;}
    createContext(1);
    Binding k={0,XK_A,BIND_TO_FUNC(KP)};
    Binding k2={0,XK_B,BIND_TO_FUNC(KR)};
    Binding m={0,Button1,BIND_TO_FUNC(MP)};
    Binding m2={0,Button2,BIND_TO_FUNC(MR)};
    bindingLengths[0]=1; bindings[0]=&k;
    bindingLengths[1]=1; bindings[1]=&k2;
    bindingLengths[2]=1; bindings[2]=&m;
    bindingLengths[3]=1; bindings[3]=&m2;
    DEVICE_EVENT_MASKS|=XI_KeyPressMask|XI_KeyReleaseMask|XI_ButtonPressMask|XI_ButtonReleaseMask;
    connectToXserver();
    //START_MY_WM
    int id=getActiveMaster()->id;
    assert(k.keyCode);
    assert(k2.keyCode);
    dis=dc->dis;
    dpy=dc->dpy;

    assert(grabButton(id, m.detail, m.mod, XI_ButtonPressMask));
    assert(grabButton(id, m2.detail, m2.mod, XI_ButtonReleaseMask));

    assert(grabKey(id, k.keyCode, k.mod, XI_KeyPressMask));
    assert(grabKey(id, k.keyCode, k.mod, XI_KeyReleaseMask));

    END
}END_TEST


START_TEST(test_detect_existing_windows){
    INIT
    int win=createArbitraryWindow(dc);
    int win2=createArbitraryWindow(dc);
    int win3=createArbitraryWindow(dc);
    mapWindow(dc, win);
    createContext(1);
    connectToXserver();
    assert(isInList(getAllWindows(), win));
    assert(isInList(getAllWindows(), win2));
    assert(isInList(getAllWindows(), win3));
    assert(isWindowMapped(win));
    assert(!isWindowMapped(win2));
    assert(!isWindowMapped(win3));
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
    distroyWindow(dc,win);
    mapWindow(dc, win2);
    START_MY_WM

    int win4=createArbitraryWindow(dc);

    WAIT_FOR(!isInList(getAllWindows(), win4)||
            !isInList(getAllWindows(), win2)||
            !isInList(getAllWindows(), win3))

    distroyWindow(dc,win2);
    distroyWindow(dc,win3);
    distroyWindow(dc,win4);

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
    //mapWindow(dc,win);
    //wait for all to be mapped
    WAIT_UNTIL_TRUE(isWindowMapped(win)&&isWindowMapped(win2)&&isWindowMapped(win3));

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

Window createArbitraryStruct(DisplayConnection* dc,int i){
    Window win=createArbitraryWindow(dc);

    xcb_ewmh_wm_strut_partial_t strut={
            0,0,0,0,
            0,size,
            0,size,
            0,size,
            0,size
    };
    ((int*)&strut)[i]=size;
    xcb_ewmh_wm_strut_partial_t strut2;
    xcb_ewmh_set_wm_strut_partial_checked(dc->ewmh, win, strut);
    ;
    assert(xcb_ewmh_get_wm_strut_partial_reply(dc->ewmh,
            xcb_ewmh_get_wm_strut_partial(dc->ewmh, win), &strut2, NULL));
    for(int i=0;i<12;i++)
        assert(((int*)&strut)[i]==((int*)&strut2)[i]);

    xcb_atom_t atom[]={dc->ewmh->_NET_WM_WINDOW_TYPE_DOCK};
    if(xcb_request_check(dc->dis,xcb_ewmh_set_wm_window_type_checked(dc->ewmh, win, 1, atom))){
        printf("could not set window type\n");
        assert(0);
    }

    return win;
}




START_TEST(test_avoid_docks){
    INIT
    int window = createArbitraryStruct(dc,DOCK_TOP);
    xcb_get_geometry_reply_t*reply=xcb_get_geometry_reply(dc->dis, xcb_get_geometry(dc->dis, window), NULL);
    createContext(1);
    onStartup();
    assert(getSize(eventRules[ProcessingWindow])==0);
    ADD_AVOID_STRUCTS_RULE;
    Rules *r=getValue(eventRules[ProcessingWindow]);
    assert(r->ruleTarget & LITERAL);
    //should detect dock
    connectToXserver();
    assert(getSize(getAllDocks())==1);
    Monitor*m=getValue(getAllMonitors());
    short int rect[]={m->viewX,m->viewY,m->viewWidth,m->viewHeight};
    assert(intersects(rect,&reply->x)==0);
    START_MY_WM
    //rule got erased with above command
    ADD_AVOID_STRUCTS_RULE;
    window = createArbitraryStruct(dc,DOCK_LEFT);
    xcb_get_geometry_reply_t*reply2=xcb_get_geometry_reply(dc->dis, xcb_get_geometry(dc->dis, window), NULL);
    assert(eventRules[ProcessingWindow]->prev==NULL);
    assert(getSize(eventRules[ProcessingWindow])>0);
    WAIT_FOR(getSize(getAllDocks())<2)
    assert(intersects(rect,&reply->x)==0);
    assert(intersects(rect,&reply2->x)==0);
    END
}END_TEST

START_TEST(test_auto_tile){
    INIT
    Window win[size];
    int count=0;
    void fakeLayout(Monitor*m,Node*n,int*i){
        count++;
    }
    DEFAULT_LAYOUTS[0].layoutFunction=fakeLayout;
    for(int i=0;i<size;i++){
        win[i]=createArbitraryWindow(dc);
        mapWindow(dc, win[i]);
    }
    createContext(1);
    onStartup();
    connectToXserver();
    assert(count!=0);
    END
}END_TEST

START_TEST(test_layouts){
    INIT
    createContext(1);
    onStartup();
    connectToXserver();
    Monitor*m=getValue(getAllMonitors());
    int targetArea=m->viewWidth*m->viewHeight;
    if(_i==LAST_LAYOUT){
        setDefaultWindowMasks(FULLSCREEN_MASK);
        targetArea=m->width*m->height;
        _i=0;
    }
    getWorkspaceByIndex(0)->activeLayout=&DEFAULT_LAYOUTS[_i];
    START_MY_WM

    int area;
    xcb_get_geometry_reply_t*reply1,*reply2;
    for(int i=1;i<=size;i++){
        //printf("round %d \n\n;",i);
        mapWindow(dc, createArbitraryWindow(dc));
        WAIT_FOR(getNumberOfTileableWindows(getActiveWindowStack())<i)

        do{
            assert(getNumberOfTileableWindows(getActiveWindowStack())==i);
            assert(getNumberOfTileableWindows(getActiveWindowStack())==getSize(getActiveWindowStack()));
            Node*p1=getActiveWindowStack();
            area=0;
            FOR_EACH(p1,
                reply1=xcb_get_geometry_reply(dc->dis, xcb_get_geometry(dc->dis, getIntValue(p1)), NULL);
                if(isWindowVisible(getIntValue(p1))){

                    area+=reply1->width*reply1->height;
                }
                //printf("%d %d %d %d -> %d\n",reply1->x,reply1->y,reply1->width,reply1->height,area);
            )
            msleep(10);

            //printf("\n%d,%d %d  diff:%d\n",i,area,m->viewWidth*m->viewHeight,area-m->viewWidth*m->viewHeight);

        }while(area!=targetArea);
    }
    Node*p1=getActiveWindowStack();
    FOR_EACH(p1,
        if(isWindowVisible(getIntValue(p1))){
            reply1=xcb_get_geometry_reply(dc->dis, xcb_get_geometry(dc->dis, getIntValue(p1)), NULL);
            Node*p2=p1->next;
            if(p2==NULL)break;
            FOR_EACH(p2,
                if(isWindowVisible(getIntValue(p2))){
                    reply2=xcb_get_geometry_reply(dc->dis, xcb_get_geometry(dc->dis, getIntValue(p2)), NULL);
                    assert(intersects(&reply1->x, &reply2->x)==0);
                }
            )
        }
    )
    printf("finished\n\n;");
    KILL_MY_WM
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


Suite *xunit_suite(void) {
    Suite*s = suite_create("My Window Manager");
    TCase* tc_core = tcase_create("MISC");
    tcase_add_test(tc_core, test_window_property_loading);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Bindings");
    tcase_add_test(tc_core, test_grab);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Window detection");
    tcase_set_timeout(tc_core, 10);
    tcase_add_test(tc_core, test_no_windows);
    tcase_add_test(tc_core, test_detect_existing_windows);
    tcase_add_test(tc_core, test_detect_new_windows);
    tcase_add_test(tc_core, test_map_windows);
    tcase_add_test(tc_core, test_delete_windows);
    tcase_add_test(tc_core, test_avoid_docks);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Functions");
    //tcase_set_timeout(tc_core, 10);
    tcase_add_test(tc_core, test_run_or_raise);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Window Operations");
    tcase_add_test(tc_core, test_workspace_change);
    tcase_add_test(tc_core, test_workspace_activation);

    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Layouts");

    tcase_add_test(tc_core, test_privileged_windows_size);
    tcase_add_test(tc_core, test_auto_tile);
    tcase_add_loop_test(tc_core,test_layouts,0,LAST_LAYOUT+1);
    suite_add_tcase(s, tc_core);


    return s;
}
void handler(int sig) {
    msleep(100);
    void *array[30];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 30);

    // print out all the frames to stderr
    LOG(LOG_LEVEL_ERROR, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}
int main(void) {
  int no_failed = 0;
  signal(SIGSEGV, handler);   // install our handler
  signal(SIGABRT, handler);   // install our handler
  signal(SIGFPE, handler);   // install our handler
  SRunner *runner;
  Suite *s;

  setLogLevel(LOG_LEVEL_ERROR);
  s = xunit_suite();
  runner = srunner_create(s);

  srunner_run_all(runner, CK_NORMAL);
  no_failed = srunner_ntests_failed(runner);
  srunner_free(runner);
  return (no_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
