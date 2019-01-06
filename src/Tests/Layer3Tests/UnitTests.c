#include "../UnitTests.h"

extern Suite *stateSuite();
extern Suite *x11Suite();
extern Suite *layoutSuite();
extern Suite *functionsSuite();
extern Suite *defaultRulesSuite();

CREATE_HANDLER


int main(void) {
    signal(SIGSEGV, handler);   // install our handler
    signal(SIGABRT, handler);   // install our handler

    SRunner *runner;
    runner = srunner_create(stateSuite());
    srunner_add_suite(runner, x11Suite());
    srunner_add_suite(runner, layoutSuite());
    srunner_add_suite(runner, defaultRulesSuite());
    srunner_add_suite(runner,functionsSuite());

    srunner_run_all(runner, CK_NORMAL);
    int failures=srunner_ntests_failed(runner);
    srunner_free(runner);
    return failures;
}

