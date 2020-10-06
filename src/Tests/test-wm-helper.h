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
#include "../wmfunctions.h"
#include "test-mpx-helper.h"
#include "../xevent.h"
#include "../settings.h"
#include "test-x-helper.h"
#include "test-event-helper.h"
#include "tester.h"


static inline WindowID getWMPrivateWindow() {
    return isMPXManagerRunning();
}
static inline long getWMIdleCount() {
    return getWindowPropertyValueInt(getWMPrivateWindow(), MPX_IDLE_PROPERTY, XCB_ATOM_CARDINAL);
}

static inline void waitUntilWMIdle() {
    static int idleCount;
    flush();
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

static inline void verifyWindowStack(const ArrayList* stack, const WindowID win[3]) {
    int i = 0;
    FOR_EACH(WindowInfo*, winInfo, stack) {
        assertEquals(winInfo->id, win[i++]);
    }
}

#endif
