
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


#define WAIT_UNTIL_FALSE(COND,EXPR...) while(COND){msleep(100);EXPR;}
#define WAIT_UNTIL_TRUE(COND,EXPR...) WAIT_UNTIL_FALSE(!(COND),EXPR)

#define CREATE_HANDLER void handler(int sig) { \
  int n=30; \
  void *array[n]; \
   fprintf(stderr, "Error: signal %d:\n", sig); \
  backtrace_symbols_fd(array, backtrace(array, n), STDERR_FILENO); \
  exit(1); \
}

#endif /* TESTS_UNITTESTS_H_ */
