#include "../layouts.h"
#include "../state.h"
#include "../window-properties.h"
#include "../wm-rules.h"
#include "../wmfunctions.h"
#include "test-event-helper.h"
#include "test-x-helper.h"
#include "tester.h"

static WindowInfo* addVisibleWindow(int i = getActiveWorkspaceIndex()) {
    WindowInfo* winInfo = new WindowInfo(createNormalWindow());
    addWindowInfo(winInfo);
    getWorkspace(i)->getWindowStack().add(winInfo);
    winInfo->addMask(MAPPABLE_MASK | MAPPED_MASK);
    return winInfo;
}
SET_ENV(createXSimpleEnv, fullCleanup);
MPX_TEST("init", {

    assert(!updateState());
});
MPX_TEST("print", {
    setLogLevel(LOG_LEVEL_VERBOSE);
    suppressOutput();

    addVisibleWindow(0);
    assert(updateState());

    assert(!updateState());

});
MPX_TEST("test_no_state_change", {

    assert(!updateState());
    assert(!updateState());

    assert(!updateState());
});

static void setup() {
    DEFAULT_NUMBER_OF_WORKSPACES = 4;
    onSimpleStartup();
    assertEquals(getAllMonitors().size(), 1);
    assertEquals(getNumberOfWorkspaces(), DEFAULT_NUMBER_OF_WORKSPACES);
    assert(getWorkspace(0)->isVisible());
    updateState();
    getEventRules(TILE_WORKSPACE).add(DEFAULT_EVENT_HIGH(incrementCount));
}
SET_ENV(setup, fullCleanup);
MPX_TEST("test_state_change_num_windows", {
    for(int i = getCount(); i < 10; i++) {
        addVisibleWindow(getActiveWorkspaceIndex());
        assertEquals(updateState(), 1);
        assertEquals(getCount(), i + 1);
    }
});
MPX_TEST("test_mask_change", {
    WindowInfo* winInfo = addVisibleWindow(getActiveWorkspaceIndex());
    winInfo->addMask(MAPPED_MASK);


    assertEquals(updateState(), 1);


    winInfo->addMask(FULLSCREEN_MASK);
    assertEquals(updateState(), 1);

    winInfo->addMask(FULLY_VISIBLE_MASK);

    assert(!updateState());
});
MPX_TEST("test_layout_change", {
    addVisibleWindow(getActiveWorkspaceIndex());
    updateState();
    Layout l = {"", NULL};
    getActiveWorkspace()->setActiveLayout(&l);

    assertEquals(updateState(), 1);
    getActiveWorkspace()->setActiveLayout(NULL);

    assertEquals(updateState(), 1);
});

MPX_TEST_ITER("test_num_workspaces_grow", 2, {
    int size = 10;
    addWorkspaces(size);
    updateState();

    int num = getNumberOfWorkspaces();
    assert(num > 2);
    if(_i)
        addWorkspaces(size);
    else removeWorkspaces(size / 2);
    assert(num != getNumberOfWorkspaces());
    //detect change only when growing
    assertEquals(updateState(), _i);
});

MPX_TEST("test_on_workspace_change", {
    int size = getNumberOfWorkspaces();
    assert(getNumberOfWorkspaces() > 2);
    for(int i = 0; i < size; i++) {
        registerWindow(mapArbitraryWindow(), root);
        getAllWindows().back()->moveToWorkspace(i);
    }
    for(int i = 0; i < size; i++)
        assert(!getWorkspace(i)->getWindowStack().empty());

    int count = getCount();
    // windows in invisible workspaces will be unmapped
    assertEquals(updateState(), 1);
    assertEquals(getCount(), ++count);
    startWM();
    waitUntilIdle();
    for(int i = 0; i < size; i++) {
        WindowInfo* winInfo = getWorkspace(i)->getWindowStack()[0];
        assert(winInfo);
        assert(winInfo->hasMask(MAPPABLE_MASK));
        bool mapped = winInfo->isNotInInvisibleWorkspace();
        assertEquals(mapped, isWindowMapped(winInfo->getID()));
        assertEquals(mapped, winInfo->hasMask(MAPPED_MASK));
    }

    assert(!updateState());
    for(int i = 1; i < size; i++) {
        switchToWorkspace(i);
        //workspace hasn't been tiled yet
        ATOMIC(assertEquals(updateState(), 1));
        waitUntilIdle();
    }
    for(int i = 0; i < size; i++) {
        switchToWorkspace(i);
        ATOMIC(assert(!updateState()));
        waitUntilIdle();
    }
});

MPX_TEST("test_on_invisible_workspace_window_add", {
    switchToWorkspace(0);
    for(int i = 0; i < 2; i++) {
        registerWindow(mapArbitraryWindow(), root);
        getAllWindows().back()->moveToWorkspace(i);
    }
    startWM();

    waitUntilIdle();
    lock();
    registerWindow(mapArbitraryWindow(), root);
    getAllWindows().back()->moveToWorkspace(1);
    unlock();
    waitUntilIdle();


    int mask = updateState();
    switchToWorkspace(1);

    mask |= updateState();
    assert(mask);
});

MPX_TEST("test_tile_limit", {
    setActiveLayout(TWO_PANE);
    assertEquals(getActiveLayout()->getArgs().limit, 2);
    addVisibleWindow()->getID();
    WindowID win2 = addVisibleWindow()->getID();

    assertEquals(updateState(), 1);
    WindowID win3 = addVisibleWindow()->getID();
    assertEquals(getNumberOfWindowsToTile(getActiveWorkspace()), 2);

    assertEquals(updateState(), 1);
    assertEquals(getRealGeometry(win3), getRealGeometry(win2));
});
