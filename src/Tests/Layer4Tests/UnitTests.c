#include "../TestX11Helper.h"
#include "../UnitTests.h"



extern Suite*functionsSuite();
CREATE_HANDLER
int main(void) {
    signal(SIGSEGV, handler);   // install our handler
    signal(SIGABRT, handler);   // install our handler

    SRunner *runner;
    runner = srunner_create(functionsSuite());
    //srunner_add_suite(runner,functionsSuite());
    srunner_run_all(runner, CK_NORMAL);
    int failures=srunner_ntests_failed(runner);
    srunner_free(runner);
    return failures;
}
