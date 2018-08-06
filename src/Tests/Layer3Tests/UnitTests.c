#include "../UnitTests.h"

extern Suite *x11Suite();

extern Suite *defaultRulesSuite();

extern Suite *stateSuite();
extern Suite *layoutSuite();
CREATE_HANDLER


int main(void) {
    signal(SIGSEGV, handler);   // install our handler
    signal(SIGABRT, handler);   // install our handler

    SRunner *runner;
    runner = srunner_create(stateSuite());


    srunner_add_suite(runner, x11Suite());
    srunner_add_suite(runner, defaultRulesSuite());
    srunner_add_suite(runner, layoutSuite());

    srunner_run_all(runner, CK_NORMAL);
    int failures=srunner_ntests_failed(runner);
    srunner_free(runner);
    return failures;
}

