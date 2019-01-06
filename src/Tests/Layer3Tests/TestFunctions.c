#include "../UnitTests.h"
#include "../TestX11Helper.h"
#include "../../layouts.h"
#include "../../wmfunctions.h"
#include "../../functions.h"


static WindowInfo*top;
static WindowInfo*bottom;
void functionSetup(){
    LOAD_SAVED_STATE=0;
    onStartup();
    int size=10;
    for(int i=0;i<size;i++)
        mapWindow(createNormalWindow());
    scan(root);
    top=getValue(getActiveWindowStack());
    bottom=getValue(getLast(getActiveWindowStack()));
}

START_TEST(test_cycle_window){
    IGNORE_KEY_REPEAT=1;
    int startKeySym=XK_Tab;
    int endKeySym=XK_Escape;
    Binding binding=CYCLE(1,0,startKeySym,0,endKeySym);
    initBinding(&binding);
    addBinding(&binding);
    grabBinding(&binding);
    START_MY_WM

    int size=3;
    int win[size];
    for(int i=0;i<size;i++)
        win[i]=mapWindow(createNormalWindow());

    WAIT_UNTIL_TRUE(getSize(getAllWindows())==3);
    Node*n=getAllWindows();
    FOR_EACH(n,focusWindow(getIntValue(n)));
    WAIT_UNTIL_TRUE(getSize(getActiveMasterWindowStack())==3);

    typeKey(getKeyCode(startKeySym));
    flush();

    WAIT_UNTIL_TRUE(getValue(getActiveMasterWindowStack())!=getFocusedWindow());
    assert(isFocusStackFrozen());
    for(int i=1;i<size*2;i++){
        int oldFocusedWindow=getFocusedWindow()->id;
        int newFocusedWindow=win[(i+1)%size];
        lock();
        typeKey(getKeyCode(startKeySym));
        flush();
        unlock();
        WAIT_UNTIL_TRUE(getFocusedWindow()->id!=oldFocusedWindow);
        assert(getFocusedWindow()->id==newFocusedWindow);
    }
    typeKey(getKeyCode(endKeySym));
    WAIT_UNTIL_TRUE(!isFocusStackFrozen());


}END_TEST

void assertWindowIsFocused(int win){
    xcb_input_xi_get_focus_reply_t *reply=xcb_input_xi_get_focus_reply(dis, xcb_input_xi_get_focus(dis, getActiveMasterKeyboardID()), NULL);
    assert(reply->focus==win);
    free(reply);
}

START_TEST(test_find_and_raise){
    char*titles[]={"a","c","d","e"};
    char*fakeTitle="x";
    int size=LEN(titles);
    Window win[size];
    for(int n=0;n<2;n++){
        for(int i=0;i<size;i++){
            win[i]=mapWindow(createNormalWindow());
            assert(xcb_request_check(dis,
                    xcb_ewmh_set_wm_name_checked(ewmh,
                            win[i], 1, titles[i]))==NULL);
            processNewWindow(createWindowInfo(win[i]));
        }
    }
    for(int i=0;i<size*2;i++){
        Rule r=CREATE_RULE(titles[i%size],LITERAL|TITLE,NULL);
        int id=findAndRaise(&r);
        assert(getWindowInfo(id));
        assert(strcmp(getWindowInfo(id)->title, titles[i%size])==0);
        if(i<size){
            assert(id==findAndRaiseLazy(&r));
            assert(id==win[i%size]);
        }
        else if(i==size-1){
            assert(!isInList(getActiveMasterWindowStack(), id));
            assert(id!=win[i%size]);
            continue;
        }

        flush();
        assertWindowIsFocused(id);
        onWindowFocus(id);
    }
    Rule r=CREATE_RULE(fakeTitle,LITERAL|TITLE,NULL);
    assert(!findAndRaise(&r));
    assert(!findAndRaiseLazy(&r));
}END_TEST
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
    assert(getExitStatusOfFork()==122);
}END_TEST

START_TEST(test_get_next_window_in_stack){
    addMask(bottom, HIDDEN_MASK);
    onWindowFocus(top->id);
    assert(getValue(getNextWindowInFocusStack(0))==top);
    assert(getValue(getNextWindowInStack(DOWN))==getValue(getActiveWindowStack()->next));
    //bottom is marked hidden
    assert(getValue(getNextWindowInStack(UP))!=bottom);
    assert(getValue(getNextWindowInStack(UP))==getValue(getLast(getActiveWindowStack())->prev));
    assert(getFocusedWindow()==top);
    switchToWorkspace(!getActiveWorkspaceIndex());
    Node*n=getNextWindowInStack(0);
    assert(n);
    assert(getValue(n)!=getFocusedWindow());
}END_TEST

START_TEST(test_activate_top_bottom){


    assert(focusBottom());
    assertWindowIsFocused(bottom->id);
    assert(focusTop());
    assertWindowIsFocused(top->id);
    assert(focusBottom());
    assertWindowIsFocused(bottom->id);
}END_TEST
START_TEST(test_shift_top){
    onWindowFocus(bottom->id);
    shiftTop();
    assert(getValue(getActiveWindowStack())==bottom);
    assert(getValue(getActiveWindowStack()->next)==top);
}END_TEST
START_TEST(test_shift_focus){

    Node *n=getActiveWindowStack();
    FOR_EACH(n,
            onWindowFocus(getIntValue(n));
            int id=shiftFocus(1);
            assert(id);
            assertWindowIsFocused(id);
            onWindowFocus(id);
    )
}END_TEST
START_TEST(test_swap_top){
    onWindowFocus(bottom->id);
    swapWithTop();
    assert(getValue(getActiveWindowStack())==bottom);
    assert(getValue(getLast(getActiveWindowStack()))==top);
}END_TEST

START_TEST(test_layout_toggle){
    addLayoutsToWorkspace(getActiveWorkspaceIndex(), DEFAULT_LAYOUTS, 1);
    assert(getActiveLayout()==&DEFAULT_LAYOUTS[0]);
    toggleLayout(&DEFAULT_LAYOUTS[1]);
    assert(getActiveLayout()==&DEFAULT_LAYOUTS[1]);
    toggleLayout(&DEFAULT_LAYOUTS[1]);
    assert(getActiveLayout()==&DEFAULT_LAYOUTS[0]);
}END_TEST

START_TEST(test_layout_cycle){
    addLayoutsToWorkspace(getActiveWorkspaceIndex(), DEFAULT_LAYOUTS, 2);
    assert(getActiveLayout()==&DEFAULT_LAYOUTS[0]);
    cycleLayouts(3);
    assert(getValue(getActiveWorkspace()->layouts)==&DEFAULT_LAYOUTS[1]);
    assert(getActiveLayout()==&DEFAULT_LAYOUTS[1]);
    cycleLayouts(1);
    assert(getActiveLayout()==&DEFAULT_LAYOUTS[0]);
}END_TEST
START_TEST(test_layout_cycle_reverse){
    addLayoutsToWorkspace(getActiveWorkspaceIndex(), DEFAULT_LAYOUTS, 2);
    assert(getActiveLayout()==&DEFAULT_LAYOUTS[0]);
    cycleLayouts(-1);
    assert(getValue(getActiveWorkspace()->layouts)==&DEFAULT_LAYOUTS[1]);
    assert(getActiveLayout()==&DEFAULT_LAYOUTS[1]);
    cycleLayouts(-1);
    assert(getActiveLayout()==&DEFAULT_LAYOUTS[0]);
}END_TEST
static int count=0;
static void dummy(){
    count++;
}
START_TEST(test_retile){
    Layout l={.layoutFunction=dummy};
    clearLayoutsOfWorkspace(getActiveWorkspaceIndex());
    addLayoutsToWorkspace(getActiveWorkspaceIndex(), &l, 1);
    retile();
    assert(count==1);
}END_TEST

START_TEST(test_swap_position){
    onWindowFocus(bottom->id);
    swapPosition(1);
    assert(isInList(getActiveWindowStack(), bottom->id)->next);
}END_TEST

START_TEST(test_focuse_window){
    focusActiveWindow(top);
    assert(getActiveFocus()==top->id);
}END_TEST

START_TEST(test_move_window_with_mouse){
    Binding mouseMove={0,Button1,MOVE_WINDOW_WITH_MOUSE(0,Button1),.mask=XCB_INPUT_XI_EVENT_MASK_MOTION};

    initBinding(&mouseMove);
    addBinding(&mouseMove);
    setRefefrencePointForMaster(0, 0, 0, 0);
    int xPos=10;
    int yPos=20;
    floatWindow(top);
    setLastKnowMasterPosition(xPos, yPos, xPos, yPos);

    moveWindowWithMouse(top);
    xcb_get_geometry_reply_t* reply=xcb_get_geometry_reply(dis, xcb_get_geometry(dis, top->id), NULL);
    assert(reply->x==xPos);
    assert(reply->y==yPos);
    free(reply);

}END_TEST

START_TEST(test_resize_window_with_mouse){
    int xPos=10;
    int yPos=20;
    setRefefrencePointForMaster(0, 0, 0, 0);
    setLastKnowMasterPosition(xPos, yPos, xPos, yPos);
    short config[4]={0};
    setConfig(top, config);
    floatWindow(top);

    resizeWindowWithMouse(top);
    xcb_get_geometry_reply_t* reply=xcb_get_geometry_reply(dis, xcb_get_geometry(dis, top->id), NULL);
    assert(reply->width==xPos);
    assert(reply->height==yPos);
    free(reply);
}END_TEST

START_TEST(test_send_to_workspace_by_name){
    char*names[]={"W1","W2"};
    setActiveWorkspaceIndex(0);
    setWorkspaceNames(names, 2);
    sendWindowToWorkspaceByName(top, names[1]);
    assert(!isInList(getActiveWindowStack(),top->id));
    assert(isInList(getWindowStack(getWorkspaceByIndex(1)),top->id));
    assert(!isInList(getWindowStack(getWorkspaceByIndex(1)),bottom->id));
    assert(isInList(getActiveWindowStack(),bottom->id));
    //TODO trying to get the index by name with an invalid name is undefined behavior
    getIndexFromName("");
}END_TEST


START_TEST(test_kill_focused){

    assert(!getFocusedWindow());
    //no focused window is set so it should do nothing
    killFocusedWindow();
    if(!fork()){
        openXDisplay();
        int win=createNormalWindow();
        addWindowInfo(createWindowInfo(win));
        onWindowFocus(win);
        assert(getFocusedWindow());
        close(2);
        registerForWindowEvents(win, XCB_EVENT_MASK_STRUCTURE_NOTIFY);
        killFocusedWindow();
        flush();
        msleep(10000);
    }
    else wait(NULL);
}END_TEST

START_TEST(test_function_bindings){
    int arbitNum=1001;
    Binding bindings[]={STACK_OPERATION(arbitNum,arbitNum,arbitNum,arbitNum)};
    for(int i=0;i<LEN(bindings);i++){
        int intArg=bindings[i].boundFunction.arg.intArg;
        assert(intArg==UP ||intArg==DOWN||intArg==0);
    }
}END_TEST

Suite *functionsSuite() {
    Suite*s = suite_create("Functions");
    TCase* tc_core;

    tc_core = tcase_create("Function Bindings");
    tcase_add_test(tc_core,test_function_bindings);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Helper_Functions");
    tcase_add_checked_fixture(tc_core, functionSetup, fullCleanup);
    tcase_add_test(tc_core, test_get_next_window_in_stack);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Simple functions");
    tcase_add_checked_fixture(tc_core, functionSetup, fullCleanup);
    tcase_add_test(tc_core, test_activate_top_bottom);
    tcase_add_test(tc_core, test_shift_top);
    tcase_add_test(tc_core, test_shift_focus);
    tcase_add_test(tc_core, test_swap_top);
    tcase_add_test(tc_core, test_swap_position);
    tcase_add_test(tc_core, test_focuse_window);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Workspace_Functions");
    tcase_add_checked_fixture(tc_core, functionSetup, fullCleanup);
    tcase_add_test(tc_core, test_send_to_workspace_by_name);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Mouse_Functions");
    tcase_add_checked_fixture(tc_core, functionSetup, fullCleanup);
    tcase_add_test(tc_core, test_move_window_with_mouse);
    tcase_add_test(tc_core, test_resize_window_with_mouse);
    suite_add_tcase(s, tc_core);


    tc_core = tcase_create("Complex_Functions");
    tcase_add_checked_fixture(tc_core, onStartup, fullCleanup);
    tcase_add_test(tc_core, test_cycle_window);
    tcase_add_test(tc_core, test_find_and_raise);
    tcase_add_test(tc_core,test_spawn);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Layout_Functions");
    tcase_add_checked_fixture(tc_core, onStartup, fullCleanup);
    tcase_add_test(tc_core, test_layout_toggle);
    tcase_add_test(tc_core, test_layout_cycle);
    tcase_add_test(tc_core, test_layout_cycle_reverse);
    tcase_add_test(tc_core, test_retile);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("MISC_Functions");
    tcase_add_checked_fixture(tc_core, onStartup, fullCleanup);
    tcase_add_test(tc_core, test_kill_focused);
    suite_add_tcase(s, tc_core);

    return s;
}
