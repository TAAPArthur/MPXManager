
#include "UnitTests.h"

#include "TestUtil.c"
#include "TestMyWmUtil.c"
#include "TestBindings.c"



void handler(int sig) {
  int n=30;
  void *array[n];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, n);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}
int main(void) {
    int no_failed = 0;

    signal(SIGSEGV, handler);   // install our handler
    signal(SIGABRT, handler);   // install our handler

    SRunner *runner;
    runner = srunner_create(utilSuite());
    srunner_add_suite(runner,mywmUtilSuite());
    srunner_add_suite(runner,bindingsSuite());


    srunner_run_all(runner, CK_NORMAL);
    no_failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return (no_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

