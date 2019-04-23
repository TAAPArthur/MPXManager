#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>

#include "../UnitTests.h"
#include "../TestX11Helper.h"
#include "../../logger.h"
#include "../../globals.h"
#include "../../wmfunctions.h"
#include "../../functions.h"
#include "../../devices.h"
#include "../../Extensions/mpx.h"

static void splitMasterHack(int i){
    xcb_input_key_press_event_t* event = getLastEvent();
    event->sourceid = i;
}
START_TEST(test_split_master){
    POLL_COUNT = 10;
    POLL_INTERVAL = 10;
    START_MY_WM;
    int size = getSize(getAllMasters());
    int id = splitMaster();
    assert(id);
    Master* newMaster = getMasterById(id);
    assert(size + 1 == getSize(getAllMasters()));
    int numOfSlaves;
    ArrayList* slaves = getSlavesOfMasterByID(NULL, 0, &numOfSlaves);
    assert(numOfSlaves == 2);
    lock();
    int keyboardId = 0, mouseId = 0;
    void dummySlaveInput(SlaveDevice * device){
        int detail = 1, type = XCB_INPUT_BUTTON_PRESS;
        if(device->keyboard){
            type = XCB_INPUT_KEY_PRESS;
            detail = 100;
            keyboardId = device->id;
        }
        else mouseId = device->id;
        sendDeviceAction(device->id, detail, type);
    }
    FOR_EACH(SlaveDevice*, slave, slaves) dummySlaveInput(slave);
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
    FOR_EACH(SlaveDevice*, slave, slaves){
        if(slave->id != DEFAULT_KEYBOARD && slave->id != DEFAULT_POINTER)
            dummySlaveInput(slave);
    }
    flush();
    assert(!xcb_poll_for_event(dis));
    unlock();
    deleteList(slaves);
    free(slaves);
    stopMPX();
}
END_TEST
START_TEST(test_start_mpx_empty){
    //shouldn't crash
    startMPX();
    FILE* fp = fopen(MASTER_INFO_PATH, "w");
    assert(fp);
    fclose(fp);
    assert(loadMasterInfo());
    startMPX();
}
END_TEST
START_TEST(test_save_load_mpx_bad){
    assert(saveMasterInfo());
    assert(remove(MASTER_INFO_PATH) == 0);
    assert(!loadMasterInfo());
    assert(saveMasterInfo());
    assert(loadMasterInfo());
    assert(loadMasterInfo());
    MASTER_INFO_PATH = "";
    assert(!saveMasterInfo());
    assert(!loadMasterInfo());
}
END_TEST
START_TEST(test_save_load_mpx){
    START_MY_WM;
    createMasterDevice("test1");
    WAIT_UNTIL_TRUE(getSize(getAllMasters()) == 2);
    Master* master = getElement(getAllMasters(), !indexOf(getAllMasters(), getActiveMaster(), sizeof(int)));
    int masterFocusColor = 255;
    int activeMasterFocusColor = 256;
    int numOfSlaves;
    MasterID defaultMasterID = _i ? getActiveMaster()->id : getActiveMaster()->pointerId;
    MasterID newMasterID = _i ? master->id : master->pointerId;
    int idle;
    master->focusColor = masterFocusColor;
    getActiveMaster()->focusColor = activeMasterFocusColor;
    lock();
    ArrayList* slaves = getSlavesOfMasterByID(&defaultMasterID, 1, &numOfSlaves);
    assert(numOfSlaves == 1);
    attachSlaveToMaster(((SlaveDevice*)getHead(slaves))->id, newMasterID);
    deleteList(slaves);
    free(slaves);
    flush();
    idle = getIdleCount();
    unlock();
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    lock();
    slaves = getSlavesOfMaster(getActiveMaster());
    assert(getSize(slaves) == 1);
    deleteList(slaves);
    free(slaves);
    slaves = getSlavesOfMasterByID(&newMasterID, 1, &numOfSlaves);
    assert(numOfSlaves == 1);
    deleteList(slaves);
    free(slaves);
    saveMasterInfo();
    stopMPX();
    unlock();
    WAIT_UNTIL_TRUE(getSize(getAllMasters()) == 1);
    lock();
    slaves = getSlavesOfMaster(getActiveMaster());
    assert(getSize(slaves) == 2);
    deleteList(slaves);
    free(slaves);
    loadMasterInfo();
    startMPX();
    assert(getSize(getAllMasters()) == 2);
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
}
END_TEST

START_TEST(test_auto_mpx){
    addAutoMPXRules();
    lock();
    START_MY_WM;
    saveMasterInfo();
    loadMasterInfo();
    startMPX();
    createMasterDevice("test1");
    flush();
    int idle = getIdleCount();
    unlock();
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    lock();
    stopMPX();
    unlock();
}
END_TEST
START_TEST(test_restart){
    createMasterDevice("test1");
    createMasterDevice("test1");
    createMasterDevice("test1");
    initCurrentMasters();
    assert(getSize(getAllMasters()) > 1);
    restartMPX();
    assert(getSize(getAllMasters()) == 1);
}
END_TEST
static void mpxStartup(void){
    MASTER_INFO_PATH = "dummy.txt";
    onStartup();
}
Suite* mpxSuite(){
    Suite* s = suite_create("MPX");
    TCase* tc_core;
    tc_core = tcase_create("SplitMaster");
    tcase_add_checked_fixture(tc_core, mpxStartup, fullCleanup);
    tcase_add_test(tc_core, test_split_master);
    tcase_add_test(tc_core, test_start_mpx_empty);
    tcase_add_test(tc_core, test_restart);
    tcase_add_test(tc_core, test_save_load_mpx_bad);
    tcase_add_loop_test(tc_core, test_save_load_mpx, 0, 2);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Auto MPX");
    tcase_add_checked_fixture(tc_core, mpxStartup, fullCleanup);
    tcase_add_test(tc_core, test_auto_mpx);
    suite_add_tcase(s, tc_core);
    return s;
}

