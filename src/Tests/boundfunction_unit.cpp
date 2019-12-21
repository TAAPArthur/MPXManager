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
MPX_TEST("boundFunction_eq", {
    std::string name = "name";
    BoundFunction* b = new BoundFunction(incrementCount, name);
    BoundFunction* b2 = new BoundFunction(*b);
    assertEquals(b->getName(), name);
    assertEquals(b2->getName(), b->getName());
    assertEquals(*b2, *b);
    ArrayList<BoundFunction*>list = {b};
    assert(!list.add(*b, ADD_UNIQUE));
    assert(!list.add(*b, ADD_UNIQUE));
    assert(!list.add(*b2, ADD_UNIQUE));
    assert(!list.add(*b2, ADD_TOGGLE));
    assert(list.add(*b2, ADD_TOGGLE));
    assert(!list.add(*b2, ADD_REMOVE));
    assert(list.add(b2, ADD_ALWAYS));
    list.deleteElements();
});
///Test to make sure callBoundedFunction() actually calls the right function
MPX_TEST("test_call_bounded_function", {
    createSimpleEnv();
    getAllMonitors().add(new Monitor(1, {0, 0, 1, 1}));
    getAllMonitors()[0]->assignWorkspace(getActiveMaster()->getWorkspace());
    addRootMonitor();
    static WindowInfo* fakeWinInfo = new WindowInfo(1);
    BoundFunction b[] = {
        {+[](WindowInfo * winInfo) {incrementCount(); assertEquals(fakeWinInfo, winInfo); return 1;}},
        incrementCount,
        {+[](WindowInfo * winInfo)->bool{incrementCount(); assertEquals(fakeWinInfo, winInfo); return 1;}},
        {+[](Master * master)->bool{incrementCount(); assertEquals(getActiveMaster(), master); return 1;}},
        {+[](Workspace * w)->bool{incrementCount(); assertEquals(getActiveMaster()->getWorkspace(), w); return 1;}},
        {+[](Monitor * m)->bool{incrementCount(); assertEquals(getActiveMaster()->getWorkspace()->getMonitor(), m); return 1;}},
        {+[]()->int{incrementCount(); return 1;}},
        {+[](uint32_t i) {incrementCount(); assertEquals(fakeWinInfo->getID(), i);}},
        {+[](uint32_t i)->int{incrementCount(); assertEquals(fakeWinInfo->getID(), i); return i;}},
        {+[](WindowInfo * winInfo) { incrementCount(); assertEquals(fakeWinInfo, winInfo);}},
        {+[](int i) {incrementCount(); assertEquals(i, 123);}, 123},
        {+[](int i)->int{incrementCount(); assertEquals(i, 123); return 1;}, 123},
        {+[](std::string s)->void{incrementCount(); assertEquals(s, "123");}, "123"},
        {+[](std::string s)->int{incrementCount(); assertEquals(s, "123"); return 1;}, "123"},
    };
    BoundFunction f = BoundFunction(b[0]);
    for(int i = 0; i < LEN(b);) {
        getEventRules(0).add(b[i]);
        b[i].execute(fakeWinInfo, getActiveMaster());
        assertEquals(getCount(), ++i);
    }
    applyEventRules(0, fakeWinInfo);
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
        {+[](uint32_t i) {assert(i); quit(1);}},
        {+[](uint32_t i)->int{quit(1); return i;}},
        {+[](WindowInfo * winInfo) {assert(winInfo); quit(1);}},
        {+[](Master * m) {assert(m); quit(1);}},
        {+[](Workspace * w) {assert(w); quit(1);}},
        {+[](Monitor * m) {assert(m); quit(1);}},
    };
    for(BoundFunction& b : funcs)
        b.execute(fakeWinInfo, fakeMaster);
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
