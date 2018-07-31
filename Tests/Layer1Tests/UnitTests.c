#include "../UnitTests.h"


//int getLogLevel(){return 0;}

extern Suite *utilSuite();
extern Suite *mywmUtilSuite();
CREATE_HANDLER
int main(void) {
    signal(SIGSEGV, handler);   // install our handler
    signal(SIGABRT, handler);   // install our handler

    SRunner *runner;
    runner = srunner_create(utilSuite());
    srunner_add_suite(runner,mywmUtilSuite());
    srunner_run_all(runner, CK_NORMAL);
    return srunner_ntests_failed(runner);
}

