#include "../UnitTests.h"
#include <X11/keysym.h>
#include "../../mywm-structs.h"
#include "../../defaults.h"
#include "../../bindings.h"

extern Suite *bindingsSuite();
extern Suite *devicesSuite();
extern Suite *xsessionSuite();
extern Suite *testFunctionSuite();
extern Suite *eventSuite();

char deviceBindingTriggerCount[]={0,0,0,0};
static void KP(){deviceBindingTriggerCount[0]++;}
static void KR(){deviceBindingTriggerCount[1]++;}
static void MP(){deviceBindingTriggerCount[2]++;}
static void MR(){deviceBindingTriggerCount[3]++;}
static Binding k[]={{0,XK_A,BIND_TO_FUNC(KP)},{0,XK_C,BIND_TO_FUNC(KP),.noGrab=1}};
static Binding k2[]={{0,XK_A,BIND_TO_FUNC(KR)},{0,XK_C,BIND_TO_FUNC(KR),.noGrab=1}};
static Binding m[]={{0,Button2,BIND_TO_FUNC(MP)},{0,Button3,BIND_TO_FUNC(MP),.noGrab=1}};
static Binding m2[]={{0,Button2,BIND_TO_FUNC(MR)},{0,Button3,BIND_TO_FUNC(MR),.noGrab=1}};

CREATE_HANDLER
int main(void) {
    setLogLevel(LOG_LEVEL_ERROR);
    for(int i=0;i<NUMBER_OF_EVENT_RULES;i++)
        eventRules[i]=createEmptyHead();

    deviceBindingLengths[0]=2; deviceBindings[0]=k;
    deviceBindingLengths[1]=2; deviceBindings[1]=k2;
    deviceBindingLengths[2]=2; deviceBindings[2]=m;
    deviceBindingLengths[3]=2; deviceBindings[3]=m2;

    signal(SIGSEGV, handler);   // install our handler
    signal(SIGABRT, handler);   // install our handler
    signal(SIGUSR1, handler);

    SRunner *runner;
    runner = srunner_create(xsessionSuite());

    srunner_add_suite(runner,testFunctionSuite());

    srunner_add_suite(runner,bindingsSuite());
    srunner_add_suite(runner,devicesSuite());
    srunner_add_suite(runner,eventSuite());
    srunner_run_all(runner, CK_NORMAL);
    int failures=srunner_ntests_failed(runner);
    srunner_free(runner);
    return failures;
}
