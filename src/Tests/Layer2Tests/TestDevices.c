#include <X11/keysym.h>

#include "../../mywm-util.h"
#include "../../devices.h"
#include "../../bindings.h"
#include "../../globals.h"
#include "../../xsession.h"
#include "../../logger.h"
#include "../UnitTests.h"
#include "../TestX11Helper.h"
#include "../../test-functions.h"


#define ALL_MASKS KEYBOARD_MASKS|POINTER_MASKS
//mod,detail,mask
Binding input[] = {
    {0, XK_a, .mask = XCB_INPUT_XI_EVENT_MASK_KEY_PRESS},
    {0, XK_a, .mask = XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE},
    {0, Button1, .mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS},
    {0, Button1, .mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE},
};
static void setup(){
    ROOT_DEVICE_EVENT_MASKS = 0;
    NON_ROOT_DEVICE_EVENT_MASKS = 0;
    ROOT_EVENT_MASKS = 0;
    NON_ROOT_EVENT_MASKS = 0;
    KEYBOARD_MASKS = XCB_INPUT_XI_EVENT_MASK_KEY_PRESS | XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE;
    POINTER_MASKS = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS | XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE;
    createContextAndSimpleConnection();
    for(int i = 0; i < LEN(input); i++){
        assert(input[i].noGrab == 0);
        initBinding(&input[i]);
        addBinding(&input[i]);
    }
    triggerAllBindings(ALL_MASKS);
    consumeEvents();
    assert(getSize(getDeviceBindings()) == LEN(input));
}

int getIntAndFree(ArrayList* node){
    int value = *(int*)getElement(node, 0);
    deleteList(node);
    free(node);
    return value;
}


START_TEST(test_create_destroy_master){
    createMasterDevice("test");
    //re-running
    initCurrentMasters();
    assert(getSize(getAllMasters()) == 2);
    destroyAllNonDefaultMasters();
    deleteList(getAllMasters());
    initCurrentMasters();
    assert(getSize(getAllMasters()) == 1);
}
END_TEST

START_TEST(test_get_associated_device){
    ArrayList* keyboardSlaves = getSlavesOfMasterByID(&getActiveMaster()->id, 1, NULL);
    ArrayList* mouseSlaves = getSlavesOfMasterByID(&getActiveMaster()->pointerId, 1, NULL);
    int slaveKeyboardId = ((SlaveDevice*)getElement(keyboardSlaves, 0))->id;
    int slavePointerId = ((SlaveDevice*)getElement(mouseSlaves, 0))->id;
    assert(getActiveMasterKeyboardID() == getAssociatedMasterDevice(slaveKeyboardId));
    assert(getActiveMasterPointerID() == getAssociatedMasterDevice(slavePointerId));
    assert(getActiveMasterKeyboardID() == getAssociatedMasterDevice(getActiveMasterPointerID()));
    assert(getActiveMasterPointerID() == getAssociatedMasterDevice(getActiveMasterKeyboardID()));
    deleteList(keyboardSlaves);
    deleteList(mouseSlaves);
    free(keyboardSlaves);
    free(mouseSlaves);
}
END_TEST

START_TEST(test_set_active_master_by_id){
    int masterKeyboard = getActiveMasterKeyboardID();
    int masterPointer = getActiveMasterPointerID();
    int slaveKeyboardId = getIntAndFree(getSlavesOfMasterByID(&getActiveMaster()->id, 1, 0));
    int slavePointerId = getIntAndFree(getSlavesOfMasterByID(&getActiveMaster()->pointerId, 1, 0));
    assert(slaveKeyboardId != slavePointerId);
    Master* realMaster = getActiveMaster();
    int fakeId = 10000;
    addFakeMaster(fakeId, fakeId);
    Master* fakeMaster = find(getAllMasters(), &fakeId, sizeof(int));
    for(int i = 0; i < 4; i++){
        setActiveMaster(fakeMaster);
        if(i == 0)
            setActiveMasterByDeviceId(slaveKeyboardId);
        else if(i == 1)
            setActiveMasterByDeviceId(masterKeyboard);
        else if(i == 2)
            setActiveMasterByDeviceId(masterPointer);
        else if(i == 3)
            setActiveMasterByDeviceId(slavePointerId);
        assert(realMaster == getActiveMaster());
    }
}
END_TEST
START_TEST(test_set_client_pointer){
    WindowID win = createNormalWindow();
    setClientPointerForWindow(win);
    assert(getActiveMasterKeyboardID() == getClientKeyboard(win));
}
END_TEST


START_TEST(test_detect_test_slaves){
    char* testDevices[] = {
        "XTEST pointer",
        "XTEST keyboard",
        "dummy XTEST pointer",
        "dummy XTEST keyboard",
    };
    char* nonTestDevices[] = {
        "XTEST",
        "xtest keyboard",
        "xtest pointer",
        "XTEST pointer dummy",
        "TEST keyboard dummy",
    };
    for(int i = 0; i < sizeof(testDevices) / sizeof(char*); i++)
        assert(isTestDevice(testDevices[i]));
    for(int i = 0; i < sizeof(nonTestDevices) / sizeof(char*); i++)
        assert(!isTestDevice(nonTestDevices[i]));
}
END_TEST
START_TEST(test_get_slaves){
    int slaveKeyboardSize = 0;
    ArrayList* slaveKeyboards = getSlavesOfMasterByID(&getActiveMaster()->id, 1, &slaveKeyboardSize);
    ArrayList* slaveKeyboards2 = getSlavesOfMasterByID(&getActiveMaster()->id, 1, NULL);
    assert(slaveKeyboards);
    assert(listsEqual(slaveKeyboards, slaveKeyboards2));
    assert(getSize(slaveKeyboards) == slaveKeyboardSize);
    int slavePointerSize;
    ArrayList* slavePointers = getSlavesOfMasterByID(&getActiveMaster()->pointerId, 1, &slavePointerSize);
    ArrayList* slavePointers2 = getSlavesOfMasterByID(&getActiveMaster()->pointerId, 1, NULL);
    assert(listsEqual(slavePointers, slavePointers2));
    assert(getSize(slaveKeyboards) == slavePointerSize);
    assert(slavePointers);
    int size;
    ArrayList* slaves = getSlavesOfMasterByID(&getActiveMaster()->id, 2, &size);
    ArrayList* slaves2 = getSlavesOfMasterByID(&getActiveMaster()->id, 2, NULL);
    assert(listsEqual(slaves, slaves2));
    assert(getSize(slaves) == size);
    assert(slaveKeyboardSize + slavePointerSize == getSize(slaves));
    //Xephry will create 2 non dummy slaves
    assert(slavePointerSize == 1);
    assert(slaveKeyboardSize == 1);
    ArrayList* slavesOfMaster = getSlavesOfMaster(getActiveMaster());
    assert(listsEqual(slavesOfMaster, slaves));
    FOR_EACH(SlaveDevice*, d, slaveKeyboards) assert(!find(slavePointers, d, sizeof(int)));
    FOR_EACH(SlaveDevice*, d, slavePointers) assert(!find(slaveKeyboards, d, sizeof(int)));
    deleteList(slavePointers);
    deleteList(slavePointers2);
    deleteList(slaveKeyboards);
    deleteList(slaveKeyboards2);
    deleteList(slaves);
    deleteList(slaves2);
    deleteList(slavesOfMaster);
    free(slavePointers);
    free(slavePointers2);
    free(slaveKeyboards);
    free(slaveKeyboards2);
    free(slaves);
    free(slaves2);
    free(slavesOfMaster);
}
END_TEST

START_TEST(test_slave_swapping_same_device){
    assert(removeMaster(getActiveMasterKeyboardID()));
    addFakeMaster(1, 2);
    setActiveMaster(getMasterById(1));
    //will error due to no Xserver is doesn't check for equality first
    swapDeviceId(getActiveMaster(), getActiveMaster());
}
END_TEST
START_TEST(test_slave_swapping){
    createMasterDevice("test");
    initCurrentMasters();
    assert(getSize(getAllMasters()) == 2);
    Master* master1 = getElement(getAllMasters(), 0);
    Master* master2 = getElement(getAllMasters(), 1);
    MasterID idKeyboard1 = master1->id, idPointer1 = master1->pointerId;
    MasterID idKeyboard2 = master2->id, idPointer2 = master2->pointerId;
    ArrayList* slaves1 = getSlavesOfMasterByID(&master1->id, 2, NULL);
    ArrayList* slaves2 = getSlavesOfMasterByID(&master2->id, 2, NULL);
    swapDeviceId(master1, master2);
    ArrayList* slavesSwap1 = getSlavesOfMasterByID(&master1->id, 2, NULL);
    ArrayList* slavesSwap2 = getSlavesOfMasterByID(&master2->id, 2, NULL);
    assert(master1->id == idKeyboard2 && master1->pointerId == idPointer2);
    assert(master2->id == idKeyboard1 && master2->pointerId == idPointer1);
    assert(listsEqual(slaves1, slavesSwap1));
    assert(listsEqual(slaves2, slavesSwap2));
    deleteList(slavesSwap1);
    deleteList(slavesSwap2);
    free(slavesSwap1);
    free(slavesSwap2);
    swapDeviceId(master1, master2);
    slavesSwap1 = getSlavesOfMasterByID(&master1->id, 2, NULL);
    slavesSwap2 = getSlavesOfMasterByID(&master2->id, 2, NULL);
    assert(master1->id == idKeyboard1 && master1->pointerId == idPointer1);
    assert(master2->id == idKeyboard2 && master2->pointerId == idPointer2);
    assert(listsEqual(slaves1, slavesSwap1));
    assert(listsEqual(slaves2, slavesSwap2));
    deleteList(slavesSwap1);
    deleteList(slavesSwap2);
    deleteList(slaves1);
    deleteList(slaves2);
    free(slavesSwap1);
    free(slavesSwap2);
    free(slaves1);
    free(slaves2);
}
END_TEST


START_TEST(test_passive_grab_ungrab){
    assert(!xcb_poll_for_event(dis) && "test failure");
    triggerAllBindings(ALL_MASKS);
    assert(!xcb_poll_for_event(dis) && "test failure?");
    passiveGrab(root, ALL_MASKS);
    triggerAllBindings(ALL_MASKS);
    waitToReceiveInput(ALL_MASKS, 0);
    consumeEvents();
    passiveUngrab(root);
    triggerAllBindings(ALL_MASKS);
    assert(!xcb_poll_for_event(dis));
}
END_TEST

START_TEST(test_grab_detail){
    FOR_EACH(Binding*, b, getDeviceBindings()) assert(!grabBinding(b));
    if(!fork()){
        openXDisplay();
        FOR_EACH(Binding*, b, getDeviceBindings()) assert(grabBinding(b));
        closeConnection();
        exit(0);
    }
    waitForCleanExit();
    // we only send events to the test events focused window
    catchError(xcb_set_input_focus_checked(dis, XCB_INPUT_FOCUS_PARENT, root, 0));
    triggerAllBindings(ALL_MASKS);
    waitToReceiveInput(ALL_MASKS, 0);
}
END_TEST
START_TEST(test_ungrab_detail){
    FOR_EACH(Binding*, b, getDeviceBindings()) assert(!grabBinding(b));
    FOR_EACH(Binding*, b, getDeviceBindings()) assert(!ungrabBinding(b));
    xcb_flush(dis);
    XFlush(dpy);
    if(!fork()){
        openXDisplay();
        FOR_EACH(Binding*, b, getDeviceBindings()) assert(!grabBinding(b));
        closeConnection();
        exit(0);
    }
    waitForCleanExit();
    triggerAllBindings(ALL_MASKS);
    assert(!xcb_poll_for_event(dis));
}
END_TEST

START_TEST(test_active_grab){
    int masks = ((int[]){
        KEYBOARD_MASKS, POINTER_MASKS, ALL_MASKS
    })[_i];
    int (*func)() = ((int (*[])()){
        grabActiveKeyboard, grabActivePointer, grabAllMasterDevices
    })[_i];
    assert(!func());
    if(!fork()){
        openXDisplay();
        assert(func());
        closeConnection();
        exit(0);
    }
    waitForCleanExit();
    triggerAllBindings(masks);
    waitToReceiveInput(masks, 0);
}
END_TEST


START_TEST(test_ungrab_device){
    if(_i){
        assert(!grabActivePointer());
        assert(!grabActiveKeyboard());
        assert(!ungrabDevice(getActiveMasterKeyboardID()));
        assert(!ungrabDevice(getActiveMasterPointerID()));
    }
    else {
        assert(!grabAllMasterDevices());
        assert(!ungrabAllMasterDevices());
    }
    XFlush(dpy);
    xcb_flush(dis);
    if(!fork()){
        openXDisplay();
        assert(!grabActiveKeyboard());
        assert(!grabActivePointer());
        assert(!grabAllMasterDevices());
        closeConnection();
        exit(0);
    }
    waitForCleanExit();
    triggerAllBindings(ALL_MASKS);
    assert(!xcb_poll_for_event(dis));
}
END_TEST

START_TEST(test_mouse_location){
    movePointer(getActiveMasterPointerID(), root, 1, 1);
    int arr[2];
    getMousePosition(getActiveMasterPointerID(), root, arr);
    /*TODO fix broken test
        for(int x=1;x<5;x++)
            for(int y=1;y<10;y+=2){
                movePointer(getActiveMasterPointerID(),root,x,y);
                flush();
                int arr[2];

                WAIT_UNTIL_TRUE(arr[0]==x&&arr[1]==y,flush();movePointer(getActiveMasterPointerID(),root,x,y);getMousePosition(getActiveMasterPointerID(),root,arr));
                printf("%d %d\n",x,y);
                assert(arr[0]==x);
                assert(arr[1]==y);
            }
    */
}
END_TEST

START_TEST(test_mouse_pos_tracking){
    Master* m = getActiveMaster();
    short result[2];
    getMouseDelta(m, result);
    assert(result[0] == 0 && result[1] == 0);
    setLastKnowMasterPosition(1, 2);
    assert(getLastKnownMasterPosition()[0] == 1 && getLastKnownMasterPosition()[1] == 2);
    getMouseDelta(m, result);
    assert(result[0] == 1 && result[1] == 2);
    setLastKnowMasterPosition(2, 1);
    getMouseDelta(m, result);
    assert(result[0] == 1 && result[1] == -1);
}
END_TEST

Suite* devicesSuite(){
    Suite* s = suite_create("Devices");
    TCase* tc_core;
    tc_core = tcase_create("Master detection");
    tcase_add_checked_fixture(tc_core, createContextAndSimpleConnection, destroyContextAndConnection);
    tcase_add_test(tc_core, test_create_destroy_master);
    tcase_add_test(tc_core, test_get_associated_device);
    tcase_add_test(tc_core, test_set_active_master_by_id);
    tcase_add_test(tc_core, test_set_client_pointer);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Slaves");
    tcase_add_checked_fixture(tc_core, createContextAndSimpleConnection, destroyContextAndConnection);
    tcase_add_test(tc_core, test_detect_test_slaves);
    tcase_add_test(tc_core, test_get_slaves);
    tcase_add_test(tc_core, test_slave_swapping);
    tcase_add_test(tc_core, test_slave_swapping_same_device);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Mouse");
    tcase_add_checked_fixture(tc_core, createContextAndSimpleConnection, destroyContextAndConnection);
    tcase_add_test(tc_core, test_mouse_location);
    tcase_add_test(tc_core, test_mouse_pos_tracking);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Grab");
    tcase_add_checked_fixture(tc_core, setup, destroyContextAndConnection);
    tcase_add_test(tc_core, test_passive_grab_ungrab);
    tcase_add_test(tc_core, test_grab_detail);
    tcase_add_test(tc_core, test_ungrab_detail);
    tcase_add_loop_test(tc_core, test_active_grab, 0, 3);
    tcase_add_loop_test(tc_core, test_ungrab_device, 0, 2);
    suite_add_tcase(s, tc_core);
    return s;
}
