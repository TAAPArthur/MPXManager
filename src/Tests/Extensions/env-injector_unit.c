#include <scutest/tester.h>
#include <stdlib.h>
#include "../../system.h"
#include "../tester.h"
#include "../test-wm-helper.h"
#include "../../Extensions/env-injector.h"
static void childEnvSetup() {
    LD_PRELOAD_INJECTION = 1;
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
        assert(getenv("LD_PRELOAD"));
        simpleCleanup();
        quit(0);
    }
    assertEquals(waitForChild(pid), 0);
}

static void setup() {
    childEnvSetup();
    createXSimpleEnv();
    focusWindow(root);
    createMasterDevice("name");
    initCurrentMasters();
    setActiveMaster(getElement(getAllMasters(), 1));
    focusWindow(root);
}
SCUTEST_SET_ENV(setup, cleanupXServer);

SCUTEST_ITER(grab, 3) {
    assertEquals(getActiveFocusOfMaster(DEFAULT_KEYBOARD), root);
    assertEquals(getActiveFocus(), root);
    assert(DEFAULT_POINTER != getActiveMasterPointerID());
    assert(!grabPointer(DEFAULT_POINTER));
    assert(!grabKeyboard(DEFAULT_KEYBOARD));
    Display* dpy;
    int pid = spawnChild(NULL);
    if(!pid) {
        if(_i == 0) {
            dpy = XOpenDisplay(NULL);
            assert(!XGrabKeyboard(dpy, root, GrabModeSync, 1, GrabModeSync, 0));
            XCloseDisplay(dpy);
        }
        else if(_i == 1) {
            dpy = XOpenDisplay(NULL);
            assert(!XGrabPointer(dpy, root, 0, 0, GrabModeSync, GrabModeSync, 0, 0, 0));
            XCloseDisplay(dpy);
        }
        else {
            dpy = XOpenDisplay(NULL);
            assert(!XGrabKeyboard(dpy, root, 1, GrabModeAsync, GrabModeAsync, 0));
            XCloseDisplay(dpy);
        }
        exit(0);
    }
    assertEquals(waitForChild(pid), 0);
}
