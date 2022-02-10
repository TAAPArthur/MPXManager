#include <stdlib.h>
#include "../../system.h"
#include "../tester.h"
#include "../test-wm-helper.h"
#include "../../Extensions/env-injector.h"
static void setup() {
    assert(getenv("LD_PRELOAD"));
    onChildSpawn = setClientMasterEnvVar;
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

