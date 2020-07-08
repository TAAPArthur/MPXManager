#include <math.h>
#include <X11/keysym.h>

#include "test-mpx-helper.h"
#include "test-x-helper.h"
#include "test-event-helper.h"
#include "tester.h"

#include "../bindings.h"
#include "../devices.h"
#include "../globals.h"
#include "../logger.h"
#include "../test-functions.h"
#include "../window-properties.h"
#include "../xsession.h"


SET_ENV(createXSimpleEnv, cleanupXServer);

MPX_TEST("test_init_current_masters", {
    assertEquals(getAllMasters().size(), 1);
    initCurrentMasters();
    assertEquals(getAllMasters().size(), 1);
    initCurrentMasters();
    assertEquals(getAllMasters().size(), 1);

});
MPX_TEST_ITER("test_create_destroy_master", 2, {
    int size = _i ? MAX_NUMBER_OF_MASTER_DEVICES : 2;
    initCurrentMasters();
    int numSlaves = getAllSlaves().size();
    for(int i = getAllMasters().size(); i < size; i++)
        createMasterDevice("test");
    initCurrentMasters();
    assertEquals(getAllMasters().size(), size);
    destroyAllNonDefaultMasters();
    initCurrentMasters();
    assertEquals(getAllMasters().size(), 1);
    assertEquals(numSlaves, getAllSlaves().size());
});
MPX_TEST("test_create_excessive_masters", {
    CRASH_ON_ERRORS = 0;
    setLogLevel(LOG_LEVEL_NONE);
    for(int i = 0; i < MAX_NUMBER_OF_MASTER_DEVICES * 128; i++) {
        createMasterDevice("test");
        if(i % MAX_NUMBER_OF_MASTER_DEVICES == 0)
            initCurrentMasters();
    }
    initCurrentMasters();
    destroyAllNonDefaultMasters();
    initCurrentMasters();
    assertEquals(getAllMasters().size(), 1);
});
MPX_TEST("test_init_current_masters_after_reset", {
    int size = 8;
    for(int i = 0; i < size; i++)
        createMasterDevice("test");
    //re-running
    initCurrentMasters();
    assert(getAllMasters().size() == size + 1);
    destroyAllNonDefaultMasters();
    initCurrentMasters();
    assert(getAllMasters().size() == 1);
});
MPX_TEST("test_init_current_slaves", {
    initCurrentMasters();
    // there should be be 2 test devices for every pair of masters
    assert(getAllSlaves().size() >= 2);
    assert(getActiveMaster()->getSlaves().size() >= 2);
});

MPX_TEST("test_get_slaves", {
    int slaveCounter[2] = {0};
    initCurrentMasters();
    assert(getActiveMaster()->getSlaves().size() >= 2);
    for(Slave* slave : getActiveMaster()->getSlaves()) {
        slaveCounter[slave->isKeyboardDevice()]++;
        assert(!Slave::isTestDevice(slave->getName()));
    }
    assert(slaveCounter[0]);
    assert(slaveCounter[1]);
});
MPX_TEST_ITER("move_slaves", 2, {
    createMasterDevice("test");
    initCurrentMasters();
    int numSlaves = getAllSlaves().size();
    assert(getAllMasters().size() == 2);
    Master* m = _i == 0 ? getAllMasters()[1] : NULL;
    assert(m != getActiveMaster());
    for(Slave* slave : getActiveMaster()->getSlaves()) {
        attachSlaveToMaster(slave, m);
    }
    flush();
    initCurrentMasters();
    assertEquals(numSlaves, getAllSlaves().size());
    assertEquals(getActiveMaster()->getSlaves().size(), 0);
    if(m)
        assertEquals(m->getSlaves().size(), 2);
});

MPX_TEST("master_by_name", {
    assert(!getMasterByName("test"));
    createMasterDevice("test");
    initCurrentMasters();
    assert(getMasterByName("test"));
    destroyAllNonDefaultMasters();
});

MPX_TEST("get_active_focus", {
    assert(getActiveFocus());
    focusWindow(root);
    assertEquals(getActiveFocus(), root);
})


MPX_TEST("test_set_active_master_by_id", {
    createMasterDevice("test1");
    createMasterDevice("test2");
    initCurrentMasters();
    Master* active = getActiveMaster();
    for(Master* master : getAllMasters()) {
        ArrayList<MasterID> ids = ArrayList<MasterID>({master->getKeyboardID(), master->getPointerID()});
        for(Slave* slave : master->getSlaves())
            ids.add(slave->getID());
        for(MasterID id : ids) {
            setActiveMasterByDeviceID(id);
            assert(getActiveMaster() == master);
            setActiveMaster(active);
        }
    }
});

MPX_TEST("get_mouse_pos", {
    short result[2];
    movePointer(0, 0);
    flush();
    getMousePosition(getActiveMasterPointerID(), root, result);
    assert(result[0] == 0 && result[1] == 0);
    movePointer(100, 200);
    flush();
    consumeEvents();
    getMousePosition(getActiveMasterPointerID(), root, result);
    assert(result[0] == 100 && result[1] == 200);
});

MPX_TEST("test_default_client_pointer", {
    WindowID win = createNormalWindow();
    assert(getActiveMaster() == getClientMaster(win));
});
MPX_TEST_ITER("test_get_client_pointer", 2, {
    createMasterDevice("test");
    initCurrentMasters();
    Master* newMaster = getAllMasters()[1];
    WindowID win = createNormalWindow();
    setClientPointerForWindow(win, newMaster->getID());
    if(_i)
        getAllMasters().removeElement(newMaster);
    else
        assert(newMaster == getClientMaster(win));
    assert(newMaster->getPointerID() == getClientPointerForWindow(win));
    if(_i)
        delete newMaster;
});

#ifndef NO_XRANDR
static void createMonitorLessEnv() {
    createXSimpleEnv();
    getAllMonitors().deleteElements();
}
SET_ENV(createMonitorLessEnv, cleanupXServer);
MPX_TEST("test_detect_one_monitor", {
    detectMonitors();
    assertEquals(getAllMonitors().size(), 1);
});
MPX_TEST("test_preserve_fake_monitors", {
    MONITOR_DUPLICATION_POLICY = 0;
    addRootMonitor();
    detectMonitors();
    assertEquals(getAllMonitors().size(), 2);
});
MPX_TEST("test_primary_monitor", {
    for(int i = 0; i < 2; i++) {
        setPrimary(NULL);
        detectMonitors();
        assert(!getPrimaryMonitor());
        setPrimary(getAllMonitors()[0]);
        detectMonitors();
        assert(getPrimaryMonitor());
    }
});
MPX_TEST("test_sorted_monitors", {
    addFakeMonitor({2, 0, 100, 100});
    addFakeMonitor({0, 0, 100, 100});
    addFakeMonitor({1, 0, 100, 100});
    detectMonitors();
    delete getAllMonitors().remove(0);
    for(int i = 0; i < 3; i++)
        assertEquals(getAllMonitors()[i]->getBase().x, i);
});
MPX_TEST("test_detect_monitors", {
    close(2);
    MONITOR_DUPLICATION_POLICY = 0;
    waitForChild(spawn("xsane-xrandr --auto split-monitor W 3 &>/dev/null"));
    detectMonitors();
    for(Monitor* m : getAllMonitors())
        assert(m->getName() != "");
});
MPX_TEST("test_detect_removed_monitors", {
    close(2);
    MONITOR_DUPLICATION_POLICY = 0;
    waitForChild(spawn("xsane-xrandr add-monitor 0 0 10 10 &>/dev/null"));
    detectMonitors();
    assertEquals(getAllMonitors().size(), 2);
    waitForChild(spawn("xsane-xrandr clear"));
    detectMonitors();
    assertEquals(getAllMonitors().size(), 1);
});
MPX_TEST("test_monitor_add_remove", {
    addWorkspaces(2);
    Monitor* monitor = addFakeMonitor({0, 0, 100, 100});
    detectMonitors();
    Workspace* w = monitor->getWorkspace();
    delete getAllMonitors().removeElement(monitor);
    monitor = addFakeMonitor({0, 0, 100, 100});
    detectMonitors();
    assertEquals(w, monitor->getWorkspace());
});
MPX_TEST("test_create_monitor_persists", {
    MONITOR_DUPLICATION_POLICY = 0;
    detectMonitors();
    assert(getAllMonitors().size() == 1);
    Rect bounds = {1, 1, 100, 100};
    addFakeMonitor(bounds);
    addFakeMonitor(bounds);
    detectMonitors();
    assertEquals(getAllMonitors().size(), 3);
});
#endif
