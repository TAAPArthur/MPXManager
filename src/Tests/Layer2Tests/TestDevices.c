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
Binding input[]={
        {0,XK_a,.mask=XCB_INPUT_XI_EVENT_MASK_KEY_PRESS},
        {0,XK_a,.mask=XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE},
        {0,Button1,.mask=XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS},
        {0,Button1,.mask=XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE},
};
static void resetAllMasks(){
    ROOT_DEVICE_EVENT_MASKS=0;
    NON_ROOT_DEVICE_EVENT_MASKS=0;
    ROOT_EVENT_MASKS=0;
    NON_ROOT_EVENT_MASKS=0;
    KEYBOARD_MASKS=XCB_INPUT_XI_EVENT_MASK_KEY_PRESS|XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE;
    POINTER_MASKS=XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS|XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE;
}
static void setup(){
    resetAllMasks();
    createContextAndSimpleConnection();

    for(int i=0;i<LEN(input);i++){
        assert(input[i].noGrab==0);
        initBinding(&input[i]);
        addBinding(&input[i]);
    }

}

int getIntAndFree(Node*node){
    int value=getIntValue(node);
    deleteList(node);
    return value;
}


START_TEST(test_create_destroy_master){
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
    Node*keyboardSlaves=getSlavesOfMasterByID(&getActiveMaster()->id, 1, NULL);
    Node*mouseSlaves=getSlavesOfMasterByID(&getActiveMaster()->pointerId, 1, NULL);
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
    int slaveKeyboardId=getIntAndFree(getSlavesOfMasterByID(&getActiveMaster()->id, 1, 0));
    int slavePointerId=getIntAndFree(getSlavesOfMasterByID(&getActiveMaster()->pointerId, 1, 0));
    assert(slaveKeyboardId!=slavePointerId);
    Master*realMaster=getActiveMaster();
    int fakeId=10000;
    addFakeMaster(fakeId, fakeId);
    Master*fakeMaster=getValue(isInList(getAllMasters(), fakeId));
    for(int i=0;i<4;i++){
        setActiveMaster(fakeMaster);
        if(i==0)
            setActiveMasterByDeviceId(slaveKeyboardId);
        else if(i==1)
            setActiveMasterByDeviceId(masterKeyboard);
        else if(i==2)
            setActiveMasterByDeviceId(masterPointer);
        else if(i==3)
            setActiveMasterByDeviceId(slavePointerId);
        assert(realMaster==getActiveMaster());
    }
}END_TEST
START_TEST(test_set_client_pointer){
    int win=createNormalWindow();
    setClientPointerForWindow(win);
    assert(getActiveMasterKeyboardID()==getClientKeyboard(win));
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
    Node* slaveKeyboards=getSlavesOfMasterByID(&getActiveMaster()->id,1,&slaveKeyboardSize);
    Node* slaveKeyboards2=getSlavesOfMasterByID(&getActiveMaster()->id,1,NULL);
    assert(slaveKeyboards);
    assert(listsEqual(slaveKeyboards, slaveKeyboards2));
    assert(getSize(slaveKeyboards)==slaveKeyboardSize);

    int slavePointerSize;
    Node* slavePointers=getSlavesOfMasterByID(&getActiveMaster()->pointerId,1,&slavePointerSize);
    Node* slavePointers2=getSlavesOfMasterByID(&getActiveMaster()->pointerId,1,NULL);
    assert(listsEqual(slavePointers,slavePointers2));
    assert(getSize(slaveKeyboards)==slavePointerSize);
    assert(slavePointers);

    int size;
    Node*slaves=getSlavesOfMasterByID(&getActiveMaster()->id,2,&size);
    Node*slaves2=getSlavesOfMasterByID(&getActiveMaster()->id,2,NULL);
    assert(listsEqual(slaves, slaves2));
    assert(getSize(slaves)==size);
    assert(slaveKeyboardSize+slavePointerSize == getSize(slaves));

    //Xephry will create 2 non dummy slaves
    assert(slavePointerSize==1);
    assert(slaveKeyboardSize==1);

    Node*slavesOfMaster=getSlavesOfMaster(getActiveMaster());
    assert(listsEqual(slavesOfMaster, slaves));

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
    deleteList(slavesOfMaster);
}END_TEST

START_TEST(test_slave_swapping_same_device){
    removeMaster(getActiveMasterKeyboardID());
    addFakeMaster(1, 1);
    setActiveMaster(getMasterById(1));
    //will error due to no Xserver is doesn't check for equality first
    swapDeviceId(getActiveMaster(),getActiveMaster());
}END_TEST
START_TEST(test_slave_swapping){
    createMasterDevice("test");
    initCurrentMasters();
    assert(getSize(getAllMasters())==2);

    Master*master1=getValue(getAllMasters());
    Master*master2=getValue(getAllMasters()->next);

    int idKeyboard1=master1->id,idPointer1=master1->pointerId;
    int idKeyboard2=master2->id,idPointer2=master2->pointerId;
    Node*slaves1=getSlavesOfMasterByID(&master1->id, 2,NULL);
    Node*slaves2=getSlavesOfMasterByID(&master2->id,2,NULL);

    swapDeviceId(master1,master2);
    Node*slavesSwap1=getSlavesOfMasterByID(&master1->id, 2,NULL);
    Node*slavesSwap2=getSlavesOfMasterByID(&master2->id, 2,NULL);
    assert(master1->id==idKeyboard2 && master1->pointerId==idPointer2);
    assert(master2->id==idKeyboard1 && master2->pointerId==idPointer1);
    assert(listsEqual(slaves1,slavesSwap1));
    assert(listsEqual(slaves2,slavesSwap2));
    deleteList(slavesSwap1);
    deleteList(slavesSwap2);

    swapDeviceId(master1,master2);
    slavesSwap1=getSlavesOfMasterByID(&master1->id, 2,NULL);
    slavesSwap2=getSlavesOfMasterByID(&master2->id, 2,NULL);
    assert(master1->id==idKeyboard1 && master1->pointerId==idPointer1);
    assert(master2->id==idKeyboard2 && master2->pointerId==idPointer2);
    assert(listsEqual(slaves1,slavesSwap1));
    assert(listsEqual(slaves2,slavesSwap2));
    deleteList(slavesSwap1);
    deleteList(slavesSwap2);
    deleteList(slaves1);
    deleteList(slaves2);
}END_TEST


START_TEST(test_passive_grab_ungrab){
    assert(!xcb_poll_for_event(dis) && "test failure");
    triggerAllBindings(ALL_MASKS);
    assert(!xcb_poll_for_event(dis) && "test failure?");
    passiveGrab(root, ALL_MASKS);
    triggerAllBindings(ALL_MASKS);
    waitToReceiveInput(ALL_MASKS);
    consumeEvents();
    passiveUngrab(root);
    triggerAllBindings(ALL_MASKS);
    assert(!xcb_poll_for_event(dis));
}END_TEST

START_TEST(test_grab_detail){
    Node*n=getDeviceBindings();
    FOR_EACH_CIRCULAR(n,assert(!grabBinding(getValue(n))));

    if(!fork()){
        n=getDeviceBindings();
        openXDisplay();
        FOR_EACH_CIRCULAR(n,assert(grabBinding(getValue(n))));
        closeConnection();
        exit(0);
    }
    waitForCleanExit();


    // we only send events to the test events focused window
    catchError(xcb_set_input_focus_checked(dis, XCB_INPUT_FOCUS_PARENT, root, 0));
    triggerAllBindings(ALL_MASKS);
    waitToReceiveInput(ALL_MASKS);
}END_TEST
START_TEST(test_ungrab_detail){
    Node*n=getDeviceBindings();

    FOR_EACH_CIRCULAR(n,assert(!grabBinding(getValue(n))));
    n=getDeviceBindings();
    FOR_EACH_CIRCULAR(n,assert(!ungrabBinding(getValue(n))));
    xcb_flush(dis);
    XFlush(dpy);
    if(!fork()){
        n=getDeviceBindings();
        openXDisplay();
        FOR_EACH_CIRCULAR(n,assert(!grabBinding(getValue(n))));
        closeConnection();
        exit(0);
    }
    waitForCleanExit();
    triggerAllBindings(ALL_MASKS);
    assert(!xcb_poll_for_event(dis));
}END_TEST

START_TEST(test_active_grab){
    int masks=((int[]){KEYBOARD_MASKS,POINTER_MASKS,ALL_MASKS})[_i];
    int (*func)()=((int (*[])()){grabActiveKeyboard,grabActivePointer,grabAllMasterDevices})[_i];
    assert(!func());
    if(!fork()){
        openXDisplay();
        assert(func());
        closeConnection();
        exit(0);
    }
    waitForCleanExit();
    triggerAllBindings(masks);
    waitToReceiveInput(masks);
}END_TEST


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

}END_TEST



Suite *devicesSuite() {
    Suite*s = suite_create("Devices");

    TCase*tc_core =tcase_create("Master detection");
    tcase_add_checked_fixture(tc_core, createContextAndSimpleConnection, closeConnection);
    tcase_add_test(tc_core,test_create_destroy_master);
    tcase_add_test(tc_core,test_get_associated_device);
    tcase_add_test(tc_core,test_set_active_master_by_id);
    tcase_add_test(tc_core,test_set_client_pointer);
    suite_add_tcase(s, tc_core);

    tc_core =tcase_create("Slaves");
    tcase_add_checked_fixture(tc_core, createContextAndSimpleConnection, closeConnection);
    tcase_add_test(tc_core,test_detect_test_slaves);
    tcase_add_test(tc_core,test_get_slaves);
    tcase_add_test(tc_core,test_slave_swapping);
    tcase_add_test(tc_core,test_slave_swapping_same_device);
    suite_add_tcase(s, tc_core);


    tc_core = tcase_create("Grab");
    tcase_add_checked_fixture(tc_core, setup, destroyContextAndConnection);
    tcase_add_test(tc_core, test_passive_grab_ungrab);
    tcase_add_test(tc_core, test_grab_detail);
    tcase_add_test(tc_core, test_ungrab_detail);
    tcase_add_loop_test(tc_core, test_active_grab,0,3);
    tcase_add_loop_test(tc_core, test_ungrab_device,0,2);
    suite_add_tcase(s, tc_core);

    return s;
}
