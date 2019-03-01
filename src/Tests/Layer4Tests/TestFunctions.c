#include "../UnitTests.h"
#include "../TestX11Helper.h"
#include "../../layouts.h"
#include "../../wmfunctions.h"
#include "../../functions.h"
#include "../../devices.h"
#include "../../masters.h"


static WindowInfo* top;
static WindowInfo* bottom;
void functionSetup(){
    LOAD_SAVED_STATE = 0;
    onStartup();
    int size = 10;
    for(int i = 0; i < size; i++)
        mapWindow(createNormalWindow());
    scan(root);
    top = getHead(getActiveWindowStack());
    bottom = getLast(getActiveWindowStack());
}

START_TEST(test_cycle_window){
    IGNORE_KEY_REPEAT = 1;
    AUTO_FOCUS_NEW_WINDOW_TIMEOUT = -1;
    START_MY_WM
    int size = 4;
    int win[size];
    for(int i = 0; i < size; i++)
        win[i] = mapWindow(createNormalWindow());
    int hidden = mapWindow(createNormalWindow());
    WAIT_UNTIL_TRUE(getSize(getAllWindows()) == size + 1);
    addMask(getWindowInfo(hidden), HIDDEN_MASK);
    FOR_EACH_REVERSED(WindowInfo * winInfo, getAllWindows(), if(isInteractable(winInfo))focusWindow(winInfo->id));
    mapWindow(createNormalWindow());
    WAIT_UNTIL_TRUE(getSize(getAllWindows()) == size + 2);
    WAIT_UNTIL_TRUE(getSize(getActiveMasterWindowStack()) == size);
    lock();
    assert(win[0] == getFocusedWindow()->id);
    focusWindowInfo(getWindowInfo(win[1]));
    unlock();
    WAIT_UNTIL_TRUE(win[1] == getFocusedWindow()->id);
    lock();
    moveWindowToWorkspace(getWindowInfo(win[0]), !getActiveWorkspaceIndex());
    activateWindow(getWindowInfo(win[0]));
    flush();
    unlock();
    WAIT_UNTIL_TRUE(win[0] == getFocusedWindow()->id);
    for(int i = 0; i < size * 2; i++){
        int newFocusedWindow = win[(i + 1) % size];
        lock();
        cycleWindows(DOWN);
        assert(isFocusStackFrozen());
        flush();
        unlock();
        WAIT_UNTIL_TRUE(getFocusedWindow()->id == newFocusedWindow);
    }
    endCycleWindows();
    assert(!isFocusStackFrozen());
}
END_TEST

void assertWindowIsFocused(int win){
    xcb_input_xi_get_focus_reply_t* reply = xcb_input_xi_get_focus_reply(dis, xcb_input_xi_get_focus(dis,
                                            getActiveMasterKeyboardID()), NULL);
    assert(reply->focus == win);
    free(reply);
}

START_TEST(test_find_and_raise){
    char* titles[] = {"a", "c", "d", "e"};
    char* fakeTitle = "x";
    int size = LEN(titles);
    Window win[size];
    for(int n = 0; n < 2; n++){
        for(int i = 0; i < size; i++){
            int temp = mapWindow(createNormalWindow());;
            if(n == 0)win[i] = temp;
            assert(xcb_request_check(dis,
                                     xcb_ewmh_set_wm_name_checked(ewmh,
                                             win[i], 1, titles[i])) == NULL);
            processNewWindow(createWindowInfo(temp));
        }
    }
    for(int i = 0; i < size * 2; i++){
        Rule r = CREATE_RULE(titles[i % size], LITERAL | TITLE, NULL);
        int id = findAndRaise(&r);
        assert(getWindowInfo(id));
        assert(strcmp(getWindowInfo(id)->title, titles[i % size]) == 0);
        if(i < size){
            assert(id == findAndRaiseLazy(&r));
            assert(id == win[i % size]);
        }
        else if(i == size - 1){
            assert(!isInList(getActiveMasterWindowStack(), id));
            assert(id != win[i % size]);
            continue;
        }
        flush();
        assertWindowIsFocused(id);
        onWindowFocus(id);
    }
    Rule r = CREATE_RULE(fakeTitle, LITERAL | TITLE, NULL);
    assert(!findAndRaise(&r));
    assert(!findAndRaiseLazy(&r));
}
END_TEST
/*
 *     setLayout(&DEFAULT_LAYOUTS[FULL]);

    START_MY_WM
    WAIT_UNTIL_TRUE(getSize(getAllWindows())==size);

    Rule fail={NULL,0,BIND(exit,127)};
    Rule r=RUN_OR_RAISE_TITLE(titles[0],"");
    for(int i=0;i<size;i++){
        Rule r[]={CREATE_RULE(titles[i],LITERAL|TITLE,NULL),{},fail,{}};
        assert(r[0].ruleTarget&LITERAL);
        int raisedWindow=findAndRaise(r);
        assert(raisedWindow);
        assert(raisedWindow==win[i]);
    }
 */

START_TEST(test_spawn){
    spawn("exit 122");
    assert(getExitStatusOfFork() == 122);
}
END_TEST

START_TEST(test_get_next_window_in_stack_fail){
    FOR_EACH(WindowInfo * winInfo, getAllWindows(), addMask(winInfo, HIDDEN_MASK));
    assert(getNextWindowInStack(0) == NULL);
    assert(getNextWindowInStack(1) == NULL);
    assert(getNextWindowInStack(-1) == NULL);
}
END_TEST
START_TEST(test_get_next_window_in_stack){
    addMask(bottom, HIDDEN_MASK);
    onWindowFocus(top->id);
    assert(getFocusedWindow() == top);
    assert(getNextWindowInStack(0) == top);
    assert(getNextWindowInStack(DOWN) == getElement(getActiveWindowStack(), 1));
    //bottom is marked hidden
    assert(getNextWindowInStack(UP) != bottom);
    assert(getNextWindowInStack(UP) == getElement(getActiveWindowStack(), getSize(getActiveWindowStack()) - 2));
    assert(getFocusedWindow() == top);
    switchToWorkspace(!getActiveWorkspaceIndex());
    assert(getNextWindowInStack(0) != getFocusedWindow());
}
END_TEST

START_TEST(test_activate_top_bottom){
    assert(focusBottom());
    assertWindowIsFocused(bottom->id);
    assert(focusTop());
    assertWindowIsFocused(top->id);
    assert(focusBottom());
    assertWindowIsFocused(bottom->id);
}
END_TEST
START_TEST(test_shift_top){
    onWindowFocus(bottom->id);
    shiftTop();
    assert(getHead(getActiveWindowStack()) == bottom);
    assert(getElement(getActiveWindowStack(), 1) == top);
}
END_TEST
START_TEST(test_shift_focus){
    FOR_EACH(WindowInfo * winInfo, getActiveWindowStack(),
             onWindowFocus(winInfo->id);
             int id = shiftFocus(1);
             assert(id);
             assertWindowIsFocused(id);
             onWindowFocus(id);
            )
}
END_TEST
START_TEST(test_swap_top){
    onWindowFocus(bottom->id);
    swapWithTop();
    assert(getHead(getActiveWindowStack()) == bottom);
    assert(getLast(getActiveWindowStack()) == top);
}
END_TEST


START_TEST(test_swap_position){
    onWindowFocus(bottom->id);
    swapPosition(1);
    assert(getHead(getActiveWindowStack()) == bottom);
    assert(getLast(getActiveWindowStack()) == top);
}
END_TEST



START_TEST(test_mouse_lock){
    volatile int count = 0;
    WindowInfo* dummy;
    void test(WindowInfo * winInfo){
        if(winInfo == dummy)count++;
    }
    Binding binding = {0, Button1, BIND(test), .mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS};
    START_MY_WM
    lock();
    dummy = createWindowInfo(1);
    addWindowInfo(dummy);
    clearList(getDeviceBindings());
    initBinding(&binding);
    grabBinding(&binding);
    addBinding(&binding);
    startMouseOperation(dummy);
    triggerBinding(&binding);
    flush();
    int idle = getIdleCount();
    unlock();
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    assert(count);
    lock();
    stopMouseOperation(dummy);
    triggerBinding(&binding);
    flush();
    idle = getIdleCount();
    count = 0;
    unlock();
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    assert(count == 0);
}
END_TEST

START_TEST(test_activate_workspace_with_mouse){
    assert(getSize(getAllMonitors()) == 1);
    Monitor* m = getHead(getAllMonitors());
    Rect bounds = m->base;
    bounds.x += bounds.width;
    updateMonitor(1, 0, bounds);
    setLastKnowMasterPosition(bounds.x + 1, bounds.y + 1);
    activateWorkspaceUnderMouse();
    assert(m != getMonitorFromWorkspace(getActiveWorkspace()));
}
END_TEST

START_TEST(test_move_resize_window_with_mouse){
    int masterPos[] = {210, 200};
    int masterPosDelta[] = {15, 25};
    floatWindow(top);
    short geo[] = {100, 100, 100, 100};
    if(_i == 2){
        masterPosDelta[0] = -200;
        masterPosDelta[1] = -200;
    }
    setGeometry(top, geo);
    startMouseOperation(top);
    setLastKnowMasterPosition(masterPos[0], masterPos[1]);
    setLastKnowMasterPosition(masterPos[0] + masterPosDelta[0], masterPos[1] + masterPosDelta[1]);
    if(_i % 2 == 0) resizeWindowWithMouse(top);
    else  moveWindowWithMouse(top);
    xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, top->id), NULL);
    if(_i == 0){
        assert(reply->width == geo[2] + masterPosDelta[0]);
        assert(reply->height == geo[3] + masterPosDelta[1]);
    }
    else if(_i == 1){
        assert(reply->x == geo[0] + masterPosDelta[0]);
        assert(reply->y == geo[1] + masterPosDelta[1]);
    }
    free(reply);
}
END_TEST

START_TEST(test_send_to_workspace_by_name){
    char* names[] = {"W1", "W2"};
    setActiveWorkspaceIndex(0);
    setWorkspaceNames(names, 2);
    sendWindowToWorkspaceByName(top, names[1]);
    assert(!isInList(getActiveWindowStack(), top->id));
    assert(isInList(getWindowStack(getWorkspaceByIndex(1)), top->id));
    assert(!isInList(getWindowStack(getWorkspaceByIndex(1)), bottom->id));
    assert(isInList(getActiveWindowStack(), bottom->id));
    //TODO trying to get the index by name with an invalid name is undefined behavior
    getIndexFromName("");
}
END_TEST


START_TEST(test_function_bindings){
    int arbitNum = 1001;
    Binding bindings[] = {STACK_OPERATION(arbitNum, arbitNum, arbitNum, arbitNum)};
    for(int i = 0; i < LEN(bindings); i++){
        int intArg = bindings[i].boundFunction.arg.intArg;
        assert(intArg == UP || intArg == DOWN || intArg == 0);
    }
}
END_TEST

Suite* functionsSuite(){
    Suite* s = suite_create("Functions");
    TCase* tc_core;
    tc_core = tcase_create("Function Bindings");
    tcase_add_test(tc_core, test_function_bindings);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Helper_Functions");
    tcase_add_checked_fixture(tc_core, functionSetup, fullCleanup);
    tcase_add_test(tc_core, test_get_next_window_in_stack);
    tcase_add_test(tc_core, test_get_next_window_in_stack_fail);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Simple functions");
    tcase_add_checked_fixture(tc_core, functionSetup, fullCleanup);
    tcase_add_test(tc_core, test_activate_top_bottom);
    tcase_add_test(tc_core, test_shift_top);
    tcase_add_test(tc_core, test_shift_focus);
    tcase_add_test(tc_core, test_swap_top);
    tcase_add_test(tc_core, test_swap_position);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Workspace_Functions");
    tcase_add_checked_fixture(tc_core, functionSetup, fullCleanup);
    tcase_add_test(tc_core, test_send_to_workspace_by_name);
    tcase_add_test(tc_core, test_activate_workspace_with_mouse);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Mouse_Functions");
    tcase_add_checked_fixture(tc_core, functionSetup, fullCleanup);
    tcase_add_loop_test(tc_core, test_move_resize_window_with_mouse, 0, 3);
    tcase_add_test(tc_core, test_mouse_lock);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Complex_Functions");
    tcase_add_checked_fixture(tc_core, onStartup, fullCleanup);
    tcase_add_test(tc_core, test_cycle_window);
    tcase_add_test(tc_core, test_find_and_raise);
    tcase_add_test(tc_core, test_spawn);
    suite_add_tcase(s, tc_core);
    return s;
}
