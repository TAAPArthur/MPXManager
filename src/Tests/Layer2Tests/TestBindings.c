#include "../../mywm-util.h"

#include "../../bindings.h"
#include "../../devices.h"
#include "../../globals.h"
#include "../UnitTests.h"
#include "../TestX11Helper.h"

#include <stdlib.h>

static BoundFunction sampleChain=CHAIN_GRAB(XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE,
        {0, Button1, .mask=XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS},
        {0, Button1, .mask=XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE, .noGrab=1},
        END_CHAIN(0, 0));
int returnFalse(void){return 0;}
int returnTrue(void){return 1;}


START_TEST(test_init_binding){
    Binding binding={0,Button1,.targetID=BIND_TO_ALL_MASTERS};
    initBinding(&binding);
    assert(binding.targetID==1);
    assert(binding.detail);
    binding.targetID=BIND_TO_ALL_DEVICES;
    initBinding(&binding);
    assert(binding.targetID==0);
}END_TEST
START_TEST(test_binding_add){
    assert(!isNotEmpty(getDeviceBindings()));
    Binding arr[5]={0};
    Binding single;
    addBinding(&single);
    assert(getValue(getDeviceBindings()->prev)==&single);
    addBindings(arr,LEN(arr));
    int i=0;
    Node *n =getDeviceBindings()->next;
    FOR_AT_MOST(n,LEN(arr),assert(getValue(n)==&arr[i++]));
}END_TEST
START_TEST(test_binding_target_id){
    Binding binding={.targetID=-1};
    assert(getIDOfBindingTarget(&binding)==getActiveMasterPointerID());
    binding.targetID=1;
    assert(getIDOfBindingTarget(&binding)==1);
}END_TEST


///does to make sure a binding will match only the correct detail/mod pair(s)
START_TEST(test_binding_match){
    int mod=1;
    int match=1;
    int fake=10;
    int fakeMod=2;
    int mask=Mod4Mask;
    int fakeMask=Mod1Mask;
    Binding binding={mod,fake,.detail=match,.mask=mask};

    int wildCardMod=0;
    int wildCardDetail=0;
    assert(binding.mask);
    for(int i=0;i<4;i++){
        assert(!doesBindingMatch(&binding, fake, fakeMod,fakeMask));
        assert(doesBindingMatch(&binding, fake, fakeMod, mask)==(wildCardDetail&wildCardMod));
        assert(!doesBindingMatch(&binding, fake, mod,fakeMask));

        assert(doesBindingMatch(&binding, fake, mod,mask)==wildCardDetail);

        assert(!doesBindingMatch(&binding, match, fakeMod,fakeMask));


        assert(doesBindingMatch(&binding, match, fakeMod,mask)==wildCardMod);
        assert(!doesBindingMatch(&binding, match, mod,fakeMask));
        assert(doesBindingMatch(&binding, match, mod,mask));

        assert(doesBindingMatch(&binding, match, mod,0));
        if(i==0){
            binding.mod=WILDCARD_MODIFIER;
            wildCardMod=1;
        }
        else if(i==1){
            binding.detail=0;
            wildCardDetail=1;
        }
        else if(i==2){
            binding.mod=mod;
            wildCardMod=0;
        }
    }


}END_TEST
START_TEST(test_check_bindings){
    int passThrough=_i;
    int count=0;
    int targetCount=0;
    void funcNoArg(void){
        count++;
    }
    IGNORE_MASK=16;
    int mask=0;
    for(int mod=0;mod<3;mod++)
        for(int detail=0;detail<3;detail++){
            int effectiveMod=mod==0?WILDCARD_MODIFIER:mod;
            Binding arr[]={
                {10,10,BIND(exit,10),.passThrough=passThrough, .detail=10},
                {effectiveMod,detail,BIND(funcNoArg),.passThrough=passThrough, .detail=detail},
                {effectiveMod,detail,BIND(funcNoArg),.passThrough=NO_PASSTHROUGH,.detail=detail},
                {effectiveMod,detail,BIND(exit,11),.passThrough=passThrough, .detail=detail},
            };
            clearList(getDeviceBindings());
            addBindings(arr,LEN(arr));

            checkBindings(1, 1, mask, NULL);
            checkBindings(1, 1|IGNORE_MASK, mask, NULL);
            if(mod!=2 && detail!=2)
                targetCount+=2*((passThrough==ALWAYS_PASSTHROUGH)+1);
            assert(count==targetCount);
    }
}END_TEST


///Test to make sure callBoundedFunction() actually calls the right function
START_TEST(test_call_bounded_function){
    int integer=1;
    void*voidPointer="c";
    int count=0;

    int targetCounts[]={4,6,5};
    int individualCounts[LEN(targetCounts)]={0};

    void funcNoArg(void){
        count++;
        individualCounts[0]++;
    }
    int funcNoArgReturn(void){
        count++;
        individualCounts[0]++;
        return 2;
    }
    void funcVoidArg(void*v){
        count++;
        assert(voidPointer==v);
        individualCounts[1]++;
    }
    int funcVoidArgReturn(void*v){
        count++;
        assert(voidPointer==v);
        individualCounts[1]++;
        return 3;
    }
    void funcWinArg(WindowInfo*v){
        count++;
        assert(voidPointer==v);
        individualCounts[1]++;
    }
    int funcWinArgReturn(WindowInfo*v){
        count++;
        assert(voidPointer==v);
        individualCounts[1]++;
        return 4;
    }
    void funcWinIntArg(WindowInfo*v,int i){
        count++;
        assert(integer==i);
        assert(voidPointer==v);
        individualCounts[1]++;
    }
    int funcWinIntArgReturn(WindowInfo*v,int i){
        count++;
        assert(integer==i);
        assert(voidPointer==v);
        individualCounts[1]++;
        return 4;
    }
    void funcInt(int i){
        count++;
        assert(integer==i);
        individualCounts[2]++;
    }
    int funcIntReturn(int i){
        count++;
        assert(integer==i);
        individualCounts[2]++;
        return 5;
    }
    int funcEcho(int i){
        assert(i==integer);
        return i;
    }




    assert(getActiveBinding()==NULL);
    BoundFunction unset = BIND(exit,110);
    unset.type=UNSET;
    BoundFunction b[]={
        unset,
        CHAIN_GRAB(0,
            {0, Button1, BIND(exit,1),.noGrab=1},
            {0, Button1, BIND(exit,1),.noGrab=1},
            END_CHAIN(0, Button1, BIND(exit,1),.noGrab=1)
        ),
        BIND(funcNoArg),
        BIND(funcNoArgReturn),
        BIND(funcInt,integer),
        BIND(funcIntReturn,integer),
        BIND(funcInt,&((BoundFunction)BIND(funcEcho,integer))),
        BIND(funcVoidArg,voidPointer),
        BIND(funcVoidArgReturn,voidPointer),
        BIND(funcWinArg),
        BIND(funcWinArgReturn),
        BIND(funcWinIntArg,integer),
        BIND(funcWinIntArgReturn,integer),
        AND(BIND(funcInt,integer),BIND(funcIntReturn,integer)),
        OR(BIND(funcNoArg),BIND(funcNoArgReturn)),
        AUTO_CHAIN_GRAB(0,1,
            {0, Button1, OR(BIND(funcNoArg),BIND(funcNoArgReturn)),.noGrab=1},
            {0, Button1, BIND(exit,1),.noGrab=1},
            END_CHAIN(0, Button1, BIND(exit,1),.noGrab=1)
        ),
    };
    //{ _Generic(funcNoArg,default: .func) =funcNoArg,.type=NO_ARGS}

    int size=sizeof(b)/sizeof(BoundFunction);

    int sum=0;
    for(int i=0;i<size;i++){
        int c=count;
        int result=callBoundedFunction(&b[i], voidPointer);
        assert(result);
        sum+=result;
        if(i<LEN(b)-3)
            if (i>1)
                assert(count-c==1);
            else assert(count==0);
    }
    for(int i=0;i<LEN(targetCounts);i++){
        assert(targetCounts[i]==individualCounts[i]);
    }

    assert(getActiveBinding()==b[LEN(b)-1].func.chainBindings);

    assert(sum==13+LEN(b));

}END_TEST

START_TEST(test_and_or_short_circuit){

    BoundFunction fail=BIND(exit,22);
    BoundFunction and=AND(BIND(returnFalse),fail);
    BoundFunction or=OR(BIND(returnTrue),fail);
    assert(!callBoundedFunction(&and, NULL));
    assert(callBoundedFunction(&or, NULL));
    BoundFunction combo=OR(and,and);
    BoundFunction combo2=AND(or,or);
    assert(!callBoundedFunction(&combo, NULL));
    assert(callBoundedFunction(&combo2, NULL));
}END_TEST
START_TEST(test_init_chain_binding){
    startChain(&sampleChain);
    assert(sampleChain.func.chainBindings->detail);
    assert(sampleChain.func.chainBindings[1].detail);
}END_TEST
START_TEST(test_start_end_chain){
    startChain(&sampleChain);
    assert(getActiveBinding()==sampleChain.func.chainBindings);
    endChain();
    assert(getActiveBinding()==NULL);
}END_TEST
START_TEST(test_chain_bindings){
    int count=0;

    int outerChainEndMod=1;
    void funcNoArg(void){
        count++;
    }
    void funcNoArg2(void){
        count--;
    }

    int mask=0;

    Binding chain={0,1,CHAIN_GRAB(0,
                {0,1,CHAIN_GRAB(0,
                    {0, 2, BIND(funcNoArg),.passThrough=NO_PASSTHROUGH,.detail=2 , .noGrab=1},
                    END_CHAIN(0,0,BIND(funcNoArg2),.passThrough=ALWAYS_PASSTHROUGH, .noGrab=1)
                ),.passThrough=NO_PASSTHROUGH, .detail=1, .noGrab=1},
                {0, 2, BIND(exit,11),.detail=2, .noGrab=1,.passThrough=NO_PASSTHROUGH},
            END_CHAIN(outerChainEndMod,0,BIND(funcNoArg2),.noEndOnPassThrough=1, .noGrab=1)
        ),.passThrough=ALWAYS_PASSTHROUGH, .detail=1, .noGrab=1};
    Binding dummy={0, 2, BIND(exit,10),.detail=2, .noGrab=1,.passThrough=NO_PASSTHROUGH};
    addBinding(&chain);
    addBinding(&dummy);
    assert(getActiveBinding()==NULL);

    //enter first chain
    assert(checkBindings(1, 0, mask, NULL));
    assert(getActiveBinding());
    assert(count==0);
    //enter second chain chain
    checkBindings(1, 0, mask, NULL);
    assert(getSize(getActiveBindingNode())==2);
    assert(count==0);
    checkBindings(2, 0, mask, NULL);
    assert(count==1);
    checkBindings(2, 0, mask, NULL);
    assert(count==2);
    //exit 2nd layer of chain
    checkBindings(123, 0, mask, NULL);
    assert(count==1);
    assert(getSize(getActiveBindingNode())==1);
    //re-enter 2nd layer of chain
    checkBindings(1, 0, mask, NULL);
    assert(getSize(getActiveBindingNode())==2);
    assert(count==1);
    //passthrough via inner chain
    checkBindings(123, outerChainEndMod, mask, NULL);
    assert(count==0);
    assert(getActiveBinding()==NULL);
    clearList(getDeviceBindings());

}END_TEST

START_TEST(test_chain_grab){

    addBinding(sampleChain.func.chainBindings);
    addBinding(&sampleChain.func.chainBindings[1]);
    startChain(&sampleChain);
    int mask=XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS|XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE;
    if(_i){
        //triggerAllBindings(mask);
        //waitToReceiveInput(mask);
    }
    endChain();
    if(!_i){
        triggerAllBindings(mask);
        assert(!xcb_poll_for_event(dis));
    }
}END_TEST



START_TEST(test_literal_string_rule){
    char*var;
    int target=LITERAL;
    if(_i){
        var="$random_name";
        const char*value="A";
        setenv(var+1,value,1);
        target|=ENV_VAR;
    }
    else var ="A";
    Rule r=CREATE_LITERAL_RULE(var,target,NULL);
    assert(doesStringMatchRule(&r, "A"));
    assert(doesStringMatchRule(&r, "a")==0);
    assert(doesStringMatchRule(&r, "b")==0);
    Rule r2=CREATE_LITERAL_RULE(var,target|CASE_INSENSITIVE,NULL);
    assert(doesStringMatchRule(&r2, "A"));
    assert(doesStringMatchRule(&r2, "a"));
    assert(doesStringMatchRule(&r2, "b")==0);

}END_TEST
START_TEST(test_string_rule){
    Rule r=CREATE_RULE(".*",MATCH_ANY_REGEX,NULL);
    assert(doesStringMatchRule(&r, "A"));
    assert(doesStringMatchRule(&r, "B"));
    regfree(r.regexExpression);
    free(r.regexExpression);
    Rule r2=CREATE_RULE("A",MATCH_ANY_REGEX,NULL);
    assert(doesStringMatchRule(&r2, "A"));
    assert(doesStringMatchRule(&r2, "a"));
    assert(doesStringMatchRule(&r2, "B")==0);
    regfree(r2.regexExpression);
    free(r2.regexExpression);
    Rule r3=CREATE_RULE("A",(MATCH_ANY_REGEX) - (CASE_INSENSITIVE),NULL);
    assert(doesStringMatchRule(&r3, "A"));
    assert(doesStringMatchRule(&r3, "a")==0);
    assert(doesStringMatchRule(&r3, "B")==0);
    regfree(r3.regexExpression);
    free(r3.regexExpression);
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

    int mask=1<<(_i+2);
    for(int i=0;i<MATCH_ANY_LITERAL;i++){
        Rule r=CREATE_LITERAL_RULE(s,i,NULL);
        int result=((i & mask)?1:0);
        assert(doesWindowMatchRule(&r, info) == result);
        assert(!doesWindowMatchRule(&r, noMatchInfo));
        assert(!doesWindowMatchRule(&r, nullInfo));
        assert(!doesWindowMatchRule(&r, NULL));
    }
    free(all);
}END_TEST
START_TEST(test_wildcard_rule){
    Rule r=CREATE_WILDCARD(NULL);
    assert(doesWindowMatchRule(&r, NULL));
}END_TEST
START_TEST(test_apply_rules){
    int count=0;

    void funcNoArg(void){
        count++;
    }
    Rule dummy=CREATE_RULE("",LITERAL,NULL);
    assert(applyRule(&dummy, NULL));
    Node*head=createCircularHead(NULL);
    assert(applyRules(head, NULL));
    int size=MATCH_ANY_LITERAL;
    size=8;
    int target=0;
    Rule r[size];
    for(int i=0;i<size;i++){
        r[i]=(Rule)CREATE_WILDCARD(BIND(funcNoArg),.passThrough=i<size/2?ALWAYS_PASSTHROUGH:NO_PASSTHROUGH);
        target+=r[i].passThrough==ALWAYS_PASSTHROUGH;
        insertHead(head,&r[i]);
    }

    applyRules(head, NULL);
    assert(count==target+1);
    destroyList(head);

}END_TEST
START_TEST(test_passthrough_rules){

    Rule rules[]={
            CREATE_WILDCARD(BIND(returnTrue),.passThrough=ALWAYS_PASSTHROUGH),
            CREATE_WILDCARD(BIND(returnTrue),.passThrough=PASSTHROUGH_IF_TRUE),
            CREATE_WILDCARD(BIND(returnFalse),.passThrough=PASSTHROUGH_IF_FALSE),
    };

    for(int i=0;i<LEN(rules);i++)
        assert(applyRule(&rules[i], NULL));

    Rule noPassThrough=CREATE_WILDCARD(BIND(returnFalse),.passThrough=NO_PASSTHROUGH);
    assert(!applyRule(&noPassThrough, NULL));

    //here to touch 'impossible case'
    passThrough(0, -1);

}END_TEST


static void setup(){
    DEFAULT_BINDING_MASKS=XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS;
    createContextAndSimpleConnection();
}
Suite *bindingsSuite(void) {
    Suite*s = suite_create("Context");

    TCase*tc_core;

    tc_core= tcase_create("Bindings");
    tcase_add_checked_fixture(tc_core, setup, destroyContextAndConnection);
    tcase_add_test(tc_core,test_init_binding);
    tcase_add_test(tc_core,test_binding_add);
    tcase_add_test(tc_core,test_binding_target_id);
    tcase_add_test(tc_core,test_binding_match);
    tcase_add_test(tc_core,test_call_bounded_function);
    tcase_add_test(tc_core,test_and_or_short_circuit);
    tcase_add_loop_test(tc_core,test_check_bindings,0,2);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Chain Bindings");
    tcase_add_checked_fixture(tc_core, setup, destroyContextAndConnection);

    tcase_add_test(tc_core,test_init_chain_binding);
    tcase_add_test(tc_core,test_start_end_chain);
    tcase_add_test(tc_core,test_chain_bindings);
    tcase_add_loop_test(tc_core,test_chain_grab,0,1);
    tcase_add_loop_test(tc_core,test_chain_grab,1,2);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Rule");
    tcase_add_loop_test(tc_core, test_literal_string_rule,0,2);
    tcase_add_test(tc_core, test_string_rule);
    tcase_add_loop_test(tc_core, test_window_matching,0,4);
    tcase_add_test(tc_core,test_wildcard_rule);
    tcase_add_test(tc_core, test_passthrough_rules);
    tcase_add_test(tc_core,test_apply_rules);
    suite_add_tcase(s, tc_core);

    return s;
}
