#include "../UnitTests.h"

#include "TestMyWmUtil.c"
#include "TestUtil.c"


Layout DEFAULT_LAYOUTS[]={0};
int NUMBER_OF_DEFAULT_LAYOUTS=1;

int getLogLevel(){return 0;}


int main(void) {
    //signal(SIGSEGV, handler);   // install our handler
    //signal(SIGABRT, handler);   // install our handler

    SRunner *runner;
    runner = srunner_create(utilSuite());
    srunner_add_suite(runner,mywmUtilSuite());
    //srunner_add_suite(runner,bindingsSuite());
    srunner_run_all(runner, CK_NORMAL);
    return srunner_ntests_failed(runner);
}

