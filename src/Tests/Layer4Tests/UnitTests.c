#include "../TestX11Helper.h"
#include "../UnitTests.h"



extern Suite* windowCloneSuite();
extern Suite* mpxSuite();
extern Suite* functionsSuite();
extern Suite* xmousecontrolSuite();
extern Suite* communicationsSuite();
int main(void){
    SRunner* runner;
    runner = srunner_create(functionsSuite());
    srunner_add_suite(runner, windowCloneSuite());
    srunner_add_suite(runner, mpxSuite());
    srunner_add_suite(runner, xmousecontrolSuite());
    srunner_add_suite(runner, communicationsSuite());
    srunner_run_all(runner, CK_NORMAL);
    int failures = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failures;
}
