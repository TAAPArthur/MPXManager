#ifndef TESTS_UNITTESTS_H_
#define TESTS_UNITTESTS_H_

#include <assert.h>
#include <check.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../mywm-util.h"
#include "../monitors.h"
#include "../workspaces.h"
#include "../masters.h"
#include "../monitors.h"

//const int size=10;


#define WAIT_UNTIL_TRUE(COND,EXPR...) do{msleep(10);EXPR;}while(!(COND))


static inline int addFakeMaster(int pointerId, int keyboardID){
    static char dummyName[] = {'a', 0};
    return addMaster(pointerId, keyboardID, dummyName, 0);
}
static inline void addFakeMonitor(int id){
    Rect arr = {0};
    updateMonitor(id, arr, 1);
}

static inline int listsEqual(ArrayList* n, ArrayList* n2){
    if(n == n2)
        return 1;
    if(getSize(n) != getSize(n2))
        return 0;
    for(int i = 0; i < getSize(n); i++)
        if(*(int*)getElement(n, i) != *(int*)getElement(n2, i))
            return 0;
    return 1;
}
#endif /* TESTS_UNITTESTS_H_ */
