#include "../UnitTests.h"
#include "../TestX11Helper.h"
#include "../../layouts.h"
#include "../../wmfunctions.h"
#include "../../functions.h"

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
    WAIT_UNTIL_TRUE(getSize(getMasterWindowStack())==3);

    typeKey(getKeyCode(startKeySym));
    flush();

    WAIT_UNTIL_TRUE(getValue(getMasterWindowStack())!=getFocusedWindow());

    assert(isFocusStackFrozen());
    for(int i=1;i<size*2;i++){
        int oldFocusedWindow=getFocusedWindow()->id;
        int newFocusedWindow=win[(i+1)%size];
        typeKey(getKeyCode(startKeySym));
        flush();
        WAIT_UNTIL_TRUE(getFocusedWindow()->id!=oldFocusedWindow);
        if(getFocusedWindow()->id!=newFocusedWindow)
            LOG(LOG_LEVEL_NONE,"old %d new %d current %d",oldFocusedWindow,newFocusedWindow,getFocusedWindow()->id);
        assert(getFocusedWindow()->id==newFocusedWindow);
    }
    typeKey(getKeyCode(endKeySym));
    WAIT_UNTIL_TRUE(!isFocusStackFrozen());


}END_TEST
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
        if(i<size)
            assert(id==findAndRaiseLazy(&r));
        else if(i==size){
            assert(!isInList(getMasterWindowStack(), id));
            clearWindowCache();
            continue;
        }
        assert(id==win[i%size]);

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
Suite *functionsSuite() {
    Suite*s = suite_create("Functions");
    TCase* tc_core = tcase_create("Complex functions");
    tcase_add_checked_fixture(tc_core, onStartup, fullCleanup);
    tcase_add_test(tc_core, test_cycle_window);
    tcase_add_test(tc_core, test_find_and_raise);
    tcase_add_test(tc_core,test_spawn);
    suite_add_tcase(s, tc_core);
    return s;
}
