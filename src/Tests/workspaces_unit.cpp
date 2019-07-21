#include "tester.h"
#include "test-mpx-helper.h"

#include "../user-events.h"
#include "../globals.h"
#include "../system.h"
#include "../logger.h"
#include "../workspaces.h"
#include "../windows.h"
#include "../monitors.h"
#include "../masters.h"

static void addFakeMonitor(MonitorID n) {
    getAllMonitors().add(new Monitor(n, {0, 0, 1, 1}));
}
SET_ENV(addDefaultMaster, simpleCleanup);
MPX_TEST("test_workspace_add_remove", {
    addWorkspaces(1);
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
});
MPX_TEST("test_workspace_add_remove_many", {
    int starting = getNumberOfWorkspaces();
    int size = 10;
    addWorkspaces(size);
    assert(getNumberOfWorkspaces() == starting + size);
    removeWorkspaces(size - 1);
    assert(getNumberOfWorkspaces() == starting + 1);
});
MPX_TEST("test_workspace_remove_last", {
    assert(getNumberOfWorkspaces() == 0);
    removeWorkspaces(1);
    assert(getNumberOfWorkspaces() == 0);
    addWorkspaces(1);
    removeWorkspaces(1);
    assert(getNumberOfWorkspaces() == 1);
});
MPX_TEST("workspace_default_name", {
    addWorkspaces(10);
    for(int i = 0; i < getNumberOfWorkspaces(); i++)
        assert(getWorkspace(i)->getName() != "");
});
MPX_TEST("getWorkspaceByName", {

    addWorkspaces(10);
    std::string str = "test";
    std::string str2 = "test3";
    getWorkspace(0)->setName(str);
    getWorkspace(2)->setName(str);
    getWorkspace(1)->setName(str2);
    assert(getWorkspace(0) == getWorkspaceByName(str));
    assert(getWorkspace(1) == getWorkspaceByName(str2));
    assert(getWorkspaceByName("UNSET_NAME") == NULL);
});

MPX_TEST("test_workspace_window_add", {
    getAllWindows().add(new WindowInfo(1));
    getAllWindows().add(new WindowInfo(2));
    addWorkspaces(2);
    getWorkspace(1)->getWindowStack().add(getAllWindows()[0]);
    getWorkspace(0)->getWindowStack().add(getAllWindows()[1]);
    removeWorkspaces(1);
});
MPX_TEST("test_monitor_add", {
    addWorkspaces(1);
    addFakeMonitor(1);
    Monitor* m = getAllMonitors()[0];
    Workspace* w = getWorkspace(0);
    assert(!w->isVisible());
    w->setMonitor(m);
    assert(w->isVisible());
    w->setMonitor(NULL);
    assert(!w->isVisible());
});

MPX_TEST("test_next_workspace_fail", {
    addWorkspaces(1);
    assert(getAllMonitors().size() == 0);
    assert(!getWorkspace(0)->getNextWorkspace(1, VISIBLE));
});
MPX_TEST("test_next_workspace", {
    addWorkspaces(4);
    for(int i = 0; i < 2; i++) {
        //will take the first n workspaces
        addFakeMonitor(i);
        getWorkspace(i)->setMonitor(getAllMonitors().find(i));
        assert(getWorkspace(i)->isVisible());
        addWindowInfo(new WindowInfo(i + 1));
        getWorkspace(i * 2)->getWindowStack().add(getWindowInfo(i + 1));
    }
    assert(getNumberOfWorkspaces() == 4);
    int target[][4] = {
        {1, 2, 3, 0}, //any -1,-1
        {1, 0, 1, 0}, //visible -1,0
        {2, 3, 2, 3}, //hidden  -1,1
        {2, 0, 2, 0}, //non empty 0,-1
        {0, 0, 0, 0}, //nonempty,visible 0,0
        {2, 2, 2, 2}, //hidden nonempty 0,1
        {1, 3, 1, 3}, //empty   1,-1
        {1, 1, 1, 1}, //visible,empty   1,0
        {3, 3, 3, 3}, //hidden empty 1,1
    };
    for(int e = -1, count = 0; e < 2; e++)
        for(int h = -1; h < 2; h++, count++) {
            Workspace* ref = getWorkspace(0);
            for(int n = 0; n < getNumberOfWorkspaces(); n++) {
                int mask = ((e + 1) << 2) + (h + 1);
                Workspace* w = ref->getNextWorkspace(1, mask);
                assert(w == ref->getNextWorkspace(1, mask));
                assert(w);
                assert(w->getID() == target[count][n]);
                ref = w;
                getActiveMaster()->setWorkspaceIndex(w->getID());
            }
        }
    for(int i = -getNumberOfWorkspaces() * 2; i <= getNumberOfWorkspaces() * 2; i++) {
        Workspace* w = getWorkspace(0)->getNextWorkspace(i, 0);
        assert(w->getID() == getAllWorkspaces().getNextIndex(0, i));
    }
});

MPX_TEST("test_mask_add_remove_toggle", {
    Workspace* workspace = new Workspace();
    assert(workspace->getMask() == 0);
    workspace->addMask(1);
    assert(workspace->getMask() == 1);
    workspace->addMask(6);
    assert(workspace->getMask() == 7);
    workspace->removeMask(12);
    assert(workspace->getMask() == 3);
    assert(!workspace->hasMask(6));
    workspace->toggleMask(6);
    assert(workspace->hasMask(6));
    assert(workspace->getMask() == 7);
    workspace->toggleMask(6);
    assert(!workspace->hasMask(6));
    assert(workspace->getMask() == 1);
    workspace->toggleMask(6);
    delete workspace;
});
static Layout* fakeLayouts[2] = {(Layout*)1, (Layout*)2};
MPX_TEST("test_layout_toggle", {
    addWorkspaces(1);
    getWorkspace(0)->getLayouts().add(fakeLayouts[0]);
    assert(getWorkspace(0)->toggleLayout(fakeLayouts[1]));
    assert(getWorkspace(0)->getActiveLayout() == fakeLayouts[1]);
    assert(getWorkspace(0)->toggleLayout(fakeLayouts[1]));
    assert(getWorkspace(0)->getActiveLayout() == fakeLayouts[0]);

    for(int i = 0; i < 2; i++) {
        getWorkspace(0)->toggleLayout(fakeLayouts[0]);
        assert(getWorkspace(0)->getActiveLayout() == fakeLayouts[0]);
    }
});

MPX_TEST_ITER("test_layout_cycle", 2, {
    addWorkspaces(1);
    getWorkspace(0)->cycleLayouts(_i ? 0 : 0);
    getWorkspace(0)->getLayouts().add(fakeLayouts[0]);
    getWorkspace(0)->getLayouts().add(fakeLayouts[1]);
    getWorkspace(0)->cycleLayouts(_i ? 3 : -1);
    assert(getWorkspace(0)->getActiveLayout() == fakeLayouts[1]);
    getWorkspace(0)->cycleLayouts(_i ? 1 : -3);
    assert(getWorkspace(0)->getActiveLayout() == fakeLayouts[0]);
});

/*
MPX_TEST("test_monitor_workspace",{
    addFakeMaster(1, 1);
    for(int i = 0; i < size + 2; i++)
        addFakeMonitor(i);
    while(1){
        int count = 0;
        for(int i = 0; i < getNumberOfWorkspaces(); i++){
            if(isWorkspaceVisible(i)){
                count++;
                Monitor* m = getMonitorFromWorkspace(getWorkspaceByIndex(i));
                assert(m);
                Workspace* w = getWorkspaceFromMonitor(m);
                assert(w);
                assert(w->id == i);
            }
        }
        assert(count == MIN(getNumberOfWorkspaces(), getSize(getAllMonitors())));
        if(getSize(getAllMonitors()) > 1)
            assert(removeMonitor(((Monitor*)getElement(getAllMonitors(), getSize(getAllMonitors()) / 2))->id));
        else break;
    }
});



*/
