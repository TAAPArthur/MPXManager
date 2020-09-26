#ifndef MPX_TEST_WM_HELPER
#define MPX_TEST_WM_HELPER

#include <assert.h>
#include "../bindings.h"
#include "../globals.h"
#include "../masters.h"
#include "../system.h"
#include "../user-events.h"
#include "../util/time.h"
#include "../wm-rules.h"
#include "test-mpx-helper.h"
#include "../util/threads.h"
#include "../xevent.h"
#include "../settings.h"
#include "test-x-helper.h"
#include "test-event-helper.h"
#include "tester.h"


static inline WindowID getWMPrivateWindow() {
    WindowID win;
    xcb_ewmh_get_supporting_wm_check_reply(ewmh, xcb_ewmh_get_supporting_wm_check(ewmh, root), &win, NULL);
    return win;
}
static inline long getWMIdleCount() {
    return getWindowPropertyValueInt(getWMPrivateWindow(), MPX_IDLE_PROPERTY, XCB_ATOM_CARDINAL);
}

static inline void waitUntilWMIdle() {
    static int idleCount;
    static int restartCounter;
    flush();
    if(restartCounter != getWindowPropertyValueInt(getWMPrivateWindow(), MPX_RESTART_COUNTER, XCB_ATOM_CARDINAL)) {
        idleCount = 0;
        restartCounter = getWindowPropertyValueInt(getWMPrivateWindow(), MPX_RESTART_COUNTER, XCB_ATOM_CARDINAL);
    }
    WAIT_UNTIL_TRUE(idleCount != getWMIdleCount());
    idleCount = getWMIdleCount();
}
static inline void onSimpleStartup() {
    set_handlers();
    addBasicRules();
    addWorkspaces(DEFAULT_NUMBER_OF_WORKSPACES);
    openXDisplay();
    assignUnusedMonitorsToWorkspaces();
    addShutdownOnIdleRule();
}

static inline void onDefaultStartup() {
    set_handlers();
    startupMethod = loadSettings;
    onStartup();
    addShutdownOnIdleRule();
}

void waitForAllThreadsToExit();

static inline void wakeupWM() {
    xcb_ewmh_send_client_message(dis, root, root, 1, 0, 0);
    flush();
}
static inline void fullCleanup() {
    DEBUG("full cleanup");
    if(!isLogging(LOG_LEVEL_DEBUG))
        setLogLevel(LOG_LEVEL_NONE);
    requestShutdown();
    if(getIdleCount() && ewmh) {
        registerForWindowEvents(root, ROOT_EVENT_MASKS);
        wakeupWM();
    }
    waitForAllThreadsToExit();
    DEBUG("validating state");
    DEBUG("Clearing bindings");
    clearBindings();
    DEBUG("Checking bound function names");
    DEBUG("cleaning up xserver");
    cleanupXServer();
}

static inline void resetState() {
    destroyAllLists();
    createSimpleEnv();
}

static inline void verifyWindowStack(ArrayList* stack, const WindowID win[3]) {
    int i = 0;
    FOR_EACH(WindowInfo*, winInfo, stack) {
        assertEquals(winInfo->id, win[i++]);
    }
}

#endif
