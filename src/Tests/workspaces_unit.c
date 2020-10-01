#include "tester.h"
#include "test-mpx-helper.h"

#include <string.h>
#include "../monitors.h"
#include "../workspaces.h"

static Layout* fakeLayouts[2] = {NULL, (void*)1};
SCUTEST_SET_ENV(addDefaultMaster, simpleCleanup);
SCUTEST(test_workspace_add_remove) {
    addWorkspaces(1);
    assert(getWorkspaceByName("1"));
    int starting = getNumberOfWorkspaces();
    for(int i = 1; i < 10; i++) {
        addWorkspaces(1);
        assert(getNumberOfWorkspaces() == starting + i);
    }
    starting = getNumberOfWorkspaces();
    for(int i = 1; i < 10; i++) {
        removeWorkspaces(1);
        assert(getNumberOfWorkspaces() == starting - i);
    }
}
SCUTEST(test_workspace_add_remove_many) {
    int starting = getNumberOfWorkspaces();
    int size = 10;
    addWorkspaces(size);
    assert(getNumberOfWorkspaces() == starting + size);
    removeWorkspaces(size - 1);
    assert(getNumberOfWorkspaces() == starting + 1);
}
SCUTEST(test_workspace_remove_last) {
    assert(getNumberOfWorkspaces() == 0);
    removeWorkspaces(1);
    assert(getNumberOfWorkspaces() == 0);
    addWorkspaces(1);
    removeWorkspaces(1);
    assert(getNumberOfWorkspaces() == 1);
}
SCUTEST(workspace_default_name) {
    addWorkspaces(10);
    for(int i = 0; i < getNumberOfWorkspaces(); i++)
        assert(strlen(getWorkspace(i)->name));
}

SCUTEST(test_workspace_window_add) {
    addFakeWindowInfo(1);
    addFakeWindowInfo(2);
    addWorkspaces(3);
    moveToWorkspace(getWindowInfo(1), 1);
    moveToWorkspace(getWindowInfo(2), 2);
    removeWorkspaces(2);
    assertEquals(getWorkspace(0), getWorkspaceOfWindow(getWindowInfo(1)));
    assertEquals(getWorkspace(0), getWorkspaceOfWindow(getWindowInfo(2)));
}

SCUTEST(test_has_window_with_workspace) {
    addWorkspaces(1);
    addFakeWindowInfo(1);
    moveToWorkspace(getWindowInfo(1), 0);
    assert(!hasWindowWithMask(getWorkspace(0), URGENT_MASK));
    addMask(getWindowInfo(1), URGENT_MASK);
    assert(hasMask(getWindowInfo(1), URGENT_MASK));
    assert(hasWindowWithMask(getWorkspace(0), URGENT_MASK));
}
SCUTEST(test_monitor_add) {
    addWorkspaces(1);
    Monitor* m = addDummyMonitor();
    Workspace* w = getWorkspace(0);
    assert(!isWorkspaceVisible(w));
    setMonitor(w, m);
    assert(isWorkspaceVisible(w));
    setMonitor(w, NULL);
    assert(!isWorkspaceVisible(w));
}

SCUTEST(test_layout_toggle) {
    addWorkspaces(1);
    addLayout(getWorkspace(0), fakeLayouts[0]);
    assert(toggleActiveLayout(fakeLayouts[1]));
    assert(getLayout(getWorkspace(0)) == fakeLayouts[1]);
    assert(toggleActiveLayout(fakeLayouts[1]));
    assert(getLayout(getWorkspace(0)) == fakeLayouts[0]);
    assert(!toggleActiveLayout(fakeLayouts[0]));
    assert(getLayout(getWorkspace(0)) == fakeLayouts[0]);
    cycleActiveLayouts(0);
    assert(getLayout(getWorkspace(0)) == fakeLayouts[0]);
    assert(!toggleActiveLayout(fakeLayouts[0]));
    assert(getLayout(getWorkspace(0)) == fakeLayouts[0]);
}

SCUTEST_ITER(test_layout_cycle, 2) {
    addWorkspaces(1);
    cycleActiveLayouts(_i ? 0 : 0);
    addLayout(getWorkspace(0), fakeLayouts[0]);
    addLayout(getWorkspace(0), fakeLayouts[1]);
    cycleActiveLayouts(_i ? 3 : -1);
    assert(getLayout(getWorkspace(0)) == fakeLayouts[1]);
    cycleActiveLayouts(_i ? 1 : -3);
    assert(getLayout(getWorkspace(0)) == fakeLayouts[0]);
}

SCUTEST_ITER(test_swapMonitors, 4) {
    addWorkspaces(2);
    for(int i = 1; i <= _i; i++)
        addDummyMonitor();
    assignUnusedMonitorsToWorkspaces();
    Monitor* monitor[2] = {getMonitor(getWorkspace(0)), getMonitor(getWorkspace(1))};
    bool visible[2] = {isWorkspaceVisible(getWorkspace(0)), isWorkspaceVisible(getWorkspace(1))};
    swapMonitors(0, 1);
    assert(monitor[1] == getMonitor(getWorkspace(0)));
    assert(monitor[0] == getMonitor(getWorkspace(1)));
    assert(visible[1] == isWorkspaceVisible(getWorkspace(0)));
    assert(visible[0] == isWorkspaceVisible(getWorkspace(1)));
}
