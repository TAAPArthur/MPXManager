#include "../UnitTests.h"

extern Suite* utilSuite();
int main(void){
    SRunner* runner;
    runner = srunner_create(utilSuite());
    srunner_run_all(runner, CK_NORMAL);
    int failures = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failures;
}

