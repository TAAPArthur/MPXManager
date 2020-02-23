#ifndef MPX_TEST_MPX_HELPER
#define MPX_TEST_MPX_HELPER

#include "../debug.h"
#include "../logger.h"
#include "../masters.h"
#include "../monitors.h"
#include "../mywm-structs.h"
#include "../system.h"
#include "../threads.h"
#include "../windows.h"
#include "../workspaces.h"

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
static inline int getAndResetCount() {
    auto c = count;
    count = 0;
    return c;
}
#endif
