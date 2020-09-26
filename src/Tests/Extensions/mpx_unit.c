#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>
#include <unistd.h>

#include "../../globals.h"
#include "../../wm-rules.h"
#include "../../wmfunctions.h"
#include "../../functions.h"
#include "../../devices.h"
#include "../../Extensions/mpx.h"
#include "../tester.h"
#include "../test-mpx-helper.h"
#include "../test-event-helper.h"
#include "../test-x-helper.h"
#include "../test-wm-helper.h"

#define DIRNAME  "/tmp"
#define BASENAME "._dummy_mpx_info.txt"
static const char* ABS_PATH = DIRNAME "/" BASENAME;



SCUTEST_SET_ENV(createXSimpleEnv, cleanupXServer);

SCUTEST(test_slave_swapping) {
    createMasterDevice("test");
    initCurrentMasters();
    assert(getAllMasters()->size == 2);
    assert(getAllSlaves()->size == 2);
    assert(getSlaves(getActiveMaster())->size >= 2);
    Master* master1 = getElement(getAllMasters(), 0);
    Master* master2 = getElement(getAllMasters(), 1);
    attachSlaveToMaster(getHead(getSlaves(master1)), master2);
    initCurrentMasters();
    Slave* slave1 = getHead(getSlaves(master1));
    Slave* slave2 = getHead(getSlaves(master2));
    assert(slave1 != slave2);
    swapXDevices(master1, master2);
    initCurrentMasters();
    assert(slave1 == getHead(getSlaves(master2)));
    assert(slave2 == getHead(getSlaves(master1)));
}

static void mpxStartup(void) {
    MASTER_INFO_PATH = ABS_PATH;
    remove(ABS_PATH);
    createXSimpleEnv();
}
SCUTEST_SET_ENV(mpxStartup, cleanupXServer);
SCUTEST(test_start_mpx_empty) {
    //shouldn't crash
    startMPX();
    FILE* fp = fopen(MASTER_INFO_PATH, "w");
    assert(fp);
    fclose(fp);
    assert(loadMPXMasterInfo());
    assert(loadMPXMasterInfo());
    startMPX();
}

SCUTEST(test_save_load_mpx_bad) {
    assert(!loadMPXMasterInfo());
    assert(saveMPXMasterInfo());
    assert(loadMPXMasterInfo());
    assert(loadMPXMasterInfo());
    MASTER_INFO_PATH = "";
    assert(!saveMPXMasterInfo());
    assert(!loadMPXMasterInfo());
}
const char* names[] = {"t1", "t2", "t3", "t4"};
int colors[LEN(names)] = {0, 0xFF, 0xFF00, 0xFF0000};
static void mpxResume() {
    DEFAULT_BORDER_COLOR = 0xABCDEF;
    setenv("HOME", DIRNAME, 1);
    MASTER_INFO_PATH = "~/" BASENAME;
    remove(ABS_PATH);
    if(!fork()) {
        createSimpleEnv();
        openXDisplay();
        for(int i = 0; i < LEN(names); i++)
            createMasterDevice(names[i]);
        initCurrentMasters();
        for(int i = 0; i < LEN(names); i++) {
            Master* m = getMasterByName(names[i]);
            assert(m);
            m->focusColor = colors[i];
        }
        attachSlaveToMaster(getElement(getAllSlaves(), 0), getMasterByName(names[0]));
        attachSlaveToMaster(getElement(getAllSlaves(), 1), getMasterByName(names[1]));
        initCurrentMasters();
        createMasterDevice("_unseen_master_");
        assert(saveMPXMasterInfo());
        initCurrentMasters();
        destroyAllNonDefaultMasters();
        flush();
        closeConnection();
        quit(0);
    }
    assertEquals(waitForChild(0), 0);
    onSimpleStartup();
    initCurrentMasters();
    assertEquals(1, getAllMasters()->size);
}
SCUTEST_SET_ENV(mpxResume, cleanupXServer);

SCUTEST(save_load) {
    startMPX();
    assertEquals(getAllMasters()->size, LEN(names) + 1);
    assertEquals(getMasterByID(DEFAULT_KEYBOARD)->focusColor, DEFAULT_BORDER_COLOR);
    for(int i = 0; i < LEN(names); i++) {
        Master* m = getMasterByName(names[i]);
        assert(m);
        assertEquals(m->focusColor, colors[i]);
    }
}

SCUTEST(save_load_slaves) {
    startMPX();
    assertEquals(getAllMasters()->size, LEN(names) + 1);
    assert(!getSlaves(getActiveMaster())->size);
    FOR_EACH(Slave*, slave, getAllSlaves()) {
        assert(getMasterForSlave(slave));
    }
}

SCUTEST(test_auto_resume) {
    addAutoMPXRules();
    createMasterDevice(names[0]);
    runEventLoop();
    assert(getSlaves(getMasterByName(names[0]))->size);
}

SCUTEST(test_split_master) {
    splitMaster();
    Master* child = getLastChildOfMaster(getActiveMaster());
    assert(child);
    FOR_EACH(Slave*, slave, getAllSlaves()) {
        getActiveMaster()->lastActiveSlave = slave->id;
        attachActiveSlaveToLastChildOfMaster();
    }
    initCurrentMasters();
    assertEquals(getAllSlaves()->size, getSlaves(child)->size);
}
static void splitMasterEnv() {
    CRASH_ON_ERRORS = -1;
    MASTER_INFO_PATH = ABS_PATH;
    remove(ABS_PATH);
    onSimpleStartup();
    splitMaster();
    Slave* slave = getHead(getAllSlaves());
    getActiveMaster()->lastActiveSlave = slave->id;
    attachActiveSlaveToLastChildOfMaster();
    initCurrentMasters();
    saveMPXMasterInfo();
    destroyAllNonDefaultMasters();
    runEventLoop();
    assertEquals(getAllMasters()->size, 1);
    loadMPXMasterInfo();
    runEventLoop();
    startMPX();
    assert(getLastChildOfMaster(getActiveMaster()));
}

SCUTEST_SET_ENV(splitMasterEnv, cleanupXServer);
SCUTEST(test_split_master_toggle_child) {
    Master* child = getLastChildOfMaster(getActiveMaster());
    assert(child);
    assertEquals(1, getSlaves(child)->size);
    toggleChildMaster();
    initCurrentMasters();
    assertEquals(0, getSlaves(child)->size);
    toggleChildMaster();
    initCurrentMasters();
    assertEquals(1, getSlaves(child)->size);
}
SCUTEST(test_split_master_toggle_parent) {
    Master* child = getLastChildOfMaster(getActiveMaster());
    setActiveMaster(child);
    assertEquals(1, getSlaves(child)->size);
    toggleParentMaster();
    initCurrentMasters();
    assertEquals(0, getSlaves(child)->size);
    toggleParentMaster();
    initCurrentMasters();
    assertEquals(1, getSlaves(child)->size);
}
