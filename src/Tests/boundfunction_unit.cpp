#include <stdlib.h>

#include "../boundfunction.h"
#include "test-event-helper.h"
#include "test-mpx-helper.h"
#include "tester.h"

SET_ENV(NULL, deleteAllRules);
MPX_TEST("print", {
    BoundFunction b = {};
    suppressOutput();
    std::cout << b;
});
MPX_TEST("print", {
    BoundFunction b = {};
    suppressOutput();
    std::cout << b;
});
MPX_TEST("boundFunction_eq", {
    std::string name = "name";
    BoundFunction b = BoundFunction(incrementCount, name);
    BoundFunction b2 = BoundFunction(b);
    assertEquals(b.getName(), name);
    assertEquals(b2.getName(), b.getName());
    assertEquals(b2, b);
});
///Test to make sure callBoundedFunction() actually calls the right function
MPX_TEST("test_call_bounded_function", {
    createSimpleEnv();
    addWorkspaces(2);
    getAllMonitors().add(new Monitor(1, {0, 0, 1, 1}));
    getAllMonitors().add(new Monitor(2, {1, 1, 1, 1}));
    getAllMasters().add(new Master(20, 21, ""));
    addRootMonitor();
    static WindowInfo* fakeWinInfo = new WindowInfo(1);
    static auto* targetWorkspace = getWorkspace(1);
    static auto* targetMonitor = getAllMonitors()[1];
    static auto* targetMaster = getAllMasters()[1];
    static uint32_t integer = 2;
    static std::string str = "string";
    BoundFunction b[] = {
        {[](WindowInfo * winInfo) {incrementCount(); assertEquals(fakeWinInfo, winInfo); return 1;}},
        incrementCount,
        []{incrementCount();},
        {[](WindowInfo * winInfo)->bool{incrementCount(); assertEquals(fakeWinInfo, winInfo); return 1;}},
        {[](Master * master)->bool{incrementCount(); assertEquals(targetMaster, master); return 1;}},
        {[](Workspace * w)->bool{incrementCount(); assertEquals(targetWorkspace, w); return 1;}},
        {[](Monitor * m)->bool{incrementCount(); assertEquals(targetMonitor, m); return 1;}},
        {[]()->int{incrementCount(); return 1;}},
        {[](std::string * s) {incrementCount(); assertEquals(&str, s);}},
        {[](uint32_t i) {incrementCount(); assertEquals(integer, i);}},
        {[](uint32_t i)->int{incrementCount(); assertEquals(integer, i); return i;}},
        {[](WindowInfo * winInfo) { incrementCount(); assertEquals(fakeWinInfo, winInfo);}},
        {+[](int i) {incrementCount(); assertEquals(i, 123);}, 123},
        {+[](int i)->int{incrementCount(); assertEquals(i, 123); return 1;}, 123},
        {+[](std::string s)->void{incrementCount(); assertEquals(s, "123");}, "123"},
        {+[](std::string s)->int{incrementCount(); assertEquals(s, "123"); return 1;}, "123"},
    };
    BoundFunctionArg arg = {.winInfo = fakeWinInfo, .master = targetMaster, .workspace = targetWorkspace, .monitor = targetMonitor, .integer = integer, .string = &str};
    for(int i = 0; i < LEN(b);) {
        getEventRules(0).add(b[i]);
        b[i].execute(arg);
        assertEquals(getCount(), ++i);
    }
    applyEventRules(0, arg);
    assertEquals(getCount(), LEN(b) * 2);
    delete fakeWinInfo;
    getEventRules(0).deleteElements();
    simpleCleanup();
});
MPX_TEST("test_call_bounded_function_null_window", {
    static WindowInfo* fakeWinInfo = NULL;
    static Master* fakeMaster = NULL;
    BoundFunction funcs[] = {
        {+[](WindowInfo * winInfo) {assert(winInfo); quit(1);}},
        {+[](WindowInfo * winInfo)->bool{assert(winInfo); quit(1); return 0;}},
        {+[](WindowInfo * winInfo) {assert(winInfo); quit(1);}},
        {+[](Master * m) {assert(m); quit(1);}},
        {+[](Workspace * w) {assert(w); quit(1);}},
        {+[](Monitor * m) {assert(m); quit(1);}},
    };
    for(BoundFunction& b : funcs)
        b.execute({fakeWinInfo, fakeMaster});
});
MPX_TEST("clear_rules", {
    getEventRules(0).add({exitFailure});
    getBatchEventRules(0).add({exitFailure});
    clearAllRules();
    applyEventRules(0);
    applyBatchEventRules();
});
MPX_TEST_ITER("apply_rules", 2, {
    for(int i = 0; i < NUMBER_OF_MPX_EVENTS; i++)
        applyEventRules(i);
    if(_i)
        applyBatchEventRules();
});
MPX_TEST_ITER("apply_rules_passthrough", 2, {
    if(_i == 2) {
        getEventRules(0).add(new BoundFunction {NO_PASSTHROUGH});
    }
    else {
        static int returnValue = !_i;
        getEventRules(0).add(BoundFunction(+[] { return returnValue;}, "", _i ? PASSTHROUGH_IF_TRUE : PASSTHROUGH_IF_FALSE));
    }
    getEventRules(0).add({[]() {exit(10);}});
    getBatchEventRules(0).add({incrementCount});
    assert(!applyEventRules(0, NULL));
    assert(getCount() == 0);
    applyBatchEventRules();
    assert(getCount() == 1);
});
MPX_TEST("apply_rules_passthrough_default", {
    getEventRules(0).add(new BoundFunction());
    getEventRules(0).add(new BoundFunction(incrementCount));
    assert(applyEventRules(0, NULL));
});
MPX_TEST("apply_rules_counter", {
    static int counter = -1;
    static int counter2 = -1;
    static BoundFunction b = {[]() {counter++;}};
    static BoundFunction b2 = {[]() {counter2++;}};
    for(int i = 0; i < NUMBER_OF_BATCHABLE_EVENTS; i++) {
        getEventRules(i).add(b);
        assert(applyEventRules(i, NULL));
        assertEquals(counter, i);
        assert(getNumberOfEventsTriggerSinceLastIdle(i) == 1);
        getBatchEventRules(i).add(b2);
    }
    assertEquals(counter2, -1);
    applyBatchEventRules();
    assertEquals(counter, counter2);
    for(int i = 0; i < NUMBER_OF_BATCHABLE_EVENTS; i++)
        assert(getNumberOfEventsTriggerSinceLastIdle(i) == 0);
});
MPX_TEST("priority_order", {
    BoundFunction func[] = {
        {[]() {assert(getCount()); incrementCount();}, "", PASSTHROUGH_IF_TRUE, .priority = 0},
        {[]() {assertEquals(getCount(), 2);}, "", PASSTHROUGH_IF_TRUE, .priority = 1},
        {[]() {assertEquals(getCount(), 2); incrementCount();}, "", PASSTHROUGH_IF_TRUE, .priority = 1},
        {[]() {assert(!getCount()); incrementCount();}, "", PASSTHROUGH_IF_TRUE, -11},
    };
    for(auto& b : func)
        getEventRules(0).add(b);
    assert(applyEventRules(0, NULL));
    assertEquals(getCount(), 3);

});
