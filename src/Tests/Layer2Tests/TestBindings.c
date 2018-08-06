#include "../../mywm-util.h"
#include "../../mywm-util-private.h"
#include "../../bindings.h"
#include "../../devices.h"
#include "../../defaults.h"

#include "../UnitTests.h"
#include "../../logger.h"


START_TEST(test_literal_string_rule){
    Rules r=CREATE_LITERAL_RULE("A",LITERAL,NULL);
    assert(doesStringMatchRule(&r, "A"));
    assert(doesStringMatchRule(&r, "a")==0);
    assert(doesStringMatchRule(&r, "b")==0);
    Rules r2=CREATE_LITERAL_RULE("A",LITERAL|CASE_INSENSITIVE,NULL);
    assert(doesStringMatchRule(&r2, "A"));
    assert(doesStringMatchRule(&r2, "a"));
    assert(doesStringMatchRule(&r2, "b")==0);

}END_TEST
START_TEST(test_string_rule){
    Rules r=CREATE_RULE(".*",MATCH_ANY_REGEX,NULL);
    COMPILE_RULE((&r));
    assert(doesStringMatchRule(&r, "A"));
    assert(doesStringMatchRule(&r, "B"));
    regfree(r.regexExpression);
    free(r.regexExpression);
    Rules r2=CREATE_RULE("A",MATCH_ANY_REGEX,NULL);
    COMPILE_RULE((&r2));
    assert(doesStringMatchRule(&r2, "A"));
    assert(doesStringMatchRule(&r2, "a"));
    assert(doesStringMatchRule(&r2, "B")==0);
    regfree(r2.regexExpression);
    free(r2.regexExpression);
    int target=(MATCH_ANY_REGEX) - (CASE_INSENSITIVE);
    Rules r3=CREATE_RULE("A",target,NULL);
    COMPILE_RULE((&r3));
    assert(doesStringMatchRule(&r3, "A"));
    assert(doesStringMatchRule(&r3, "a")==0);
    assert(doesStringMatchRule(&r3, "B")==0);
    regfree(r3.regexExpression);
    free(r3.regexExpression);
}END_TEST

START_TEST(test_call_bounded_function){
    createContext(1);
    addMaster(1,2);
    int integer=1;
    void*voidPointer="c";
    int count=0;
    int individualCounts[4]={};
    int targetCounts[4]={1,3,1,2};

    void funcNoArg(){
        count++;
        individualCounts[0]++;
    }
    void funcVoidArg(void*v){
        count++;
        assert(voidPointer==v);
        individualCounts[1]++;
    }
    void funcWinArg(WindowInfo*v){
        count++;
        assert(voidPointer==v);
        individualCounts[1]++;
    }
    void funcInt(int i){
        count++;
        assert(integer==i);
        individualCounts[2]++;
    }

    assert(getActiveBinding()==NULL);
    BoundFunction unset = BIND_TO_INT_FUNC((void(*)(int))exit,110);
    unset.type=UNSET;
    BoundFunction b[]={
        CHAIN(
            {0, 0, BIND_TO_INT_FUNC(exit,1)},
            {0, 0, BIND_TO_INT_FUNC(exit,1)},
            END_CHAIN(0, 0, BIND_TO_INT_FUNC(exit,1))
        ),
        unset,
        BIND_TO_FUNC(funcNoArg),
        BIND_TO_INT_FUNC(funcInt,integer),
        BIND_TO_STR_FUNC(funcVoidArg,voidPointer),
        BIND_TO_VOID_FUNC(funcVoidArg,voidPointer),
        BIND_TO_WIN_FUNC(funcWinArg),

    };

    int size=sizeof(b)/sizeof(BoundFunction);

    for(int i=0;i<size;i++){
        int c=count;
        callBoundedFunction(&b[i], voidPointer);
        if(i>1)
            assert(count-c==1);
        else assert(count==0);
    }
    for(int i=0;i<2;i++)
        assert(targetCounts[i]==individualCounts[i]);
    assert(getActiveBinding()==b[0].func.chainBindings);

}END_TEST

START_TEST(test_binding_match){
    int mod=1;
    int match=1;
    int fake=10;
    int fakeMod=2;
    Binding binding={mod,fake,.detail=match};

    anyModier=-1;

    assert(!doesBindingMatch(&binding, fake, fakeMod));
    assert(!doesBindingMatch(&binding, fake, mod));
    assert(!doesBindingMatch(&binding, match, fakeMod));
    assert(doesBindingMatch(&binding, match, mod));

    binding.mod=anyModier;
    assert(!doesBindingMatch(&binding, fake, fakeMod));
    assert(!doesBindingMatch(&binding, fake, mod));
    assert(doesBindingMatch(&binding, match, fakeMod));
    assert(doesBindingMatch(&binding, match, mod));

    binding.detail=0;

    assert(doesBindingMatch(&binding, fake, fakeMod));
    assert(doesBindingMatch(&binding, fake, mod));
    assert(doesBindingMatch(&binding, match, fakeMod));
    assert(doesBindingMatch(&binding, match, mod));

    binding.mod=mod;

    assert(!doesBindingMatch(&binding, fake, fakeMod));
    assert(doesBindingMatch(&binding, fake, mod));
    assert(!doesBindingMatch(&binding, match, fakeMod));
    assert(doesBindingMatch(&binding, match, mod));

}END_TEST
START_TEST(test_bindings){
    createContext(1);
    addMaster(1,2);
    int passThrough=_i;
    int count=0;
    int targertCount=0;
    void funcNoArg(){
        count++;
    }
    IGNORE_MASK=16;
    anyModier=0;
    for(int i=0;i<=MOUSE_RELEASE;i++){
        deviceBindingLengths[i]=4;
        for(int mod=0;mod<3;mod++)
            for(int detail=0;detail<3;detail++){
                deviceBindings[i]=(Binding[]){
                    {10,10,BIND_TO_INT_FUNC(exit,10),.passThrough=passThrough, .detail=10},
                    {mod,detail,BIND_TO_FUNC(funcNoArg),.passThrough=passThrough, .detail=detail},
                    {mod,detail,BIND_TO_FUNC(funcNoArg),.passThrough=0,.detail=detail},
                    {mod,detail,BIND_TO_INT_FUNC(exit,11),.passThrough=passThrough, .detail=detail},
                };
                assert(deviceBindings[i][0].boundFunction.func.funcNoArg);
                assert(doesBindingMatch(&deviceBindings[i][1], detail, mod));

                checkBindings(1, 1, i, NULL);
                checkBindings(1, 1|IGNORE_MASK, i, NULL);
                if(mod!=2 && detail!=2)
                    targertCount+=2*(passThrough+1);
                assert(count==targertCount);
        }
    }
}END_TEST

START_TEST(test_chain_bindings){
    createContext(1);
    addMaster(1,2);
    int count=0;

    int outerChainEndMod=1;
    void funcNoArg(){
        count++;
    }
    void funcNoArg2(){
        count--;
    }

    for(int i=0;i<4;i++){
        count=0;
        deviceBindingLengths[i]=2;
        deviceBindings[i]=(Binding[]){
            {0,1,CHAIN(
                    {0,1,CHAIN(
                        {0, 2, BIND_TO_FUNC(funcNoArg),.passThrough=0,.detail=2},
                        END_CHAIN(0,0,BIND_TO_FUNC(funcNoArg2),.passThrough=1)
                    ),.passThrough=0, .detail=1, .noGrab=1},
                    {0, 2, BIND_TO_INT_FUNC(exit,11),.detail=2},
                END_CHAIN(outerChainEndMod,0,BIND_TO_FUNC(funcNoArg2),.noEndOnPassThrough=1)
            ),.passThrough=1, .detail=1, .noGrab=1},
            {0, 2, BIND_TO_INT_FUNC(exit,10),.detail=2},
        };
        assert(getActiveBinding()==NULL);

        //enter first chain
        assert(checkBindings(1, 0, i, NULL));
        assert(getActiveBinding());
        assert(count==0);
        //enter second chain chain
        checkBindings(1, 0, i, NULL);
        assert(getSize(getActiveBindingNode())==2);
        assert(count==0);
        checkBindings(2, 0, i, NULL);
        assert(count==1);
        checkBindings(2, 0, i, NULL);
        assert(count==2);
        //exit 2nd layer of chain
        checkBindings(123, 0, i, NULL);
        assert(count==1);
        assert(getSize(getActiveBindingNode())==1);
        //re-enter 2nd layer of chain
        checkBindings(1, 0, i, NULL);
        assert(getSize(getActiveBindingNode())==2);
        assert(count==1);
        //passthrough via inner chain
        assert(!checkBindings(123, outerChainEndMod, i, NULL));
        assert(count==0);
        assert(getActiveBinding()==NULL);
    }

}END_TEST

START_TEST(test_window_matching){

    WindowInfo*all=calloc(3, sizeof(WindowInfo));
    WindowInfo*info=&all[0];
    WindowInfo*noMatchInfo=&all[1];
    WindowInfo*nullInfo=&all[2];

    char *s="test";
    int windowResourceIndex=_i;
    (&info->typeName)[windowResourceIndex]=s;
    (&noMatchInfo->typeName)[windowResourceIndex]="gibberish";
    int count=0;

    void funcNoArg(){
        count++;
    }
    int mask=1<<(_i+2);
    for(int i=0;i<MATCH_ANY_LITERAL;i++){
        count=0;
        Rules r=CREATE_LITERAL_RULE(s,i,BIND_TO_FUNC(funcNoArg));
        int result=((i & mask)?1:0);
        assert(doesWindowMatchRule(&r, info) == result);
        assert(applyRule(&r, info) == result);
        assert(count==result);
        assert(!doesWindowMatchRule(&r, noMatchInfo));
        assert(!applyRule(&r, noMatchInfo));
        assert(!doesWindowMatchRule(&r, nullInfo));
        assert(!applyRule(&r, nullInfo));
        assert(!doesWindowMatchRule(&r, NULL));
        assert(!applyRule(&r, NULL));
    }
    free(all);
}END_TEST
START_TEST(test_wildcard_rule){
    int count=0;

    void funcNoArg(){
        count++;
    }
    Rules r=CREATE_WILDCARD(BIND_TO_FUNC(funcNoArg));
    assert(applyRule(&r, NULL));
    assert(count==1);
    Rules r2=CREATE_DEFAULT_EVENT_RULE(funcNoArg);
    assert(applyRule(&r2, NULL));
    assert(count==2);
}END_TEST
START_TEST(test_apply_rules){
    int count=0;

    void funcNoArg(){
        count++;
    }
    Node*head=createEmptyHead();
    int size=MATCH_ANY_LITERAL;
    size=8;
    int target=0;
    Rules r[size];
    for(int i=0;i<size;i++){
        r[i]=(Rules)CREATE_WILDCARD(BIND_TO_FUNC(funcNoArg),.passThrough=i>=size/2);
        target+=i>=size/2;
        insertHead(head,&r[i]);
    }

    applyEventRules(head, NULL);
    assert(count==target+1);
    Node*temp=head;
    FOR_AT_MOST(temp,size/2-1);
    assert(((Rules*)getValue(temp->prev))->passThrough);
    destroyList(temp);
    assert(applyEventRules(head, NULL));
    destroyList(head);

}END_TEST


Suite *bindingsSuite(void) {
    Suite*s = suite_create("Context");

    TCase*tc_core = tcase_create("Bindings");

    tcase_add_test(tc_core,test_call_bounded_function);

    tcase_add_test(tc_core,test_binding_match);
    tcase_add_loop_test(tc_core,test_bindings,0,2);
    tcase_add_test(tc_core,test_chain_bindings);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Rule");
    tcase_add_test(tc_core, test_literal_string_rule);
    tcase_add_test(tc_core, test_string_rule);


    tcase_add_loop_test(tc_core, test_window_matching,0,4);

    tcase_add_test(tc_core,test_wildcard_rule);
    tcase_add_test(tc_core,test_apply_rules);

    suite_add_tcase(s, tc_core);

    return s;
}
