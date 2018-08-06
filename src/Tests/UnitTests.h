
#ifndef TESTS_UNITTESTS_H_
#define TESTS_UNITTESTS_H_

#include <check.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <check.h>

#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include<sys/wait.h>

#include "../mywm-util.h"

//const int size=10;


#define WAIT_FOR(EXPR)  while(EXPR)msleep(100)
#define WAIT_UNTIL_TRUE(EXPR) WAIT_FOR(!(EXPR));
#define WAIT_UNTIL(EXPR) WAIT_FOR((EXPR));
#define WAIT_UNTIL_FALSE(EXPR) WAIT_FOR((EXPR));

#define CREATE_HANDLER void handler(int sig) { \
  int n=30; \
  void *array[n]; \
   fprintf(stderr, "Error: signal %d:\n", sig); \
  backtrace_symbols_fd(array, backtrace(array, n), STDERR_FILENO); \
  exit(1); \
}

#endif /* TESTS_UNITTESTS_H_ */
