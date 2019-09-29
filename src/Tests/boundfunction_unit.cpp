
#include <stdlib.h>

#include "../boundfunction.h"
#include "test-mpx-helper.h"
#include "test-event-helper.h"
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
    BoundFunction* b2 = new BoundFunction(incrementCount, name);
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
    static WindowInfo* fakeWinInfo = new WindowInfo(1);
    BoundFunction b[] = {
        {+[](WindowInfo * winInfo) {incrementCount(); assertEquals(fakeWinInfo, winInfo); return 0;}},
        incrementCount,
        {+[]()->int{incrementCount(); return 0;}},
        {+[](WindowInfo * winInfo)->bool{incrementCount(); assertEquals(fakeWinInfo, winInfo); return 0;}},
        {+[](uint32_t i) {incrementCount(); assertEquals(fakeWinInfo->getID(), i);}},
        {+[](uint32_t i)->int{incrementCount(); assertEquals(fakeWinInfo->getID(), i); return i;}},
        {+[](WindowInfo * winInfo) { incrementCount(); assertEquals(fakeWinInfo, winInfo);}},
    };
    for(int i = 0; i < LEN(b);) {
        b[i].execute(fakeWinInfo);
        assert(getCount() == ++i);
    }
    delete fakeWinInfo;
});
MPX_TEST("test_call_bounded_function_null_window", {
    static WindowInfo* fakeWinInfo = NULL;
    BoundFunction funcs[] = {
        {+[](WindowInfo * winInfo) {assert(winInfo); quit(1);}},
        {+[](WindowInfo * winInfo)->bool{assert(winInfo); quit(1); return 0;}},
        {+[](uint32_t i) {assert(i); quit(1);}},
        {+[](uint32_t i)->int{quit(1); return i;}},
        {+[](WindowInfo * winInfo) {assert(winInfo); quit(1);}},
    };
    for(BoundFunction& b : funcs)
        b.execute(fakeWinInfo);
});
MPX_TEST("apply_rules_passthrough", {
    getEventRules(0).add(new BoundFunction {NO_PASSTHROUGH});
    getEventRules(0).add(new BoundFunction{[]() {exit(10);}});
    getBatchEventRules(0).add(new BoundFunction{incrementCount});
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
    for(int i = 0; i < MPX_LAST_EVENT; i++) {
        getEventRules(i).add(b);
        assert(applyEventRules(i, NULL));
        assertEquals(counter, i);
        assert(getNumberOfEventsTriggerSinceLastIdle(i) == 1);
        getBatchEventRules(i).add(b2);
    }
    assertEquals(counter2, -1);
    applyBatchEventRules();
    assertEquals(counter, counter2);
    for(int i = 0; i < MPX_LAST_EVENT; i++)
        assert(getNumberOfEventsTriggerSinceLastIdle(i) == 0);
});
