#include "../UnitTests.h"
#include "../TestX11Helper.h"

START_TEST(test_run_or_raise){
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
}END_TEST
