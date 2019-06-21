#include "../../mywm-util.h"

#include "../../bindings.h"
#include "../../devices.h"
#include "../../globals.h"
#include "../UnitTests.h"
#include "../TestX11Helper.h"

#include <stdlib.h>

static BoundFunction sampleChain = CHAIN_GRAB(XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE,
{0, Button1, .mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS},
{0, Button1, .mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE, .noGrab = 1});


START_TEST(test_init_binding){
    Binding binding = {0, Button1, .targetID = BIND_TO_ALL_MASTERS};
    initBinding(&binding);
    assert(binding.targetID == 1);
    assert(binding.detail);
    binding.targetID = BIND_TO_ALL_DEVICES;
    initBinding(&binding);
    assert(binding.targetID == 0);
}
END_TEST
START_TEST(test_binding_add){
    assert(!isNotEmpty(getDeviceBindings()));
    Binding arr[5] = {0};
    addBindings(arr, LEN(arr));
    assert(getSize(getDeviceBindings()) == LEN(arr));
    Binding single = {0};
    addBindings(&single, 1);
    assert(getSize(getDeviceBindings()) == LEN(arr) + 1);
    assert(getLast(getDeviceBindings()) == &single);
}
END_TEST
START_TEST(test_binding_target_id){
    Binding binding = {.targetID = -1};
    assert(getIDOfBindingTarget(&binding) == getActiveMasterPointerID());
    binding.targetID = 1;
    assert(getIDOfBindingTarget(&binding) == 1);
}
END_TEST


START_TEST(test_binding_mode_match){
    setCurrentMode(NORMAL_MODE);
    Binding binding = {WILDCARD_MODIFIER, 0, .detail = 0, .mask = -1};
    assert(!doesBindingMatch(&binding, 0, 0, 0, 0));
    binding.mode = NORMAL_MODE;
    assert(doesBindingMatch(&binding, 0, 0, 0, 0));
    binding.mode = 0;
    assert(!doesBindingMatch(&binding, 0, 0, 0, 0));
    setCurrentMode(ALL_MODES);
    assert(doesBindingMatch(&binding, 0, 0, 0, 0));
}
END_TEST
///does to make sure a binding will match only the correct detail/mod pair(s)
START_TEST(test_binding_match){
    int mod = 1;
    int match = 1;
    int fake = 10;
    int fakeMod = 2;
    int mask = Mod4Mask;
    int fakeMask = Mod1Mask;
    Binding binding = {mod, fake, .detail = match, .mask = mask};
    int wildCardMod = 0;
    int wildCardDetail = 0;
    assert(binding.mask);
    for(int i = 0; i < 4; i++){
        assert(!doesBindingMatch(&binding, fake, fakeMod, fakeMask, 0));
        assert(doesBindingMatch(&binding, fake, fakeMod, mask, 0) == (wildCardDetail & wildCardMod));
        assert(!doesBindingMatch(&binding, fake, mod, fakeMask, 0));
        assert(doesBindingMatch(&binding, fake, mod, mask, 0) == wildCardDetail);
        assert(!doesBindingMatch(&binding, match, fakeMod, fakeMask, 0));
        assert(doesBindingMatch(&binding, match, fakeMod, mask, 0) == wildCardMod);
        assert(!doesBindingMatch(&binding, match, mod, fakeMask, 0));
        assert(doesBindingMatch(&binding, match, mod, mask, 0));
        assert(doesBindingMatch(&binding, match, mod, 0, 0));
        if(i == 0){
            binding.mod = WILDCARD_MODIFIER;
            wildCardMod = 1;
        }
        else if(i == 1){
            binding.detail = 0;
            wildCardDetail = 1;
        }
        else if(i == 2){
            binding.mod = mod;
            wildCardMod = 0;
        }
    }
}
END_TEST
START_TEST(test_check_bindings){
    int passThrough = _i;
    int count = 0;
    int targetCount = 0;
    void funcNoArg(void){
        count++;
    }
    IGNORE_MASK = 16;
    int mask = 0;
    for(int mod = 0; mod < 3; mod++)
        for(int detail = 0; detail < 3; detail++){
            int effectiveMod = mod == 0 ? WILDCARD_MODIFIER : mod;
            Binding arr[] = {
                {10, 10, BIND(exit, 10), .passThrough = passThrough, .detail = 10},
                {effectiveMod, detail, BIND(funcNoArg), .passThrough = passThrough, .detail = detail, .windowTarget = mod == 0 ? FOCUSED_WINDOW : TARGET_WINDOW},
                {effectiveMod, detail, BIND(funcNoArg), .passThrough = NO_PASSTHROUGH, .detail = detail},
                {effectiveMod, detail, BIND(exit, 11), .passThrough = passThrough, .detail = detail},
            };
            addBindings(arr, LEN(arr));
            checkBindings(1, 1, mask, NULL, 0);
            checkBindings(1 | IGNORE_MASK, 1, mask, NULL, 0);
            if(mod != 2 && detail != 2)
                targetCount += 2 * ((passThrough == ALWAYS_PASSTHROUGH) + 1);
            assert(count == targetCount);
            clearList(getDeviceBindings());
        }
    count = 0;
    WindowInfo* dummy = (WindowInfo*)1;
    WindowInfo* focusDummy = createWindowInfo(2);
    addWindowInfo(focusDummy);
    onWindowFocus(focusDummy->id);
    void funcWinArg(WindowInfo * winInfo){
        switch(count){
            case DEFAULT:
            case FOCUSED_WINDOW:
                assert(focusDummy == winInfo);
                break;
            case TARGET_WINDOW:
                assert(dummy == winInfo);
        }
        count++;
    }
    int numTypes = 4;
    for(WindowParamType i = 0; i < numTypes + 1; i++){
        Binding b = {WILDCARD_MODIFIER, 0, BIND(funcWinArg), .mask = KEYBOARD_MASKS, .detail = 0, .windowTarget = i};
        if(i == numTypes){
            dummy = NULL;
            b.windowTarget = TARGET_WINDOW;
        }
        addBindings(&b, 1);
        checkBindings(1, 1, mask, dummy, 0);
        clearList(getDeviceBindings());
    }
    assert(count == numTypes);
}
END_TEST

START_TEST(test_bounded_function_negate_result){
    BoundFunction resultTrue = NBIND(returnFalse);
    BoundFunction resultFalse = NBIND(returnTrue);
    assert(callBoundedFunction(&resultTrue, NULL));
    assert(!callBoundedFunction(&resultFalse, NULL));
}
END_TEST
///Test to make sure callBoundedFunction() actually calls the right function
START_TEST(test_call_bounded_function){
    int integer = 256;
    int integer2 = -2;
    long longNumber = 1L << 32 + 2;
    void* voidPointer = "c";
    int count = 0;
    int targetCounts[] = {6, 6, 6};
    int individualCounts[LEN(targetCounts)] = {0};
    void funcNoArg(void){
        count++;
        individualCounts[0]++;
    }
    int funcNoArgReturn(void){
        count++;
        individualCounts[0]++;
        return 2;
    }
    void funcLongArg(long l){
        count++;
        assert(longNumber == l);
        individualCounts[0]++;
    }
    void funcVoidArg(void* v){
        count++;
        assert(voidPointer == v);
        individualCounts[1]++;
    }
    int funcVoidArgReturn(void* v){
        count++;
        assert(voidPointer == v);
        individualCounts[1]++;
        return 3;
    }
    void funcWinArg(WindowInfo * v){
        count++;
        assert(voidPointer == v);
        individualCounts[1]++;
    }
    int funcWinArgReturn(WindowInfo * v){
        count++;
        assert(voidPointer == v);
        individualCounts[1]++;
        return 4;
    }
    void funcWinIntArg(WindowInfo * v, int i){
        count++;
        assert(integer == i);
        assert(voidPointer == v);
        individualCounts[1]++;
    }
    int funcWinIntArgReturn(WindowInfo * v, int i){
        count++;
        assert(integer == i);
        assert(voidPointer == v);
        individualCounts[1]++;
        return 4;
    }
    void funcInt(int i){
        count++;
        assert(integer == i);
        individualCounts[2]++;
    }
    void funcIntInt(int i, int j){
        count++;
        assert(integer == i);
        assert(integer2 == j);
    }
    int funcIntIntReturnInt(int i, int j){
        count++;
        assert(integer == i);
        assert(integer2 == j);
        return 1;
    }
    int funcEcho(int i){
        count++;
        assert(integer == i);
        individualCounts[2]++;
        return i;
    }
    int funcIntReturn(int i){
        count++;
        assert(integer == i);
        individualCounts[2]++;
        return 5;
    }
    assert(getActiveChain() == NULL);
    BoundFunction unset = BIND(exit, 110);
    unset.type = UNSET;
    BoundFunction b[] = {
        unset,
        CHAIN_GRAB(0,
        {0, Button1, BIND(exit, 1), .noGrab = 1},
        {0, Button1, BIND(exit, 1), .noGrab = 1},
        {0, Button1, BIND(exit, 1), .noGrab = 1}
                  ),
        BIND(funcNoArg),
        BIND(funcNoArgReturn),
        BIND(funcInt, integer),
        BIND(funcIntReturn, integer),
        BIND(funcLongArg, longNumber),
        BIND(funcVoidArg, voidPointer),
        BIND(funcVoidArgReturn, voidPointer),
        BIND(funcWinArg),
        BIND(funcIntInt, integer, integer2),
        BIND(funcIntIntReturnInt, integer, integer2),
        BIND(funcWinArgReturn),
        BIND(funcWinIntArg, integer),
        BIND(funcWinIntArgReturn, integer),
        AND(BIND(funcInt, integer), BIND(funcIntReturn, integer)),
        OR(BIND(funcNoArg), BIND(funcNoArgReturn)),
        BOTH(BIND(funcNoArg), BIND(funcNoArgReturn)),
        PIPE(BIND(funcEcho, integer), BIND(funcInt)),
        CHAIN_GRAB(0, {0, Button1, BIND(exit, 1), .noGrab = 1}),
    };
    //{ _Generic(funcNoArg,default: .func) =funcNoArg,.type=NO_ARGS}
    int size = sizeof(b) / sizeof(BoundFunction);
    int sum = 0;
    for(int i = 0; i < size; i++){
        assert(b[i].negateResult == 0);
        int result = callBoundedFunction(&b[i], voidPointer);
        assert(result);
        sum += result;
        if(i < LEN(b) - 4)
            if(i == 1)
                assert(count == 0);
    }
    for(int i = 0; i < LEN(targetCounts); i++){
        assert(targetCounts[i] == individualCounts[i]);
    }
    assert(getActiveChain() == &b[LEN(b) - 1]);
    assert(sum == 13 + LEN(b));
}
END_TEST

START_TEST(test_and_or_short_circuit){
    BoundFunction fail = BIND(exit, 22);
    BoundFunction and = AND(BIND(returnFalse), fail);
    BoundFunction or = OR(BIND(returnTrue), fail);
    assert(!callBoundedFunction(& and, NULL));
    assert(callBoundedFunction(& or, NULL));
    BoundFunction combo = OR( and, and);
    BoundFunction combo2 = AND( or, or);
    assert(!callBoundedFunction(&combo, NULL));
    assert(callBoundedFunction(&combo2, NULL));
}
END_TEST
START_TEST(test_init_chain_binding){
    startChain(&sampleChain);
    assert(sampleChain.func.chainBindings->detail);
    assert(sampleChain.func.chainBindings[1].detail);
}
END_TEST
START_TEST(test_start_end_chain){
    startChain(&sampleChain);
    assert(getActiveChain() == &sampleChain);
    endChain();
    assert(getActiveChain() == NULL);
}
END_TEST
START_TEST(test_chain_bindings){
    int count = 0;
    int outerChainEndMod = 1;
    void add(void){
        count++;
    }
    void subtract(void){
        count--;
    }
    int mask = 0;
    int dangerDetail = 2;
    int exitDetail = 3;
    Binding chain = {0, 1, CHAIN_GRAB(0,
        {
            0, 1, CHAIN_GRAB(0,
            {0, 1, BIND(add), .passThrough = ALWAYS_PASSTHROUGH},
            {0, 1, CHAIN_GRAB(0, {0, 0, .endChain = 1, .passThrough = NO_PASSTHROUGH})},
            {0, dangerDetail, BIND(add), .passThrough = NO_PASSTHROUGH, .noGrab = 1},
            {0, exitDetail, BIND(subtract), .passThrough = ALWAYS_PASSTHROUGH, .noGrab = 1}
                            ), .passThrough = NO_PASSTHROUGH, .noGrab = 1
        },
        {0, dangerDetail, BIND(exit, 11), .noGrab = 1, .passThrough = NO_PASSTHROUGH},
        {outerChainEndMod, 0, BIND(subtract),  .noGrab = 1, .endChain = 1, .passThrough = ALWAYS_PASSTHROUGH,},
        {WILDCARD_MODIFIER, 0, BIND(subtract), .passThrough = NO_PASSTHROUGH},
                                         ), .passThrough = ALWAYS_PASSTHROUGH, .noGrab = 1
    };
    Binding dummy = {0, dangerDetail, BIND(exit, 10), .noGrab = 1, .passThrough = NO_PASSTHROUGH};
    addBindings(&dummy, 1);
    addBindings(&chain, 1);
    addBindings(&dummy, 1);
    FOR_EACH(Binding*, binding, getDeviceBindings())initBinding(binding);
    assert(getActiveChain() == NULL);
    //enter first chain & second chain
    checkBindings(0, 1, mask, NULL, 0);
    assert(getSize(getActiveChains()) == 2);
    assert(getActiveChain());
    assert(count == 1);
    checkBindings(0, dangerDetail, mask, NULL, 0);
    assert(count == 2);
    checkBindings(0, dangerDetail, mask, NULL, 0);
    assert(count == 3);
    //exit second chain
    checkBindings(0, exitDetail, mask, NULL, 0);
    assert(getSize(getActiveChains()) == 1);
    assert(getActiveChain());
    assert(count == 1);
    //exit fist chain
    checkBindings(outerChainEndMod, 0, mask, NULL, 0);
    assert(count == 0);
    assert(getActiveChain() == NULL);
    clearList(getDeviceBindings());
}
END_TEST

START_TEST(test_chain_grab){
    addBindings(sampleChain.func.chainBindings, 2);
    startChain(&sampleChain);
    int mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS | XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE;
    if(_i){
        //triggerAllBindings(mask);
        //waitToReceiveInput(mask);
    }
    endChain();
    if(!_i){
        triggerAllBindings(mask);
        assert(!xcb_poll_for_event(dis));
    }
}
END_TEST



START_TEST(test_env_rule_sub){
    int target = LITERAL | ENV_VAR;
    char* var = "$_var";
    char* var2 = "$_var2";
    char* value = "A";
    char* value2 = "ABCDEFGHI";
    char* str[] = {"echo '$_var'", "$_var", "$_var text $_var", "\\$var", "$_var2 long long text", "$_unsetVar"};
    char* subVar[] = {"echo 'A'", "A", "A text A", "$var", "ABCDEFGHI long long text", ""};
    setenv(var + 1, value, 1);
    setenv(var2 + 1, value2, 1);
    unsetenv("_unsetVar");
    for(int i = 0; i < LEN(str); i++){
        Rule r = {str[i], target | LITERAL, NULL};
        initRule(&r);
        assert(strcmp(r.literal, subVar[i]) == 0);
        free(r.literal);
    }
    char longValue[255];
    for(int i = 0; i < 255; i++)longValue[i] = 'A';
    longValue[LEN(longValue) - 1] = 0;
    setenv(var + 1, longValue, 1);
    Rule r = {var, target | LITERAL, NULL};
    initRule(&r);
    assert(strcmp(r.literal, longValue) == 0);
    free(r.literal);
}
END_TEST
START_TEST(test_literal_string_rule){
    char* var;
    int target = LITERAL;
    if(_i){
        var = "$random_name";
        const char* value = "A";
        setenv(var + 1, value, 1);
        target |= ENV_VAR;
    }
    else var = "A";
    Rule r = {var, target | CASE_SENSITIVE | LITERAL, NULL};
    assert(doesStringMatchRule(&r, "A"));
    assert(doesStringMatchRule(&r, "random_text_A") == 0);
    assert(doesStringMatchRule(&r, "A_random_text") == 0);
    assert(doesStringMatchRule(&r, "random_text_A_random_text") == 0);
    assert(doesStringMatchRule(&r, "a") == 0);
    assert(doesStringMatchRule(&r, "b") == 0);
    Rule r2 = {var, target | LITERAL, NULL};
    assert(doesStringMatchRule(&r2, "A"));
    assert(doesStringMatchRule(&r2, "a"));
    assert(doesStringMatchRule(&r2, "b") == 0);
    if(_i){
        free(r.literal);
        free(r2.literal);
    }
}
END_TEST
START_TEST(test_string_rule){
    Rule r = {".*", MATCH_ANY_REGEX, NULL};
    assert(doesStringMatchRule(&r, "A"));
    assert(doesStringMatchRule(&r, "B"));
    regfree(r.regexExpression);
    free(r.regexExpression);
    Rule r2 = {"A", MATCH_ANY_REGEX, NULL};
    assert(doesStringMatchRule(&r2, "A"));
    assert(doesStringMatchRule(&r2, "a"));
    assert(doesStringMatchRule(&r2, "B") == 0);
    regfree(r2.regexExpression);
    free(r2.regexExpression);
    Rule r3 = {"A", (MATCH_ANY_REGEX | CASE_SENSITIVE), NULL};
    assert(doesStringMatchRule(&r3, "A"));
    assert(doesStringMatchRule(&r3, "a") == 0);
    assert(doesStringMatchRule(&r3, "B") == 0);
    regfree(r3.regexExpression);
    free(r3.regexExpression);
}
END_TEST

START_TEST(test_window_matching){
    WindowInfo* all = calloc(3, sizeof(WindowInfo));
    WindowInfo* info = &all[0];
    WindowInfo* noMatchInfo = &all[1];
    WindowInfo* nullInfo = &all[2];
    char* s = "test";
    WindowID windowResourceIndex = _i;
    (&info->typeName)[windowResourceIndex] = s;
    (&noMatchInfo->typeName)[windowResourceIndex] = "gibberish";
    int mask = 1 << (_i + 2);
    for(int i = 0; i < MATCH_ANY_LITERAL; i++){
        Rule r = {s, i | LITERAL, NULL};
        Rule nullRule = {NULL, i | LITERAL, NULL};
        Rule filter = {s, i, NULL, .filterMatch = BIND(returnFalse)};
        Rule filterNull = {NULL, i, NULL, .filterMatch = BIND(returnFalse)};
        int result = ((i & mask) ? 1 : 0);
        assert(doesWindowMatchRule(&r, info) == result);
        assert(doesWindowMatchRule(&nullRule, info));
        assert(!doesWindowMatchRule(&r, noMatchInfo));
        assert(!doesWindowMatchRule(&r, nullInfo));
        assert(!doesWindowMatchRule(&r, NULL));
        assert(!doesWindowMatchRule(&filter, info));
        assert(!doesWindowMatchRule(&filterNull, info));
    }
    free(all);
}
END_TEST
START_TEST(test_wildcard_rule){
    Rule r = CREATE_WILDCARD(NULL);
    assert(doesWindowMatchRule(&r, NULL));
}
END_TEST
START_TEST(test_apply_rules){
    int count = 0;
    void funcNoArg(void){
        count++;
    }
    static ArrayList head;
    assert(applyRules(&head, NULL));
    int size = MATCH_ANY_LITERAL;
    size = 8;
    int target = 0;
    int eventIndex = 2;
    Rule r[size];
    for(int i = 0; i < size; i++){
        r[i] = (Rule)CREATE_WILDCARD(BIND(funcNoArg), .passThrough = i > size / 2 ? ALWAYS_PASSTHROUGH : NO_PASSTHROUGH);
        target += r[i].passThrough == ALWAYS_PASSTHROUGH;
        prependToList(getEventRules(eventIndex), &r[i]);
    }
    applyRules(getEventRules(eventIndex), NULL);
    assert(count == target + 1);
    clearList(&head);
}
END_TEST
START_TEST(test_passthrough_rules){
    ArrayList list = {0};
    Rule rules[] = {
        CREATE_WILDCARD(BIND(returnTrue), .passThrough = ALWAYS_PASSTHROUGH),
        CREATE_WILDCARD(BIND(returnTrue), .passThrough = PASSTHROUGH_IF_TRUE),
        CREATE_WILDCARD(BIND(returnFalse), .passThrough = PASSTHROUGH_IF_FALSE),
    };
    for(int i = 0; i < LEN(rules); i++)
        addToList(&list, &rules[i]);
    for(int i = 0; i < LEN(rules); i++)
        assert(applyRules(&list, NULL) == 1);
    Rule noPassThrough = CREATE_WILDCARD(BIND(returnFalse), .passThrough = NO_PASSTHROUGH);
    addToList(&list, &noPassThrough);
    assert(!applyRules(&list, NULL));
    clearList(&list);
}
END_TEST

START_TEST(test_add_remove_rule){
    static Rule rules[3];
    int size = LEN(rules);
    for(int n = 0; n < 6; n++){
        ArrayList*(*ruleGetter)() = n % 2 ? getEventRules : getBatchEventRules;
        void (*ruleClearer)(void) = n % 2 ? clearAllRules : clearAllBatchRules;
        for(int i = 0; i < size; i++){
            if(n % 2)
                addToList(ruleGetter(n), &rules[i]);
            else prependToList(ruleGetter(n), &rules[i]);
            assert(getSize(ruleGetter(n)) == i + 1);
            assert(getElement(ruleGetter(n), 0) == (n % 2 ? &rules[0] : &rules[i]));
        }
        if(n < 2)
            for(int i = 0; i < size; i++){
                assert(getSize(ruleGetter(n)) == size - i);
                removeElementFromList(ruleGetter(n), &rules[i], sizeof(Rule));
            }
        else
            ruleClearer();
        assert(getSize(ruleGetter(n)) == 0);
        if(n == 4)size = 1;
    }
}
END_TEST


static void setup(){
    DEFAULT_BINDING_MASKS = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS;
    createContextAndSimpleConnection();
    setCurrentMode(ALL_MODES);
}
Suite* bindingsSuite(void){
    Suite* s = suite_create("Bindings");
    TCase* tc_core;
    tc_core = tcase_create("Bindings");
    tcase_add_checked_fixture(tc_core, setup, destroyContextAndConnection);
    tcase_add_test(tc_core, test_init_binding);
    tcase_add_test(tc_core, test_binding_add);
    tcase_add_test(tc_core, test_binding_target_id);
    tcase_add_test(tc_core, test_binding_match);
    tcase_add_test(tc_core, test_binding_mode_match);
    tcase_add_test(tc_core, test_call_bounded_function);
    tcase_add_test(tc_core, test_bounded_function_negate_result);
    tcase_add_test(tc_core, test_and_or_short_circuit);
    tcase_add_loop_test(tc_core, test_check_bindings, 0, 2);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Chain Bindings");
    tcase_add_checked_fixture(tc_core, setup, destroyContextAndConnection);
    tcase_add_test(tc_core, test_init_chain_binding);
    tcase_add_test(tc_core, test_start_end_chain);
    tcase_add_test(tc_core, test_chain_bindings);
    tcase_add_loop_test(tc_core, test_chain_grab, 0, 1);
    tcase_add_loop_test(tc_core, test_chain_grab, 1, 2);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Rule");
    tcase_add_test(tc_core, test_env_rule_sub);
    tcase_add_loop_test(tc_core, test_literal_string_rule, 0, 2);
    tcase_add_test(tc_core, test_string_rule);
    tcase_add_loop_test(tc_core, test_window_matching, 0, 4);
    tcase_add_test(tc_core, test_wildcard_rule);
    tcase_add_test(tc_core, test_passthrough_rules);
    tcase_add_test(tc_core, test_apply_rules);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Rule_Loading_Events");
    tcase_add_test(tc_core, test_add_remove_rule);
    suite_add_tcase(s, tc_core);
    return s;
}
