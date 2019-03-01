#include "../UnitTests.h"
#include "../../xsession.h"

extern Suite* bindingsSuite();
extern Suite* devicesSuite();
extern Suite* xsessionSuite();
extern Suite* testFunctionSuite();

extern Suite* monitorsSuite();
extern Suite* eventSuite();
extern Suite* windowsSuite();
extern Suite* mywmUtilSuite();

CREATE_HANDLER

int main(int argc, char** argv __attribute__((unused))){
    if(argc > 1){
        if(!fork())
            openXDisplay();
        else
            wait(NULL);
        exit(0);
    }
    signal(SIGSEGV, handler);   // install our handler
    signal(SIGABRT, handler);   // install our handler
    signal(SIGUSR1, handler);
    SRunner* runner;
    runner = srunner_create(xsessionSuite());
    srunner_add_suite(runner, testFunctionSuite());
    srunner_add_suite(runner, mywmUtilSuite());
    srunner_add_suite(runner, devicesSuite());
    srunner_add_suite(runner, monitorsSuite());
    srunner_add_suite(runner, bindingsSuite());
    srunner_add_suite(runner, eventSuite());
    srunner_add_suite(runner, windowsSuite());
    srunner_run_all(runner, CK_NORMAL);
    int failures = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failures;
}
