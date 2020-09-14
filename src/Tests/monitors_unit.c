#include "../masters.h"
#include "../monitors.h"
#include "../mywm-structs.h"
#include "../windows.h"
#include "test-mpx-helper.h"
#include "tester.h"
#include <string.h>

static WindowInfo* winInfo;
const uint16_t rootDims[2] = {100, 100};
static void setupEnvWithDock(int i) {
    setRootDims(rootDims);
    addDefaultMaster();
    addWorkspaces(2);
    winInfo = addFakeWindowInfo(1);
    addMask(winInfo, MAPPED_MASK | MAPPABLE_MASK);
    winInfo->dock = 1;
    winInfo->dockProperties = (DockProperties) {i % 4, .thickness = 1};
}
SCUTEST_SET_ENV(setupEnvWithDock, simpleCleanup);
SCUTEST_ITER(test_avoid_docks_ignore, 5 * 4) {
    Rect base = {0, 0, getRootWidth(), getRootHeight()};
    switch(_i / 4) {
        case 0:
            addMask(winInfo, PRIMARY_MONITOR_MASK);
            break;
        case 1:
            moveToWorkspace(winInfo, 1);
            break;
        case 2:
            removeMask(winInfo, MAPPED_MASK | MAPPABLE_MASK);
            break;
        case 3:
            base.x += getRootWidth();
            break;
        case 4:
            base.y += getRootHeight();
            break;
    }
    Monitor* monitor = addFakeMonitor(base);
    assignWorkspace(monitor, getWorkspace(0));
    resizeAllMonitorsToAvoidDock(winInfo);
    assertEqualsRect(monitor->base, monitor->view);
}
SCUTEST_ITER(test_avoid_docks, 4) {
    Rect base = {0, 0, getRootWidth(), getRootHeight()};
    Monitor* monitor = addFakeMonitor(base);
    assignWorkspace(monitor, getWorkspace(0));
    resizeAllMonitorsToAvoidAllDocks();
    assert(getRootWidth() == getRootHeight());
    assertEquals((getRootWidth() - winInfo->dockProperties.thickness)*getRootWidth(), getArea(monitor->view));
}

SCUTEST_ITER(test_large_docks, 4) {
    winInfo->dockProperties.thickness = getRootWidth();
    Rect base = {0, 0, getRootWidth(), getRootHeight()};
    addFakeMonitor(base);
    addFakeMonitor((Rect) {0, 0, 1, 1});
    assignUnusedMonitorsToWorkspaces();
    resizeAllMonitorsToAvoidDock(winInfo);
    printSummary();
    FOR_EACH(Monitor*, monitor, getAllMonitors()) {
        assert(!isMonitorActive(monitor));
    }
}

SCUTEST_SET_ENV(addDefaultMaster, simpleCleanup);


static int masks[] = {TAKE_PRIMARY, TAKE_LARGER, TAKE_SMALLER, TAKE_PRIMARY, 0};
SCUTEST_ITER(test_monitor_dup_resolution, LEN(masks)) {
    MONITOR_DUPLICATION_POLICY = INTERSECTS | CONTAINS | SAME_DIMS;
    MONITOR_DUPLICATION_RESOLUTION = masks[_i];
    Monitor* smaller = addFakeMonitor((Rect) {1, 1, 1, 1});
    Monitor* larger = addFakeMonitor((Rect) {0, 0, 100, 100});
    Monitor* primary = _i % 2 == 0 ? larger : smaller;
    setPrimary(primary->id);
    removeDuplicateMonitors();
    if(_i == LEN(masks) - 1) {
        assert(getAllMonitors()->size == 2);
    }
    else {
        assert(getAllMonitors()->size == 1);
        assert(getElement(getAllMonitors(), 0) == (_i < LEN(masks) / 2 ? larger : smaller));
    }
}
SCUTEST(test_monitor_dup_remove_all) {
    MONITOR_DUPLICATION_POLICY = INTERSECTS | CONTAINS | SAME_DIMS;
    MONITOR_DUPLICATION_RESOLUTION = TAKE_LARGER;
    for(int i = 0; i < 10; i++) {
        addFakeMonitor((Rect) {1, 1, 1, 1});
    }
    removeDuplicateMonitors();
    assertEquals(getAllMonitors()->size, 1);
}

SCUTEST(test_monitor_dup_keep_all, .iter = 2) {
    MONITOR_DUPLICATION_POLICY = _i ? 0 : CONSIDER_ONLY_NONFAKES;
    MONITOR_DUPLICATION_RESOLUTION = TAKE_LARGER;
    int N = 10;
    for(int i = 0; i < N; i++) {
        addFakeMonitor((Rect) {1, 1, 1, 1});
    }
    removeDuplicateMonitors();
    assertEquals(getAllMonitors()->size, N);
}
SCUTEST(test_monitor_no_dup) {
    MONITOR_DUPLICATION_POLICY = INTERSECTS | CONTAINS | SAME_DIMS;
    MONITOR_DUPLICATION_RESOLUTION = TAKE_LARGER;
    addFakeMonitor((Rect) {1, 1, 1, 1});
    addFakeMonitor((Rect) {2, 2, 1, 1});
    removeDuplicateMonitors();
    assertEquals(getAllMonitors()->size, 2);
}
static int monitor_masks[] = {SAME_DIMS, CONTAINS, CONTAINS_PROPER, INTERSECTS};
SCUTEST_ITER(test_monitor_intersection, LEN(monitor_masks)) {
    MONITOR_DUPLICATION_RESOLUTION = TAKE_LARGER;
    MONITOR_DUPLICATION_POLICY = masks[_i];
    Monitor* winner = NULL;
    switch(MONITOR_DUPLICATION_POLICY) {
        case SAME_DIMS:
            winner = addDummyMonitor();
            addDummyMonitor();
            break;
        case CONTAINS:
            addFakeMonitor((Rect) {1, 1, 1, 1});
            winner = addFakeMonitor((Rect) {0, 0, 2, 2});
            break;
        case CONTAINS_PROPER:
            addFakeMonitor((Rect) {1, 1, 1, 1});
            winner = addFakeMonitor((Rect) {0, 0, 3, 3});
            break;
        case INTERSECTS:
            addFakeMonitor((Rect) {0, 0, 2, 2});
            winner = addFakeMonitor((Rect) {1, 1, 3, 3});
    }
    removeDuplicateMonitors();
    assertEquals(getAllMonitors()->size, 1);
    assertEquals(getElement(getAllMonitors(), 0), winner);
}

SCUTEST_ITER(assign_workspace, 2) {
    addWorkspaces(2);
    Monitor* m = addDummyMonitor();
    assignWorkspace(m, _i ? getWorkspace(0) : NULL);
    assert(getMonitor(getWorkspace(0)) == m);
}
SCUTEST(test_auto_assign_workspace) {
    addWorkspaces(2);
    for(int n = 1; n < getNumberOfWorkspaces() + 1; n++) {
        addDummyMonitor();
        assignUnusedMonitorsToWorkspaces();
        int count = 0;
        for(int i = 0; i < getNumberOfWorkspaces(); i++)
            count += isWorkspaceVisible(getWorkspace(i));
        assertEquals(count, n);
        for(int i = 0; i < n && i < getNumberOfWorkspaces(); i++)
            assert(isWorkspaceVisible(getWorkspace(i)));
        for(int i = n; i < getNumberOfWorkspaces(); i++)
            assert(!isWorkspaceVisible(getWorkspace(i)));
    }
}
SCUTEST(test_auto_assign_workspace_active_first) {
    addWorkspaces(3);
    setActiveWorkspaceIndex(2);
    setActiveMaster(addFakeMaster(4, 5));
    setActiveWorkspaceIndex(1);
    addDummyMonitor();
    assert(!isWorkspaceVisible(getActiveWorkspace()));
    assignUnusedMonitorsToWorkspaces();
    assert(isWorkspaceVisible(getActiveWorkspace()));
}

