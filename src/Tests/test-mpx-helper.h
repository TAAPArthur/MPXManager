#ifndef MPX_TEST_MPX_HELPER
#define MPX_TEST_MPX_HELPER

#include <assert.h>
#include <err.h>
#include "tester.h"
#include "../logger.h"
#include "../system.h"
#include "../mywm-structs.h"
#include "../monitors.h"
#include "../windows.h"
#include "../masters.h"
#include "../workspaces.h"
#include "../debug.h"

static inline void createSimpleEnv(void) {
    addDefaultMaster();
    addWorkspaces(1);
}
static inline bool addWindowInfo(WindowInfo* winInfo) {
    return getAllWindows().addUnique(winInfo);
}
static inline void simpleCleanup() {
    validate();
    suppressOutput();
    printSummary();
}
static volatile int count = 0;
static inline void incrementCount(void) {
    LOG_RUN(0, printStackTrace());
    count++;
}
static inline void decrementCount(void) {
    LOG_RUN(0, printStackTrace());
    count--;
}
static inline int getCount() {return count;}
#endif
