#include "../windows.h"

#include "test-mpx-helper.h"
#include "tester.h"

SCUTEST_SET_ENV(createSimpleEnv, simpleCleanup);
SCUTEST(test_mask_add_remove_toggle) {
    WindowInfo* winInfo = addFakeWindowInfo(1);
    assert(winInfo->mask == 0);
    addMask(winInfo, 1);
    assert(winInfo->mask == 1);
    addMask(winInfo, 6);
    assert(winInfo->mask == 7);
    removeMask(winInfo, 12);
    assert(winInfo->mask == 3);
    assert(!hasMask(winInfo, 6));
    toggleMask(winInfo, 6);
    assert(hasMask(winInfo, 6));
    assert(winInfo->mask == 7);
    toggleMask(winInfo, 6);
    assert(!hasMask(winInfo, 6));
    assert(winInfo->mask == 1);
    toggleMask(winInfo, 6);
}

SCUTEST(test_window_workspace_masks) {
    WindowInfo* winInfo = addFakeWindowInfo(1);
    moveToWorkspace(winInfo, 0);
    assert(!hasMask(winInfo, HIDDEN_MASK));
    assert(!hasMask(winInfo, FLOATING_MASK));
    addWorkspaceMask(getWorkspace(0), HIDDEN_MASK);
    assert(hasMask(winInfo, HIDDEN_MASK));
    addWorkspaceMask(getWorkspace(0), FLOATING_MASK);
    assert(hasMask(winInfo, FLOATING_MASK));
    removeWorkspaceMask(getWorkspace(0), FLOATING_MASK | HIDDEN_MASK);
    assert(!hasPartOfMask(winInfo, FLOATING_MASK | HIDDEN_MASK));
}
SCUTEST_ITER(dock_properties, 2) {
    WindowInfo* winInfo = addFakeWindowInfo(1);
    addMask(winInfo, DOCK_MASK);
    assert(!getDockProperties(winInfo));
    int arr[12] = {0};
    setDockProperties(winInfo, arr, 1);
    assert(!getDockProperties(winInfo));
    arr[1] = 1;
    assert(!getDockProperties(winInfo));
    setDockProperties(winInfo, arr, 1);
    assert(getDockProperties(winInfo));
}

SCUTEST(dock_properties_clear) {
    WindowInfo* winInfo = addFakeWindowInfo(1);
    int arr[12] = {1};
    setDockProperties(winInfo, arr, 1);
    assert(getDockProperties(winInfo));
    setDockProperties(winInfo, NULL, 0);
    assert(!getDockProperties(winInfo));
}
SCUTEST(test_enable_tilingoverride) {
    WindowInfo* winInfo = addFakeWindowInfo(1);
    int len = 5;
    for(int i = 0; i < len; i++) {
        assert(!isTilingOverrideEnabledAtIndex(winInfo, i));
        setTilingOverrideEnabled(winInfo, 1 << i);
        assert(isTilingOverrideEnabledAtIndex(winInfo, i));
        for(int n = 0; n < len; n++)
            if(n != i)
                assert(!isTilingOverrideEnabledAtIndex(winInfo, n));
        setTilingOverrideDisabled(winInfo, 1 << i);
    }
    for(int i = 0; i < len; i++) {
        assert(!isTilingOverrideEnabledAtIndex(winInfo, i));
        setTilingOverrideEnabled(winInfo, 1 << i);
        assert(isTilingOverrideEnabledAtIndex(winInfo, i));
    }
}
SCUTEST(moveWorkspaces) {
    addWorkspaces(2);
    int N = 10;
    for(int i = 1; i <= N; i++)
        addFakeWindowInfo(i);
    FOR_EACH(WindowInfo*, winInfo, getAllWindows()) {
        moveToWorkspace(winInfo, 0);
    }
    int i = 1;
    FOR_EACH(WindowInfo*, winInfo, getWorkspaceWindowStack(getWorkspace(0))) {
        assertEquals(winInfo->id, i++);
    }
    FOR_EACH(WindowInfo*, winInfo, getAllWindows()) {
        moveToWorkspace(winInfo, 1);
    }
    assert(!getWorkspaceWindowStack(getWorkspace(0))->size);
    assert(getWorkspaceWindowStack(getWorkspace(1))->size == N);
}
SCUTEST(test_window_in_visible_worspace) {
    Monitor* m = addDummyMonitor();
    WindowInfo* winInfo = addFakeWindowInfo(1);
    assert(isNotInInvisibleWorkspace(winInfo));
    moveToWorkspace(winInfo, 0);
    assert(!isNotInInvisibleWorkspace(winInfo));
    setMonitor(getWorkspace(0), m);
    assert(isNotInInvisibleWorkspace(winInfo));
    setMonitor(getWorkspace(0), NULL);
    assert(!isNotInInvisibleWorkspace(winInfo));
}
SCUTEST_ITER(test_sticky_window_add, 2) {
    WindowInfo* winInfo = addFakeWindowInfo(1);
    bool notInAnyWorkpace = _i;
    if(!notInAnyWorkpace)
        moveToWorkspace(winInfo, 0);
    moveToWorkspace(winInfo, -1);
    assert(hasMask(winInfo, STICKY_MASK));
    assertEquals(!getWorkspaceOfWindow(winInfo), notInAnyWorkpace);
}
