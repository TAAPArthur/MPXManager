#include "../UnitTests.h"

#include "../../mywm-structs.h"




int main(void) {
    signal(SIGSEGV, handler);   // install our handler
    signal(SIGABRT, handler);   // install our handler

    SRunner *runner;
    runner = srunner_create(x11Suite());
    srunner_run_all(runner, CK_NORMAL);
    return srunner_ntests_failed(runner);
}
