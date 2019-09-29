#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>

#include "../../logger.h"
#include "../../globals.h"
#include "../../wm-rules.h"
#include "../../wmfunctions.h"
#include "../../functions.h"
#include "../../functions.h"
#include "../../devices.h"
#include "../../Extensions/mpx.h"
#include "../tester.h"
#include "../test-mpx-helper.h"
#include "../test-event-helper.h"
#include "../test-x-helper.h"

static void mpxStartup(void) {
    MASTER_INFO_PATH = "/tmp/dummy.txt";
    remove(MASTER_INFO_PATH.c_str());
    onStartup();
}
SET_ENV(mpxStartup, fullCleanup);
MPX_TEST("test_start_mpx_empty", {
    //shouldn't crash
    startMPX();
    FILE* fp = fopen(MASTER_INFO_PATH.c_str(), "w");
    assert(fp);
    fclose(fp);
    assert(loadMPXMasterInfo());
    assert(loadMPXMasterInfo());
    startMPX();
});

MPX_TEST("test_save_load_mpx_bad", {
    assert(!loadMPXMasterInfo());
    assert(saveMPXMasterInfo());
    assert(loadMPXMasterInfo());
    assert(loadMPXMasterInfo());
    MASTER_INFO_PATH = "";
    assert(!saveMPXMasterInfo());
    assert(!loadMPXMasterInfo());
});
std::string names[] = {"t1", "t2", "t3", "t4"};
int colors[LEN(names)] = {0, 0xFF, 0xFF00, 0xFF0000};
int defaultColor = 0xABCDEF;
static void mpxResume() {
    mpxStartup();
    saveXSession();
    if(!fork()) {
        openXDisplay();
        getActiveMaster()->setFocusColor(defaultColor);
        for(auto name : names)
            createMasterDevice(name);
        initCurrentMasters();
        for(int i = 0; i < LEN(names); i++) {
            Master* m = getMasterByName(names[i]);
            assert(m);
            m->setFocusColor(colors[i]);
        }
        attachSlaveToMaster(getAllSlaves()[1], getMasterByName(names[0]));
        attachSlaveToMaster(getAllSlaves()[0], getMasterByName(names[1]));
        initCurrentMasters();
        createMasterDevice("_unseen_master_");
        assert(saveMPXMasterInfo());
        initCurrentMasters();
        destroyAllNonDefaultMasters();
        flush();
        fullCleanup();
        quit(0);
    }
    assertEquals(waitForChild(0), 0);
}
SET_ENV(mpxResume, fullCleanup);

MPX_TEST("save_load", {
    startMPX();
    assertEquals(getAllMasters().size(), LEN(names) + 1);
    assertEquals(getMasterById(DEFAULT_KEYBOARD)->getFocusColor(), defaultColor);
    for(int i = 0; i < LEN(names); i++) {
        Master* m = getMasterByName(names[i]);
        assert(m);
        assertEquals(m->getFocusColor(), colors[i]);
    }
});

MPX_TEST("save_load_slaves", {
    startMPX();
    assertEquals(getAllMasters().size(), LEN(names) + 1);
    assert(getActiveMaster()->getSlaves().empty());
    assertEquals(getAllSlaves()[1]->getMaster(), getMasterByName(names[0]));
    assertEquals(getAllSlaves()[0]->getMaster(), getMasterByName(names[1]));

});
MPX_TEST("test_restart", {
    startMPX();
    createMasterDevice("test1");
    createMasterDevice("test1");
    createMasterDevice("test1");
    initCurrentMasters();
    assert(getAllMasters().size() > LEN(names) + 1 + 1);
    restartMPX();
    assertEquals(getAllMasters().size(), LEN(names) + 1);
});
/* TODO
static void splitMasterHack(int i){
    xcb_input_key_press_event_t* event = (xcb_input_key_press_event_t*)getLastEvent();
    event->sourceid = i;
}
MPX_TEST("test_split_master", {
    POLL_COUNT = 10;
    POLL_INTERVAL = 10;
    startWM();
    int size = getAllMasters().size();
    int id = splitMaster();
    assert(id);
    Master* newMaster = getMasterById(id);
    assert(size + 1 == getAllMasters().size());
    int numOfSlaves;
    ArrayList* slaves = getSlavesOfMasterByID(NULL, 0, &numOfSlaves);
    assert(numOfSlaves == 2);
    lock();
    int keyboardId = 0, mouseId = 0;
    void dummySlaveInput(Slave * device){
        int detail = 1, type = XCB_INPUT_BUTTON_PRESS;
        if(device->keyboard){
            type = XCB_INPUT_KEY_PRESS;
            detail = 100;
            keyboardId = device->getID();
        }
        else mouseId = device->getID();
        sendDeviceAction(device->getID(), detail, type);
    }
    for(Slave* slave : slaves) dummySlaveInput(slave);
    flush();
    assert(keyboardId);
    assert(mouseId);
    Rule hackKeyboard = CREATE_WILDCARD(BIND(splitMasterHack, keyboardId), .passThrough = ALWAYS_PASSTHROUGH);
    Rule hackMouse = CREATE_WILDCARD(BIND(splitMasterHack, mouseId), .passThrough = ALWAYS_PASSTHROUGH);
    prependToList(getEventRules(GENERIC_EVENT_OFFSET + XCB_INPUT_KEY_PRESS), &hackKeyboard);
    prependToList(getEventRules(GENERIC_EVENT_OFFSET + XCB_INPUT_BUTTON_PRESS), &hackMouse);
    int idle = getIdleCount();
    unlock();
    WAIT_UNTIL_TRUE(getIdleCount() > idle);
    ArrayList* newSlaves = getSlavesOfMaster(newMaster);
    assert(listsEqual(newSlaves, slaves));
    deleteList(newSlaves);
    free(newSlaves);
    endSplitMaster();
    consumeEvents();
    msleep(50);
    lock();
    for(Slave* slave : slaves){
        if(slave->getID() != DEFAULT_KEYBOARD && slave->getID() != DEFAULT_POINTER)
            dummySlaveInput(slave);
    }
    flush();
    assert(!xcb_poll_for_event(dis));
    unlock();
    deleteList(slaves);
    free(slaves);
    stopMPX();
}
        );
MPX_TEST_ITER("test_save_load_mpx", 2, {
    startWM();
    createMasterDevice("test1");
    WAIT_UNTIL_TRUE(getAllMasters().size() == 2);
    Master* master = getElement(getAllMasters(), !indexOf(getAllMasters(), getActiveMaster(), sizeof(int)));
    int masterFocusColor = 255;
    int activeMasterFocusColor = 256;
    int numOfSlaves;
    MasterID defaultMasterID = _i ? getActiveMaster()->getID() : getActiveMaster()->pointerId;
    MasterID newMasterID = _i ? master->getID() : master->pointerId;
    int idle;
    master->focusColor = masterFocusColor;
    getActiveMaster()->focusColor = activeMasterFocusColor;
    lock();
    ArrayList* slaves = getSlavesOfMasterByID(&defaultMasterID, 1, &numOfSlaves);
    assert(numOfSlaves == 1);
    attachSlaveToMaster(((Slave*)getHead(slaves))->getID(), newMasterID);
    deleteList(slaves);
    free(slaves);
    flush();
    idle = getIdleCount();
    unlock();
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    lock();
    slaves = getSlavesOfMaster(getActiveMaster());
    assert(size(slaves) == 1);
    deleteList(slaves);
    free(slaves);
    slaves = getSlavesOfMasterByID(&newMasterID, 1, &numOfSlaves);
    assert(numOfSlaves == 1);
    deleteList(slaves);
    free(slaves);
    saveMPXMasterInfo();
    stopMPX();
    unlock();
    WAIT_UNTIL_TRUE(getAllMasters().size() == 1);
    lock();
    slaves = getSlavesOfMaster(getActiveMaster());
    assert(size(slaves) == 2);
    deleteList(slaves);
    free(slaves);
    loadMPXMasterInfo();
    startMPX();
    assert(getAllMasters().size() == 2);
    slaves = getSlavesOfMasterByID(&defaultMasterID, 1, &numOfSlaves);
    deleteList(slaves);
    free(slaves);
    assert(numOfSlaves == 0);
    slaves = getSlavesOfMasterByID(&newMasterID, 1, &numOfSlaves);
    deleteList(slaves);
    free(slaves);
    assert(numOfSlaves == 1);
    master = getElement(getAllMasters(), !indexOf(getAllMasters(), getActiveMaster(), sizeof(int)));
    assert(master->focusColor == masterFocusColor);
    assert(getActiveMaster()->focusColor == activeMasterFocusColor);
    stopMPX();
    unlock();
});


*/
