#include "../../mywm-util.h"
#include "../../bindings.h"
#include "../../devices.h"
#include "../../defaults.h"
#include "../../xsession.h"
#include "../../logger.h"
#include "../UnitTests.h"
#include "../TestX11Helper.h"

#include <X11/keysym.h>
#include "../../test-functions.h"

void waitForCleanExit(){
    int status=0;
    wait(&status);
    if ( WIFEXITED(status) ) {
        const int es = WEXITSTATUS(status);
        assert(es==EXIT_SUCCESS);
    }
}
int getIntAndFree(Node*node){
    int value=getIntValue(node);
    deleteList(node);
    return value;
}
int listsEqual(Node*n,Node*n2){
    if(n==n2)
        return 1;
    if(getSize(n)!=getSize(n2))
        return 0;
    FOR_EACH(n,
        if(getIntValue(n)!=getIntValue(n2))
            return 0;
        n2=n2->next;
    )
    return 1;
}

static int checkDeviceEventMatchesType(xcb_input_key_press_event_t*event,int type){
    int result= event->event_type==type;
    free(event);
    return result;
}



START_TEST(test_create_remove_master){
    createMasterDevice("test");
    //re-running
    initCurrentMasters();
    assert(getSize(getAllMasters())==2);
    destroyMasterDevice(getIntValue(getAllMasters()), 2, 3);
    assert(getSize(getAllMasters())==2);
    initCurrentMasters();
    assert(getSize(getAllMasters())==2);
}END_TEST

START_TEST(test_get_associated_device){
    Node*keyboardSlaves=getSlavesOfMaster(&getActiveMaster()->id, 1, NULL);
    Node*mouseSlaves=getSlavesOfMaster(&getActiveMaster()->pointerId, 1, NULL);
    int slaveKeyboardId=getIntValue(keyboardSlaves);
    int slavePointerId=getIntValue(mouseSlaves);
    assert(getActiveMasterKeyboardID()==getAssociatedMasterDevice(slaveKeyboardId));
    assert(getActiveMasterPointerID()==getAssociatedMasterDevice(slavePointerId));
    assert(getActiveMasterKeyboardID()==getAssociatedMasterDevice(getActiveMasterPointerID()));
    assert(getActiveMasterPointerID()==getAssociatedMasterDevice(getActiveMasterKeyboardID()));
    deleteList(keyboardSlaves);
    deleteList(mouseSlaves);
}END_TEST

START_TEST(test_set_active_master_by_id){
    int masterKeyboard=getActiveMasterKeyboardID();
    int masterPointer=getActiveMasterPointerID();
    int slaveKeyboardId=getIntAndFree(getSlavesOfMaster(&getActiveMaster()->id, 1, 0));
    int slavePointerId=getIntAndFree(getSlavesOfMaster(&getActiveMaster()->pointerId, 1, 0));
    assert(slaveKeyboardId!=slavePointerId);
    Master*realMaster=getActiveMaster();
    int fakeId=10000;
    addMaster(fakeId, fakeId);
    Master*fakeMaster=getValue(isInList(getAllMasters(), fakeId));
    for(int i=0;i<4;i++){
        setActiveMaster(fakeMaster);
        if(i==0)
            assert(!setActiveMasterByKeyboardId(slaveKeyboardId));
        else if(i==1)
            assert(setActiveMasterByKeyboardId(masterKeyboard));
        else if(i==2)
            assert(setActiveMasterByMouseId(masterPointer));
        else if(i==3)
            assert(!setActiveMasterByMouseId(slavePointerId));
        assert(realMaster==getActiveMaster());
    }
}END_TEST



START_TEST(test_detect_test_slaves){
    char*testDevices[]={
        "XTEST pointer",
        "XTEST keyboard",
        "dummy XTEST pointer",
        "dummy XTEST keyboard",
    };
    char*nonTestDevices[]={
        "XTEST",
        "xtest keyboard",
        "xtest pointer",
        "XTEST pointer dummy",
        "TEST keyboard dummy",
    };
    for(int i=0;i<sizeof(testDevices)/sizeof(char*);i++)
        assert(isTestDevice(testDevices[i]));
    for(int i=0;i<sizeof(nonTestDevices)/sizeof(char*);i++)
        assert(!isTestDevice(nonTestDevices[i]));
}END_TEST
START_TEST(test_get_slaves){
    int slaveKeyboardSize=0;
    Node* slaveKeyboards=getSlavesOfMaster(&getActiveMaster()->id,1,&slaveKeyboardSize);
    Node* slaveKeyboards2=getSlavesOfMaster(&getActiveMaster()->id,1,NULL);
    assert(slaveKeyboards);
    assert(listsEqual(slaveKeyboards, slaveKeyboards2));
    assert(getSize(slaveKeyboards)==slaveKeyboardSize);

    int slavePointerSize;
    Node* slavePointers=getSlavesOfMaster(&getActiveMaster()->pointerId,1,&slavePointerSize);
    Node* slavePointers2=getSlavesOfMaster(&getActiveMaster()->pointerId,1,NULL);
    assert(listsEqual(slavePointers,slavePointers2));
    assert(getSize(slaveKeyboards)==slavePointerSize);
    assert(slavePointers);
    assert(SLAVE_POINTER|SLAVE_KEYBOARD==SLAVE_ANY);

    int size;
    Node*slaves=getSlavesOfMaster(&getActiveMaster()->id,2,&size);
    Node*slaves2=getSlavesOfMaster(&getActiveMaster()->id,2,NULL);
    assert(listsEqual(slaves, slaves2));
    assert(getSize(slaves)==size);
    assert(slaveKeyboardSize+slavePointerSize == getSize(slaves));

    Node* n=slaveKeyboards;
    FOR_EACH(n,assert(!isInList(slavePointers, getIntValue(n))));
    n=slavePointers;
    FOR_EACH(n,assert(!isInList(slaveKeyboards, getIntValue(n))));

    deleteList(slavePointers);
    deleteList(slavePointers2);
    deleteList(slaveKeyboards);
    deleteList(slaveKeyboards2);
    deleteList(slaves);
    deleteList(slaves2);
}END_TEST



START_TEST(test_slave_swapping_same_device){
    removeMaster(getActiveMasterKeyboardID());
    addMaster(1, 1);
    setActiveMaster(getMasterById(1));
    //will error due to no Xserver is doesn't check for equality first
    swapSlaves(getActiveMaster(),getActiveMaster());
}END_TEST
START_TEST(test_slave_swapping){
    createMasterDevice("test");
    initCurrentMasters();
    assert(getSize(getAllMasters())==2);

    Master*master1=getValue(getAllMasters());
    Master*master2=getValue(getAllMasters()->next);

    int idKeyboard1=master1->id,idPointer1=master1->pointerId;
    int idKeyboard2=master2->id,idPointer2=master2->pointerId;
    Node*slaves1=getSlavesOfMaster(&master1->id, 2,NULL);
    Node*slaves2=getSlavesOfMaster(&master2->id,2,NULL);

    swapSlaves(master1,master2);
    Node*slavesSwap1=getSlavesOfMaster(&master1->id, 2,NULL);
    Node*slavesSwap2=getSlavesOfMaster(&master2->id, 2,NULL);
    assert(master1->id==idKeyboard2 && master1->pointerId==idPointer2);
    assert(master2->id==idKeyboard1 && master2->pointerId==idPointer1);
    assert(listsEqual(slaves1,slavesSwap1));
    assert(listsEqual(slaves2,slavesSwap2));
    deleteList(slavesSwap1);
    deleteList(slavesSwap2);

    swapSlaves(master1,master2);
    slavesSwap1=getSlavesOfMaster(&master1->id, 2,NULL);
    slavesSwap2=getSlavesOfMaster(&master2->id, 2,NULL);
    assert(master1->id==idKeyboard1 && master1->pointerId==idPointer1);
    assert(master2->id==idKeyboard2 && master2->pointerId==idPointer2);
    assert(listsEqual(slaves1,slavesSwap1));
    assert(listsEqual(slaves2,slavesSwap2));
    deleteList(slavesSwap1);
    deleteList(slavesSwap2);
    deleteList(slaves1);
    deleteList(slaves2);

}END_TEST

START_TEST(test_grab_keyboard){
    assert(!grabActiveKeyboard());
    if(!fork()){
        openXDisplay();
        assert(grabActiveKeyboard());
        closeConnection();
        exit(0);
    }
    waitForCleanExit();
}END_TEST
START_TEST(test_grab_pointer){
    assert(!grabActivePointer());
    if(!fork()){
        openXDisplay();
        assert(grabActivePointer());
        closeConnection();
        exit(0);
    }
    waitForCleanExit();
}END_TEST

START_TEST(test_ungrab_device){
    assert(!grabActivePointer());
    assert(!grabActiveKeyboard());
    assert(!ungrabDevice(getActiveMasterKeyboardID()));
    assert(!ungrabDevice(getActiveMasterPointerID()));
    XFlush(dpy);
    xcb_flush(dis);
    if(!fork()){
        openXDisplay();
        assert(!grabActiveKeyboard());
        assert(!grabActivePointer());
        closeConnection();
        exit(0);
    }
    waitForCleanExit();


}END_TEST

START_TEST(test_passive_grab){
    int masks=XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS|XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE;
    passiveGrab(root, masks);
    sendButtonPress(1);
    WAIT_UNTIL_TRUE(checkDeviceEventMatchesType(getNextDeviceEvent(),XCB_INPUT_BUTTON_PRESS))
    sendButtonRelease(1);
    WAIT_UNTIL_TRUE(checkDeviceEventMatchesType(getNextDeviceEvent(),XCB_INPUT_BUTTON_RELEASE))

}END_TEST



START_TEST(test_register_for_device_events){
    ROOT_DEVICE_EVENT_MASKS=XI_KeyPressMask|XI_KeyReleaseMask|
                XI_ButtonPressMask|XI_ButtonReleaseMask;
    registerForDeviceEvents();
    for(int i=0;i<4;i++)
        assert(deviceBindings[i]->detail);
    sendButtonPress(1);
    free(getNextDeviceEvent());
}END_TEST
START_TEST(test_grab_detail){
    for(int i=0;i<4;i++)
        assert(!grabActiveDetail(&deviceBindings[i][1], i>=2));
    if(!fork()){
        openXDisplay();
        for(int i=0;i<4;i++)
            assert(grabActiveDetail(&deviceBindings[i][1], i>=2));
        closeConnection();
        exit(0);
    }
    //wait(NULL);
    waitForCleanExit();

}END_TEST

void resetAllMasks(){
    ROOT_DEVICE_EVENT_MASKS=0;
    NON_ROOT_DEVICE_EVENT_MASKS=0;
    ROOT_EVENT_MASKS=0;
    NON_ROOT_EVENT_MASKS=0;
}
static void setup(){
    resetAllMasks();
    createContext(1);
    connectToXserver();
}

static void teardown(){
    closeConnection();
    destroyContext();
}

Suite *devicesSuite() {
    Suite*s = suite_create("Devices");

    TCase*tc_core =tcase_create("Master detection");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core,test_create_remove_master);
    tcase_add_test(tc_core,test_get_associated_device);
    tcase_add_test(tc_core,test_set_active_master_by_id);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Grab");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core,test_register_for_device_events);
    tcase_add_test(tc_core,test_passive_grab);
    tcase_add_test(tc_core, test_grab_detail);
    tcase_add_test(tc_core, test_grab_pointer);
    tcase_add_test(tc_core, test_grab_keyboard);
    tcase_add_test(tc_core,test_ungrab_device);

    suite_add_tcase(s, tc_core);

    tc_core =tcase_create("Slaves");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core,test_detect_test_slaves);
    tcase_add_test(tc_core,test_get_slaves);
    tcase_add_test(tc_core,test_slave_swapping);
    tcase_add_test(tc_core,test_slave_swapping_same_device);
    suite_add_tcase(s, tc_core);

    return s;
}
