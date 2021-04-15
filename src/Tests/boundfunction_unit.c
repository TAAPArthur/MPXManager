#include "../masters.h"
#include "../monitors.h"
#include "../mywm-structs.h"
#include "../windows.h"
#include "../boundfunction.h"
#include "test-mpx-helper.h"
#include "tester.h"
#include <string.h>

//static BoundFunction func[] = { };
SCUTEST_SET_ENV(NULL, clearAllRules);
static void assertEqual1(void* p) {incrementCount(); assertEquals(p, 1);}
SCUTEST(test_apply_rule_with_context) {
    addEvent(0, DEFAULT_EVENT(assertEqual1));
    applyEventRules(0, (void*)1);
    assertEquals(1, getCount());
}

SCUTEST(test_add_apply_rules) {
    BoundFunction func = DEFAULT_EVENT(incrementCount);
    assert(!func.intFunc);
    for(int i = 0; i < NUMBER_OF_MPX_EVENTS; i++) {
        addEvent(i, func);
        addBatchEvent(i, func);
    }
    for(int i = 0; i < NUMBER_OF_MPX_EVENTS; i++)
        applyEventRules(i, NULL);
    assertEquals(getCount(), NUMBER_OF_MPX_EVENTS);
    applyBatchEventRules();
    assertEquals(getCount(), 2 * NUMBER_OF_MPX_EVENTS);
}

static void assertCount0() {assert(getCount() == 0);}
static void assertCount1() {assert(getCount() == 1);}
static void assertCount2() {assert(getCount() == 2);}
static void assertCount3() {assert(getCount() == 3);}
SCUTEST_ITER(test_rule_priority, 2) {
    BoundFunction func[] = {
        DEFAULT_EVENT(assertCount0, HIGHEST_PRIORITY),
        DEFAULT_EVENT(incrementCount, HIGHER_PRIORITY),
        DEFAULT_EVENT(assertCount1, HIGH_PRIORITY),
        DEFAULT_EVENT(incrementCount, NORMAL_PRIORITY),
        DEFAULT_EVENT(assertCount2, LOW_PRIORITY),
        DEFAULT_EVENT(incrementCount, LOWER_PRIORITY),
        DEFAULT_EVENT(assertCount3, LOWEST_PRIORITY),
    };
    for(int i = 0; i < LEN(func); i++) {
        addEvent(0, func[_i ? LEN(func) - 1 - i : i]);
    }
    applyEventRules(0, NULL);
    assertCount3();
}

SCUTEST(test_apply_rule_abort) {
    BoundFunction nonAbort = DEFAULT_EVENT(returnTrue);
    BoundFunction normal = USER_EVENT(incrementCount);
    BoundFunction abort = DEFAULT_EVENT(returnFalse, LOWEST_PRIORITY, .abort = 1);
    addEvent(0, normal);
    addEvent(0, nonAbort);
    addEvent(0, abort);
    applyEventRules(0, NULL);
    assertEquals(1, getCount());
    addEvent(0, DEFAULT_EVENT(returnFalse, HIGHEST_PRIORITY, .abort = 1));
    applyEventRules(0, NULL);
    assertEquals(1, getCount());
}
