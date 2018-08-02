#include "../UnitTests.h"

#include "../../mywm-structs.h"

extern Suite *x11Suite();

extern Suite *defaultRulesSuite();
Layout DEFAULT_LAYOUTS[]={0};
int NUMBER_OF_DEFAULT_LAYOUTS=1;

CREATE_HANDLER


int main(void) {
    signal(SIGSEGV, handler);   // install our handler
    signal(SIGABRT, handler);   // install our handler

    SRunner *runner;
    runner = srunner_create(x11Suite());

    srunner_add_suite(runner,defaultRulesSuite());
    srunner_run_all(runner, CK_NORMAL);
    return srunner_ntests_failed(runner);
}

