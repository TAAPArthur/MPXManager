#include "../../Extensions/session.h"
#include "../../Extensions/ewmh.h"
#include "../../devices.h"
#include "../../tile.h"
#include "../../layouts.h"
#include "../../masters.h"
#include "../../wm-rules.h"
#include "../../wmfunctions.h"
#include "../test-event-helper.h"
#include "../test-mpx-helper.h"
#include "../test-x-helper.h"
#include "../test-wm-helper.h"
#include "../tester.h"
static void loadCustomState() {
    loadSavedNonWindowState();
    loadSavedWindowState();
}


static void setup() {
    //addEWMHRules();
    addResumeCustomStateRules();
    onDefaultStartup();
}

static inline void resetState() {
    destroyAllLists();
    createSimpleEnv();
    addWorkspaces(DEFAULT_NUMBER_OF_WORKSPACES);
    initCurrentMasters();
    detectMonitors();
    scan(root);
}

static void saveResetState() {
    saveCustomState();
    saveMonitorWorkspaceMapping();
    resetState();
    spawnAndWait("xprop -root");
}
static void saveResetReloadState() {
    saveResetState();
    loadCustomState();
}

SCUTEST_SET_ENV(setup, cleanupXServer);
SCUTEST(test_restore_workspace_layouts) {
    char longName[MAX_NAME_LEN];
    for(int i = 0; i < LEN(longName) - 1; i++)
        longName[i] = 'A';
    longName[LEN(longName) - 1] = 0;
    Layout l = {longName, NULL};
    registerLayout(&l);
    toggleActiveLayout(&l);
    saveResetReloadState();
    assertEquals(getActiveLayout(), &l);
}

SCUTEST(test_restore_master_focus_stack) {
    createMasterDevice("test");
    createMasterDevice("test2");
    initCurrentMasters();
    WindowID wins[] = {createNormalWindow(), createNormalWindow(), createNormalWindow()};
    int i = 0;
    FOR_EACH(Master*, master, getAllMasters()) {
        WindowID win = mapWindow(createNormalWindow());
        registerWindow(win, root, NULL);
        focusWindowAsMaster(win, master);
        mapWindow(wins[i]);
        registerWindow(wins[i], root, NULL);
        focusWindowAsMaster(wins[i], master);
        i++;
    }
    runEventLoop();
    FOR_EACH(Master*, master, getAllMasters()) {
        assertEquals(getMasterWindowStack(master)->size, 2);
    }
    saveResetReloadState();
    i = 0;
    FOR_EACH(Master*, master, getAllMasters()) {
        assertEquals(getMasterWindowStack(master)->size, 2);
        assertEquals(getFocusedWindowOfMaster(master)->id, wins[i]);
        assertEquals(getActiveFocusOfMaster(master->id), wins[i]);
        i++;
    }
}

SCUTEST(test_restore_workspace_order) {
    addEWMHRules();
    WindowID wins[] = {
        createNormalWindow(),
        createNormalWindow(),
        createNormalWindow(),
        createNormalWindow(),
        createNormalWindow(),
    };
    raiseWindow(wins[3], 0);
    runEventLoop();
    raiseWindow(wins[1], 0);
    lowerWindow(wins[2], 0);
    verifyWindowStack(getActiveWindowStack(), wins);
    saveResetReloadState();
    verifyWindowStack(getActiveWindowStack(), wins);
}
SCUTEST(test_restore_active_master) {
    createMasterDevice("test");
    createMasterDevice("test2");
    initCurrentMasters();
    setActiveMaster(getMasterByName("test"));
    saveResetReloadState();
    initCurrentMasters();
    assertEquals(getActiveMaster(), getMasterByName("test"));
}

SCUTEST(test_restore_state_window_masks_change, .iter = 2) {
    WindowID win = mapWindow(createNormalWindow());
    scan(root);
    addMask(getWindowInfo(win), FLOATING_MASK);
    saveResetReloadState();
    assert(hasMask(getWindowInfo(win), FLOATING_MASK));
    if(_i) {
        addMask(getWindowInfo(win), FULLSCREEN_MASK);
        runEventLoop();
        resetState();
        loadCustomState();
        runEventLoop();
        assert(hasMask(getWindowInfo(win), FULLSCREEN_MASK));
    }
}
SCUTEST(test_restore_state_clear_previous) {
    WindowID win = mapWindow(createNormalWindow());
    scan(root);
    WindowInfo* winInfo = getWindowInfo(win);
    addMask(winInfo, FLOATING_MASK);
    saveCustomState();
    removeMask(winInfo, FLOATING_MASK);
    // needs to clear the previously saved value
    saveCustomState();
    loadCustomState();
    assert(!hasMask(winInfo, hasMask(winInfo, FLOATING_MASK)));
}

SCUTEST(test_restore_state_monitor_change_fake) {
    CRASH_ON_ERRORS = -1;
    Rect bounds[] = {{0, 20, 100, 100}, {0, 40, 100, 100}};
    const char* names[255] = {"name1", "name2"};
    WorkspaceID workspaces[2] = {4, 7};
    for(int i = 0; i < LEN(bounds); i++)
        assignWorkspace(addFakeMonitorWithName(bounds[i], names[i]), getWorkspace(workspaces[i]));
    saveResetReloadState();
    assertEquals(getAllMonitors()->size, 3);
    for(int i = 0; i < LEN(bounds); i++)
        assertEqualsRect(bounds[i], getMonitorByName(names[i])->base);
    for(int i = 0; i < LEN(bounds); i++) {
        assert(getWorkspaceOfMonitor(getMonitorByName(names[i])));
        assertEquals(getWorkspaceOfMonitor(getMonitorByName(names[i]))->id, workspaces[i]);
    }
}


SCUTEST(test_restore_many_monitors, .iter = 2) {
    CRASH_ON_ERRORS = -1;
    Rect bounds = {0, 20, 100, 100};
    const char* name = "name";
    int N = 100;
    for(int i = 0; i < N; i++) {
        addFakeMonitorWithName(bounds, name);
    }
    saveResetState();
    if(_i)
        addFakeMonitorWithName((Rect) {0, 0, 1, 1}, name);
    loadCustomState();
    assertEquals(getAllMonitors()->size, N + 1);
    FOR_EACH(Monitor*, m, getAllMonitors()) {
        if(m->fake)
            assertEqualsRect(m->base, bounds);
    }
}
