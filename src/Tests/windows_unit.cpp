
#include "../logger.h"
#include "../globals.h"
#include "../windows.h"

#include "tester.h"
#include "test-mpx-helper.h"

static void teardown() {
    suppressOutput();
    printSummary();
}

SET_ENV(createSimpleEnv, teardown);
MPX_TEST("win_int", {
    WindowInfo winInfo = WindowInfo(1);
    assert((int)winInfo == 1);
});
MPX_TEST("test_mask_add_remove_toggle", {
    WindowInfo* winInfo = new WindowInfo(1);
    assert(winInfo->getMask() == 0);
    winInfo->addMask(1);
    assert(winInfo->getMask() == 1);
    winInfo->addMask(6);
    assert(winInfo->getMask() == 7);
    winInfo->removeMask(12);
    assert(winInfo->getMask() == 3);
    assert(!winInfo->hasMask(6));
    winInfo->toggleMask(6);
    assert(winInfo->hasMask(6));
    assert(winInfo->getMask() == 7);
    winInfo->toggleMask(6);
    assert(!winInfo->hasMask(6));
    assert(winInfo->getMask() == 1);
    winInfo->toggleMask(6);
    delete winInfo;
});

MPX_TEST("test_mask_reset", {
    WindowInfo* winInfo = new WindowInfo(1);
    addWindowInfo(winInfo);
    winInfo->addMask(X_MAXIMIZED_MASK | Y_MAXIMIZED_MASK | ROOT_FULLSCREEN_MASK | FULLSCREEN_MASK);
    int nonUserMask = MAPPED_MASK | PARTIALLY_VISIBLE;
    winInfo->addMask(nonUserMask);
    winInfo->resetUserMask();
    assert(winInfo->getUserMask() == 0);
    assert(winInfo->getMask() == nonUserMask);
}
        );
MPX_TEST("test_window_workspace_masks", {
    WindowInfo* winInfo = new WindowInfo(1);
    getAllWindows().add(winInfo);
    getWorkspace(0)->getWindowStack().add(winInfo);
    assert(!winInfo->hasMask(HIDDEN_MASK));
    assert(!winInfo->hasMask(FLOATING_MASK));
    getWorkspace(0)->addMask(HIDDEN_MASK);
    assert(winInfo->hasMask(HIDDEN_MASK));
    getWorkspace(0)->addMask(FLOATING_MASK);
    assert(winInfo->hasMask(FLOATING_MASK));
    getWorkspace(0)->removeMask(FLOATING_MASK | HIDDEN_MASK);

    assert(!getWorkspace(0)->hasPartOfMask(FLOATING_MASK | HIDDEN_MASK));
    assert(!winInfo->hasPartOfMask(FLOATING_MASK | HIDDEN_MASK));
});
MPX_TEST("dock", {
    WindowInfo* winInfo = new WindowInfo(1);
    getAllWindows().add(winInfo);
    winInfo->setDock();
    assert(winInfo->isDock());
    assert(winInfo->getMask() == 0);
});
MPX_TEST("dock_properties", {
    WindowInfo* winInfo = new WindowInfo(1);
    getAllWindows().add(winInfo);
    auto* prop = winInfo->getDockProperties();
    assert(prop == NULL);
    int arr[12] = {0};
    winInfo->setDockProperties(arr, 12);
    prop = winInfo->getDockProperties();
    assert(prop != NULL);
    assert(memcmp(arr, prop, sizeof(arr)) == 0);
});
MPX_TEST("override_redirect", {
    WindowInfo* winInfo = new WindowInfo(1);
    getAllWindows().add(winInfo);
    winInfo->markAsOverrideRedirect();
    assert(winInfo->isOverrideRedirectWindow());
    assert(winInfo->hasMask(FLOATING_MASK | STICKY_MASK));
});
MPX_TEST("other", {
    WindowInfo* winInfo = new WindowInfo(1);
    getAllWindows().add(winInfo);
    for(int i = 0; i < 2; i++) {
        winInfo->setGroup(i);
        assert(winInfo->getGroup() == i);
        winInfo->setTransientFor(i % 2);
        assert(winInfo->getTransientFor() == i % 2);
    }
});
MPX_TEST("test_enable_tilingoverride", {
    WindowInfo* winInfo = new WindowInfo(1);
    getAllWindows().add(winInfo);
    int len = 5;
    for(int i = 0; i < len; i++) {
        assert(!winInfo->isTilingOverrideEnabledAtIndex(i));
        winInfo->setTilingOverrideEnabled(1 << i, 1);
        assert(winInfo->isTilingOverrideEnabledAtIndex(i));
        for(int n = 0; n < len; n++)
            if(n != i)
                assert(!winInfo->isTilingOverrideEnabledAtIndex(n));
        winInfo->setTilingOverrideEnabled(1 << i, 0);
    }
    for(int i = 0; i < len; i++) {
        assert(!winInfo->isTilingOverrideEnabledAtIndex(i));
        winInfo->setTilingOverrideEnabled(1 << i, 1);
        assert(winInfo->isTilingOverrideEnabledAtIndex(i));
    }
});
MPX_TEST("getWorkspace", {
    WindowInfo* winInfo = new WindowInfo(1);
    getAllWindows().add(winInfo);
    assert(winInfo->getWorkspaceIndex() == NO_WORKSPACE);
    getWorkspace(0)->getWindowStack().add(winInfo);
    assert(winInfo->getWorkspace() == getWorkspace(0));
    assert(winInfo->getWorkspaceIndex() == 0);
});
MPX_TEST("removeWorkspaces", {
    addWorkspaces(2);
    for(int i = 0; i < 10; i++)
        getAllWindows().add(new WindowInfo(i));
    for(WindowInfo* winInfo : getAllWindows())
        winInfo->moveToWorkspace(0);
    getWindowInfo(5)->removeFromWorkspace();
    assertEquals(getWorkspace(0)->getWindowStack()[5]->getID(), 6);
});
MPX_TEST("moveWorkspaces", {
    addWorkspaces(2);
    int i = 0;
    for(i = 0; i < 10; i++)
        getAllWindows().add(new WindowInfo(i));
    for(WindowInfo* winInfo : getAllWindows())
        winInfo->moveToWorkspace(0);
    i = 0;
    for(WindowInfo* winInfo : getWorkspace(0)->getWindowStack())
        assertEquals(winInfo->getID(), i++);
    for(WindowInfo* winInfo : getAllWindows())
        winInfo->moveToWorkspace(1);
    assert(getWorkspace(0)->getWindowStack().empty());
    for(WindowInfo* winInfo : getAllWindows())
        winInfo->moveToWorkspace(0);
    assert(getWorkspace(1)->getWindowStack().empty());
    getWorkspace(0)->getWindowStack()[1]->removeFromWorkspace();
    i = 0;
    for(WindowInfo* winInfo : getWorkspace(0)->getWindowStack()) {
        assertEquals(winInfo->getID(), i++);
        if(i == 1)i++;
    }
});
MPX_TEST("test_window_in_visible_worspace", {
    addRootMonitor();
    WindowInfo* winInfo = new WindowInfo(1);
    addWindowInfo(winInfo);
    assert(winInfo->isNotInInvisibleWorkspace());
    getWorkspace(0)->getWindowStack().add(winInfo);
    assert(!winInfo->isNotInInvisibleWorkspace());
    getWorkspace(0)->setMonitor(getAllMonitors()[0]);
    assert(winInfo->isNotInInvisibleWorkspace());
    getWorkspace(0)->setMonitor(NULL);
    assert(!winInfo->isNotInInvisibleWorkspace());
});
MPX_TEST("test_sticky_window_add", {
    WindowInfo* winInfo = new WindowInfo(1);
    getAllWindows().add(winInfo);
    winInfo->moveToWorkspace(-1);
    assert(winInfo->hasMask(STICKY_MASK));
});
MPX_TEST("matches", {
    WindowInfo* winInfo = new WindowInfo(1);
    std::string str[] = {"A", "B", "C", "D"};
    int i = 0;
    for(auto s : str) {
        assert(!winInfo->matches(s));
        if(i == 0)
            winInfo->setTitle(s);
        else if(i == 1)
            winInfo->setClassName(s);
        else if(i == 2)
            winInfo->setInstanceName(s);
        else if(i == 3)
            winInfo->setTypeName(s);
        assert(winInfo->matches(s));
    }
    delete winInfo;
});