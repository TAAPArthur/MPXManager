#include "../../layouts.h"
#include "../../ewmh.h"
#include "../../wmfunctions.h"
#include "../../devices.h"
#include "../../masters.h"
#include "../../wm-rules.h"
#include "../../Extensions/session.h"
#include "../tester.h"
#include "../test-mpx-helper.h"
#include "../test-x-helper.h"
#include "../test-event-helper.h"

static long rectToLong(const Rect& r) {
    return ((long)r.x) << 48L | ((long)r.y) << 32L | r.width << 16L | r.height;
}
static const ArrayList<long>serializeState(uint8_t mask) {
    ArrayList<long> list;
    ArrayList<const ArrayList<WindowInfo*>*> listsOfStacks;
    if(mask & 1) {
        list.add(getAllMasters().size());
        list.add(getActiveMaster()->getID());
        for(Master* master : getAllMasters()) {
            list.add(master->getID());
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
            list.add(rectToLong(monitor->getBase()));
            list.add(rectToLong(monitor->getViewport()));
            list.add(monitor->getWorkspace() ? monitor->getWorkspace()->getID() : 0);
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
    onSimpleStartup();
    addEWMHRules();
    addResumeCustomStateRules();
}

SET_ENV(setup, fullCleanup);
MPX_TEST_ITER("test_restore_state", 16, {
    int mask = _i;
    CRASH_ON_ERRORS = -1;
    saveCustomState();
    flush();
    loadCustomState();
    createMasterDevice("test");
    createMasterDevice("test2");
    initCurrentMasters();
    MONITOR_DUPLICATION_POLICY = 0;
    Monitor* ref = getActiveWorkspace()->getMonitor();
    addFakeMonitor({0, 20, 100, 100});
    addFakeMonitor(ref->getBase());
    detectMonitors();
    swapMonitors(0, 2);
    swapMonitors(1, 4);
    for(int i = 0; i < 32; i++)
        createNormalWindow();
    scan(root);
    for(WindowInfo* winInfo : getAllWindows())
        getActiveMaster()->onWindowFocus(winInfo->getID());
    setActiveMaster(getAllMasters()[1]);
    getActiveMaster()->onWindowFocus(getAllWindows()[0]->getID());
    getAllWindows()[1]->moveToWorkspace(!getActiveWorkspaceIndex());
    getAllWindows()[0]->moveToWorkspace(getActiveWorkspaceIndex());
    char longName[512];
    for(int i = 0; i < LEN(longName) - 1; i++)
        longName[i] = 'A';
    longName[LEN(longName) - 1] = 0;

    Layout l = Layout(longName, NULL);
    registeredLayout(&l);
    setActiveLayout(&l);
    switchToWorkspace(1);
    setActiveLayout(NULL);
    getAllMonitors().sort();
    const auto& list = serializeState(mask);
    saveCustomState();
    flush();
    if(!fork()) {
        saveXSession();
        RUN_AS_WM = 0;
        destroyAllLists();
        RUN_AS_WM = 0;
        // reapply onXConnect rules
        ewmh = NULL;
        onSimpleStartup();
        getAllMonitors().sort();
        const auto& list2 = serializeState(mask);
        for(int i = 0; i < list.size(); i++) {
            if(list[i] != list2[i])
                std::cout << i << " " << list[i] << " " << list2[i] << "\n";
        }
        assertEquals(list.size(), list2.size());
        assertEquals(list, list2);
        saveCustomState();
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
    for(int i = 0; i < 32; i++) {
        getAllWindows()[i]->addMask((1 << i) & ~EXTERNAL_MASKS);
        oldWindowMasks.add(getAllWindows()[i]->getMask());
    }
    saveCustomState();
    getAllWindows().deleteElements();
    scan(root);
    loadCustomState();
    for(int i = 0; i < 32; i++) {
        assertEquals(oldWindowMasks[i], getAllWindows()[i]->getMask());
    }
});
MPX_TEST("test_restore_state_monitor_change", {
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

MPX_TEST("replace_monitors", {
    WorkspaceID origin = 5;
    switchToWorkspace(origin);
    getAllMonitors()[0]->setName("S");
    Rect bounds1 = {0, 20, 100, 100};
    Rect bounds2 = {0, 40, 100, 100};
    addFakeMonitor(bounds1, "A");
    addFakeMonitor(bounds2, "B");
    for(Workspace* w : getAllWorkspaces())
        w->setMonitor(NULL);
    applyBatchEventRules();
    detectMonitors();
    for(Monitor* m : getAllMonitors()) {
        assert(m->getWorkspace());
    }
    assert(getWorkspace(origin)->isVisible());
    assert(getWorkspace(0)->isVisible());
    switchToWorkspace(0);
    applyBatchEventRules();
    getAllMonitors().deleteElements();
    detectMonitors();
    assertEquals(getAllMonitors().size(), 1);
    assert(getWorkspace(origin)->isVisible());
});

MPX_TEST("replace_monitors", {
    startWM();
    WorkspaceID other = 5;

    ATOMIC(switchToWorkspace(2));
    WindowID win = mapWindow(createNormalWindow());
    waitUntilIdle();

    lock();
    switchToWorkspace(other);
    getAllMonitors()[0]->setName("S");
    unlock();

    waitUntilIdle();

    lock();
    getAllMonitors().deleteElements();
    detectMonitors();
    unlock();

    waitUntilIdle(1);
    assert(isWindowMapped(win));
    // This is a side effect but explicitly intended
    assert(!getActiveWorkspace()->isVisible());
});
