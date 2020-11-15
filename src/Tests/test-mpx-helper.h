#ifndef MPX_TEST_MPX_HELPER
#define MPX_TEST_MPX_HELPER

#include "../util/debug.h"
#include "../util/logger.h"
#include "../masters.h"
#include "../monitors.h"
#include "../mywm-structs.h"
#include "../windows.h"
#include "../workspaces.h"
#include "tester.h"

#define assertEqualsRect(A,B)do{fflush(NULL);Rect __b=B; dumpRect(A);dumpRect(__b);assert(memcmp(&A, &__b, sizeof(Rect))==0);}while(0)
static inline WindowInfo* addFakeWindowInfo(WindowID win) {
    return newWindowInfo(win, 0);
}


static inline Master* addFakeMaster(MasterID pointerID, MasterID keyboardID) {
    return newMaster(pointerID, keyboardID, "");
}
static inline Monitor* addDummyMonitor() {
    return addFakeMonitor((Rect) {0, 0, 1, 1});
}
static inline WindowInfo* getWindowByIndex(int index) {
    return getElement(getAllWindows(), index);
}
static inline void destroyAllLists() {
    setActiveMaster(NULL);
    FOR_EACH_R(Monitor*, monitor, getAllMonitors()) {
        freeMonitor(monitor);
    }
    FOR_EACH_R(WindowInfo*, winInfo, getAllWindows()) {
        freeWindowInfo(winInfo);
    }
    FOR_EACH_R(Slave*, slave, getAllSlaves()) {
        freeSlave(slave);
    }
    assert(!getAllSlaves()->size);
    FOR_EACH_R(Master*, master, getAllMasters()) {
        freeMaster(master);
    }
    removeWorkspaces(getNumberOfWorkspaces() - 1);
    if(getNumberOfWorkspaces()) {
        extern ArrayList workspaces;
        void freeWorkspace(Workspace * workspace);
        freeWorkspace(pop(&workspaces));
    }
}

static inline void createSimpleEnv(void) {
    addDefaultMaster();
    addWorkspaces(1);
}
static inline void simpleCleanup() {
    destroyAllLists();
}
static inline void fail() {assert(0);}
#endif
