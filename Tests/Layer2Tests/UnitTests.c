#include "../UnitTests.h"

#include "../../mywm-structs.h"

#include "TestBindings.c"
#include "TestDevices.c"


Layout DEFAULT_LAYOUTS[]={0};
int NUMBER_OF_DEFAULT_LAYOUTS=1;



int main(void) {
    //signal(SIGSEGV, handler);   // install our handler
    //signal(SIGABRT, handler);   // install our handler

    SRunner *runner;
    runner = srunner_create(bindingsSuite());
    srunner_add_suite(runner,devicesSuite());
    srunner_run_all(runner, CK_NORMAL);
    return srunner_ntests_failed(runner);
}
