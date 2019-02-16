
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
#include "../monitors.h"
#include "../workspaces.h"
#include "../masters.h"
#include "../monitors.h"

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

static inline int addFakeMaster(int pointerId,int keyboardID){
    return addMaster(pointerId,keyboardID,"",0);
}
static inline void addFakeMonitor(int id){
    short arr[4]={0};
    updateMonitor(id,0,arr);
}

static inline int listsEqual(ArrayList*n,ArrayList*n2){
    if(n==n2)
        return 1;
    if(getSize(n)!=getSize(n2))
        return 0;
    for(int i=0;i<getSize(n);i++)
        if(*(int*)getElement(n,i)!=*(int*)getElement(n2,i))
            return 0;
    return 1;
}
#endif /* TESTS_UNITTESTS_H_ */
