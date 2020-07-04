#include "../../Extensions/session.h"
#include "../../devices.h"
#include "../../ewmh.h"
#include "../../extra-rules.h"
#include "../../layouts.h"
#include "../../masters.h"
#include "../../state.h"
#include "../../wm-rules.h"
#include "../../wmfunctions.h"
#include "../test-event-helper.h"
#include "../test-mpx-helper.h"
#include "../test-x-helper.h"
#include "../tester.h"
static void loadCustomState() {
    loadSavedNonWindowState();
    loadSavedWindowState();
}

static const ArrayList<long>serializeState(uint8_t mask) {
    ArrayList<long> list;
    ArrayList<const ArrayList<WindowInfo*>*> listsOfStacks;
    if(mask % 16 == 0)
        mask = 15;
    if(mask & 1) {
        list.add(getAllMasters().size());
        list.add(getActiveMaster()->getID());
        list.add(getActiveMaster()->getWorkspaceIndex());
        for(Master* master : getAllMasters()) {
            list.add(master->getID());
            list.add(master->getWorkspaceIndex());
            listsOfStacks.add(&master->getWindowStack());
        }
    }
    if(mask & 2) {
        list.add(getNumberOfWorkspaces());
        for(Workspace* workspace : getAllWorkspaces()) {
            list.add(-1);
            list.add(workspace->getID());
            list.add(workspace->getMonitor() ? workspace->getMonitor()->getID() : 0);
            list.add(workspace->getLayoutOffset());
            list.add(std::hash<std::string> {}(workspace->getActiveLayout() ? workspace->getActiveLayout()->getName() : ""));
            listsOfStacks.add(&workspace->getWindowStack());
        }
    }
    if(mask & 4) {
        list.add(getAllMonitors().size());
        for(Monitor* monitor : getAllMonitors()) {
            list.add(monitor->getID());
            list.add(monitor->getBase());
            list.add(monitor->getViewport());
            list.add(monitor->getWorkspace() ? monitor->getWorkspace()->getID() : 0);
            list.add(std::hash<std::string> {}(monitor->getName()));
        }
    }
    if(mask & 8)
        for(const ArrayList<WindowInfo*>* stack : listsOfStacks) {
            list.add(0);
            for(WindowInfo* winInfo : *stack)
                list.add(winInfo->getID());
        }
    return list;
}


static void setup() {
    addEWMHRules();
    addResumeCustomStateRules();
    onSimpleStartup();
}

SET_ENV(setup, fullCleanup);
MPX_TEST_ITER("test_restore_state", 2 * 16, {
    bool explicitSave = _i & 16;
    int mask = _i;
    CRASH_ON_ERRORS = -1;
    AUTO_FOCUS_NEW_WINDOW_TIMEOUT = 0;
    saveCustomState();
    flush();
    loadCustomState();
    loadSavedMonitorWorkspaceMapping();
    createMasterDevice("test");
    createMasterDevice("test2");
    initCurrentMasters();
    MONITOR_DUPLICATION_POLICY = 0;
    addFakeMonitor({0, 20, 100, 100});
    addFakeMonitor({0, 40, 100, 100});
    detectMonitors();
    assignUnusedMonitorsToWorkspaces();
    swapMonitors(0, 2);
    swapMonitors(1, 4);
    for(int i = 0; i < 5; i++)
        createNormalWindow();
    scan(root);
    for(WindowInfo* winInfo : getAllWindows())
        getActiveMaster()->onWindowFocus(winInfo->getID());
    setActiveMaster(getAllMasters()[1]);
    getActiveMaster()->onWindowFocus(getAllWindows()[0]->getID());
    getActiveMaster()->onWindowFocus(getAllWindows()[1]->getID());
    getAllWindows()[1]->moveToWorkspace(!getActiveWorkspaceIndex());
    getAllWindows()[0]->moveToWorkspace(getActiveWorkspaceIndex());
    char longName[512];
    for(int i = 0; i < LEN(longName) - 1; i++)
        longName[i] = 'A';
    longName[LEN(longName) - 1] = 0;

    static Layout l = Layout(longName, NULL);
    registeredLayout(&l);
    setActiveLayout(&l);
    switchToWorkspace(1);
    setActiveLayout(NULL);
    getAllMonitors().sort();
    const auto& list = serializeState(mask);
    raiseWindow(getAllWindows()[3]);
    lowerWindow(getAllWindows()[4]);

    if(explicitSave) {
        addShutdownOnIdleRule();
        runEventLoop();
        const auto& list2 = serializeState(mask);
        assertEquals(list, list2);
    }
    else {
        saveCustomState();
        saveMonitorWorkspaceMapping();
    }
    flush();
    raiseWindow(getAllWindows()[0]);
    lowerWindow(getAllWindows()[1]);
    if(!fork()) {
        RUN_AS_WM = 0;
        STEAL_WM_SELECTION = 1;
        saveXSession();
        destroyAllLists();
        clearAllRules();
        // reapply onXConnect rules
        ewmh = NULL;
        setup();
        getAllMonitors().sort();
        const auto& list2 = serializeState(mask);
        for(int i = 0; i < list.size(); i++) {
            if(list[i] != list2[i])
                std::cout << i << " " << list[i] << " " << list2[i] << "\n";
        }
        assertEquals(list.size(), list2.size());
        assertEquals(list, list2);
        saveCustomState();
        saveMonitorWorkspaceMapping();
        const auto& list3 = serializeState(mask);
        assertEquals(list, list3);
        fullCleanup();
        quit(0);
    }
    assertEquals(waitForChild(0), 0);
});

MPX_TEST("test_restore_state_window_masks_change", {
    for(int i = 0; i < 32; i++)
        mapWindow(createNormalWindow());
    scan(root);
    ArrayList<WindowMask>oldWindowMasks;
    for(int i = 0; i < 8; i++) {
        getAllWindows()[i]->addMask((1 << i) & ~EXTERNAL_MASKS);
        oldWindowMasks.add(getAllWindows()[i]->getMask());
    }
    saveCustomState();
    getAllWindows().deleteElements();
    scan(root);
    loadCustomState();
    for(int i = 0; i < oldWindowMasks.size(); i++) {
        assertEquals(oldWindowMasks[i], getAllWindows()[i]->getMask());
    }
});
MPX_TEST("test_restore_state_clear_previous", {
    WindowID win = mapWindow(createNormalWindow());
    scan(root);
    WindowInfo* winInfo = getWindowInfo(win);
    winInfo->addMask(FLOATING_MASK);
    saveCustomState();
    winInfo->removeMask(FLOATING_MASK);
    // needs to clear the previously saved value
    saveCustomState();
    loadCustomState();
    assert(!winInfo->hasMask(FLOATING_MASK));
});
MPX_TEST("test_restore_state_monitor_change_fake", {
    CRASH_ON_ERRORS = -1;
    Rect bounds1 = {0, 20, 100, 100};
    Rect bounds2 = {0, 40, 100, 100};
    addFakeMonitor(bounds1);
    addFakeMonitor(bounds2);
    saveCustomState();
    getAllMonitors().deleteElements();
    assertEquals(getAllMonitors().size(), 0);
    addFakeMonitor({0, 0, 1, 1});
    loadCustomState();
    assertEquals(getAllMonitors().size(), 2);
    assertEquals(getAllMonitors()[0]->getBase(), bounds1);
    assertEquals(getAllMonitors()[1]->getBase(), bounds2);
});

MPX_TEST("test_restore_state_monitor_change", {
    CRASH_ON_ERRORS = -1;
    Rect bounds[] = {{0, 20, 100, 100}, {0, 40, 100, 100}};
    MonitorID ids[] = {generateID(), generateID()};
    WorkspaceID workspaceIs[] = {1, 3};
    getAllMonitors().deleteElements();
    for(int i = 0; i < LEN(ids); i++) {
        getAllMonitors().add(new Monitor(ids[i], bounds[i]));
        getAllMonitors()[i]->assignWorkspace(getWorkspace(workspaceIs[i]));
    }
    saveMonitorWorkspaceMapping();
    assertEquals(getAllMonitors().size(), 2);
    for(int i = 0; i < LEN(ids); i++)
        getAllMonitors().add(new Monitor(generateID(), bounds[i]));
    for(auto id : ids)
        assertAndDelete(getAllMonitors().removeElement(id));
    loadSavedMonitorWorkspaceMapping();
    assertEquals(getAllMonitors().size(), 2);
    for(int i = 0; i < LEN(ids); i++) {
        assertEquals(getAllMonitors()[i]->getBase(), bounds[i]);
        assertEquals(getAllMonitors()[i]->getWorkspace(), getWorkspace(workspaceIs[i]));
    }
});

MPX_TEST("dock_undock", {
    MONITOR_DUPLICATION_POLICY = CONTAINS | INTERSECTS;
    MONITOR_DUPLICATION_RESOLUTION = TAKE_SMALLER;
    Rect rects[] = {{0, 0, 10, 10}, {10, 0, 10, 10}, {20, 0, 10, 10}};
    std::string names[] = {"m1", "m2", "m3"};
    WorkspaceID workspaceIds[] = {1, 2, 0};
    startWM();
    waitUntilIdle();
    lock();
    for(int i = 0; i < LEN(rects); i++) {
        getWorkspace(workspaceIds[i])->setMonitor(addFakeMonitor(rects[i], names[i]));
    }
    unlock();
    for(int n = 0; n < 10; n++) {
        wakeupWM();
        waitUntilIdle();
        auto getExpectedName = [ = ](int i) {return names[i] + (n % 2 == 0 ? "" : "_" + std::to_string(n));};
        lock();
        getAllMonitors().deleteElements();
        applyEventRules(SCREEN_CHANGE);
        unlock();
        wakeupWM();
        waitUntilIdle();
        lock();
        assertEquals(getAllMonitors().size(), 1);
        for(int i = 0; i < LEN(rects); i++)
            addFakeMonitor(rects[i], getExpectedName(i));
        applyEventRules(SCREEN_CHANGE);
        unlock();
        wakeupWM();
        waitUntilIdle();
        assertEquals(getAllMonitors().size(), LEN(rects));
        for(int i = 0; i < LEN(rects); i++) {
            assert(getWorkspace(workspaceIds[i])->getMonitor());
            assertEquals(getWorkspace(workspaceIds[i])->getMonitor()->getName(), getExpectedName(i));
        }
    }
})
