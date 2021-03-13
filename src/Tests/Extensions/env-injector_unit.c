#include <scutest/tester.h>
#include <stdlib.h>
#include "../../system.h"
#include "../tester.h"
#include "../test-wm-helper.h"
#include "../../Extensions/env-injector.h"
static void childEnvSetup() {
    onChildSpawn = setClientMasterEnvVar;
}
SCUTEST_SET_ENV(childEnvSetup, simpleCleanup);
SCUTEST_ITER(spawn_env, 2) {
    bool large = _i;
    createSimpleEnv();
    WindowInfo* winInfo = addFakeWindowInfo(large ? 1 << 31 : 1);
    strcpy(winInfo->title,"someTitle");
    strcpy(winInfo->className,"someClass");
    strcpy(winInfo->instanceName,"someInstance");
    onWindowFocus(winInfo->id);
    Monitor*m=addFakeMonitor((Rect) {0, -3, 2, (large ? 1<<16 -1 : 1)});
    assignUnusedMonitorsToWorkspaces();
    int pid = spawnChild(NULL);
    if(!pid) {
        char buffer[255];
        sprintf(buffer, "%u", getActiveMasterKeyboardID());
        assertEqualsStr(getenv(DEFAULT_KEYBOARD_ENV_VAR_NAME), buffer) ;
        sprintf(buffer, "%u", getActiveMasterPointerID());
        assertEqualsStr(getenv(DEFAULT_POINTER_ENV_VAR_NAME), buffer) ;
        sprintf(buffer, "%u", winInfo->id);
        assertEqualsStr(getenv("_WIN_ID"), buffer);
        assertEqualsStr(getenv("_WIN_TITLE"), winInfo->title);
        assertEqualsStr(getenv("_WIN_CLASS"), winInfo->className);
        assertEqualsStr(getenv("_WIN_INSTANCE"), winInfo->instanceName);
        assertEqualsStr(getenv("_MON_X"),"0");
        sprintf(buffer, "%u", m->base.y);
        assertEqualsStr(getenv("_MON_Y"), buffer);
        sprintf(buffer, "%u", m->base.width);
        assertEqualsStr(getenv("_MON_WIDTH"), buffer);
        sprintf(buffer, "%u", m->base.height);
        spawnAndWait("env");
        assertEqualsStr(getenv("_MON_HEIGHT"), buffer);
        simpleCleanup();
        quit(0);
    }
    assertEquals(waitForChild(pid), 0);
}

