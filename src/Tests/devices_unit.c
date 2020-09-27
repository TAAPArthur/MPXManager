#include <math.h>
#include <X11/keysym.h>

#include "test-mpx-helper.h"
#include "test-x-helper.h"
#include "test-event-helper.h"
#include "tester.h"

#include "../bindings.h"
#include "../xutil/window-properties.h"
#include "../devices.h"
#include "../globals.h"


SCUTEST_SET_ENV(createXSimpleEnv, cleanupXServer);

SCUTEST(test_init_current_masters) {
    assertEquals(getAllMasters()->size, 1);
    initCurrentMasters();
    assertEquals(getAllMasters()->size, 1);
    initCurrentMasters();
    assertEquals(getAllMasters()->size, 1);
    createMasterDevice("test");
    initCurrentMasters();
    assertEquals(getAllMasters()->size, 2);
}
SCUTEST(test_create_excessive_masters) {
    CRASH_ON_ERRORS = 0;
    for(int i = 0; i < MAX_NUMBER_OF_MASTER_DEVICES * 10; i++) {
        createMasterDevice("test");
        if(i % MAX_NUMBER_OF_MASTER_DEVICES == 0)
            initCurrentMasters();
    }
    initCurrentMasters();
}
SCUTEST(test_init_current_slaves) {
    initCurrentMasters();
    // there should be be 2 test devices for every pair of masters
    assert(getAllSlaves()->size >= 2);
    assert(getSlaves(getActiveMaster())->size >= 2);
    int slaveCounter[2] = {0};
    initCurrentMasters();
    FOR_EACH(Slave*, slave, getSlaves(getActiveMaster())) {
        slaveCounter[slave->keyboard]++;
        assert(!isTestDevice(slave->name));
    }
    assert(slaveCounter[0]);
    assert(slaveCounter[1]);
}
SCUTEST_ITER(move_slaves, 2) {
    createMasterDevice("test");
    initCurrentMasters();
    int numSlaves = getAllSlaves()->size;
    assert(getAllMasters()->size == 2);
    Master* m = _i == 0 ? getElement(getAllMasters(), 1) : NULL;
    assert(m != getActiveMaster());
    FOR_EACH(Slave*, slave, getSlaves(getActiveMaster())) {
        attachSlaveToMaster(slave, m);
    }
    flush();
    initCurrentMasters();
    assertEquals(numSlaves, getAllSlaves()->size);
    assertEquals(getSlaves(getActiveMaster())->size, 0);
    if(m)
        assertEquals(getSlaves(m)->size, 2);
}

SCUTEST(master_by_name) {
    assert(!getMasterByName("test"));
    createMasterDevice("test");
    initCurrentMasters();
    assert(getMasterByName("test"));
    destroyAllNonDefaultMasters();
}

SCUTEST(get_active_focus) {
    assert(focusWindow(root));
    flush();
    assertEquals(getActiveFocus(), root);
}


SCUTEST(test_set_active_master_by_id) {
    createMasterDevice("test1");
    createMasterDevice("test2");
    initCurrentMasters();
    FOR_EACH(Master*, master, getAllMasters()) {
        assert(setActiveMasterByDeviceID(getKeyboardID(master)));
        assert(getActiveMaster() == master);
        assert(setActiveMasterByDeviceID(getPointerID(master)));
        FOR_EACH(Slave*, slave, getSlaves(getActiveMaster())) {
            assert(setActiveMasterByDeviceID(slave->id));
            assert(getActiveMaster() == master);
        }
    }
}

SCUTEST(get_mouse_pos) {
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
}

SCUTEST(test_default_client_pointer) {
    WindowID win = createNormalWindow();
    assert(getActiveMaster() == getClientMaster(win));
}
SCUTEST_ITER(test_get_client_pointer, 2) {
    createMasterDevice("test");
    initCurrentMasters();
    Master* newMaster = getElement(getAllMasters(), 1);
    WindowID win = createNormalWindow();
    MasterID pointerID = getPointerID(newMaster);
    setClientPointerForWindow(win, pointerID);
    assert(newMaster == getClientMaster(win));
    if(_i)
        freeMaster(newMaster);
    assertEquals(pointerID, getClientPointerForWindow(win));
}

#ifndef NO_XRANDR
static void createMonitorLessEnv() {
    openXDisplay();
    createSimpleEnv();
}
SCUTEST_SET_ENV(createMonitorLessEnv, cleanupXServer);
SCUTEST(test_detect_one_monitor) {
    detectMonitors();
    assertEquals(getAllMonitors()->size, 1);
}
SCUTEST(test_preserve_fake_monitors) {
    MONITOR_DUPLICATION_POLICY = 0;
    addRootMonitor();
    detectMonitors();
    assertEquals(getAllMonitors()->size, 2);
}
SCUTEST(test_primary_monitor) {
    for(int i = 0; i < 2; i++) {
        setPrimary(0);
        detectMonitors();
        assert(!getPrimaryMonitor());
        setPrimary(((Monitor*)getElement(getAllMonitors(), 0))->id);
        detectMonitors();
        assert(getPrimaryMonitor());
    }
}
SCUTEST(test_detect_removed_monitors) {
    MONITOR_DUPLICATION_POLICY = 0;
    assertEquals(0, spawnAndWait("xsane-xrandr add-monitor 0 0 10 10 &>/dev/null"));
    detectMonitors();
    assertEquals(getAllMonitors()->size, 2);
    assertEquals(0, spawnAndWait("xsane-xrandr clear"));
    detectMonitors();
    assertEquals(getAllMonitors()->size, 1);
}
SCUTEST(test_create_monitor_persists) {
    MONITOR_DUPLICATION_POLICY = 0;
    detectMonitors();
    assert(getAllMonitors()->size == 1);
    Rect bounds = {1, 1, 100, 100};
    addFakeMonitor(bounds);
    addFakeMonitor(bounds);
    detectMonitors();
    assertEquals(getAllMonitors()->size, 3);
}
#endif
