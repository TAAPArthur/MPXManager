#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI.h>
#include <X11/keysym.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>

#include "../../mywm-util.h"
#include "../../bindings.h"
#include "../../devices.h"
#include "../../defaults.h"

#include "../UnitTests.h"
#include "../../logger.h"
void connectToXserver(){
    LOG(LOG_LEVEL_DEBUG," connecting to XServe \n");
    dpy = XOpenDisplay(NULL);
    while (!dpy){
        msleep(100);
        dpy = XOpenDisplay(NULL);
    }
    dis = XGetXCBConnection(dpy);
    if(xcb_connection_has_error(dis)){
        LOG(LOG_LEVEL_ERROR," failed to conver xlib display to xcb version\n");
        exit(1);
    }
    XSetEventQueueOwner(dpy, XCBOwnsEventQueue);

    screen=xcb_setup_roots_iterator (xcb_get_setup (dis)).data;

    root = DefaultRootWindow(dpy);
}
START_TEST(test_create_remove_master){
    connectToXserver();
    createContext(1);
    createMasterDevice("test");
    initCurrentMasters();
    assert(getSize(getAllMasters())==2);
    destroyMasterDevice(getIntValue(getAllMasters()), 2, 3);
    destroyContext();
    createContext(1);
    initCurrentMasters();
    assert(getSize(getAllMasters())==1);

}END_TEST
START_TEST(test_get_slaves){
    createContext(1);
    connectToXserver();
    initCurrentMasters();
    int slaveKeyboards=getSize(getSlavesOfMaster(getActiveMaster(),1,0));
    int slavePointers=getSize(getSlavesOfMaster(getActiveMaster(),0,1));
    assert(slaveKeyboards);
    assert(slavePointers);
    assert(slaveKeyboards+slavePointers ==
            getSize(getSlavesOfMaster(getActiveMaster(),1,1)));

    assert(getIntValue(getSlavesOfMaster(getActiveMaster(),1,0)) !=
            getIntValue(getSlavesOfMaster(getActiveMaster(),0,1)));
}END_TEST

START_TEST(test_get_associated_device){
    createContext(1);
    connectToXserver();
    initCurrentMasters();
    int slaveKeyboardId=getIntValue(getSlavesOfMaster(getActiveMaster(), 1, 0));
    int slavePointerId=getIntValue(getSlavesOfMaster(getActiveMaster(), 0, 1));
    assert(getActiveMasterKeyboardID()==getAssociatedMasterDevice(slaveKeyboardId));
    assert(getActiveMasterPointerID()==getAssociatedMasterDevice(slavePointerId));
    assert(getActiveMasterKeyboardID()==getAssociatedMasterDevice(getActiveMasterPointerID()));
    assert(getActiveMasterPointerID()==getAssociatedMasterDevice(getActiveMasterKeyboardID()));

}END_TEST
START_TEST(test_set_active_master_by_id){
    createContext(1);
    connectToXserver();
    initCurrentMasters();
    int masterKeyboard=getActiveMasterKeyboardID();
    int masterPointer=getActiveMasterPointerID();
    int slaveKeyboardId=getIntValue(getSlavesOfMaster(getActiveMaster(), 1, 0));
    int slavePointerId=getIntValue(getSlavesOfMaster(getActiveMaster(), 0, 1));
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
START_TEST(test_grab_keyboard){
    createContext(1);
    connectToXserver();
    initCurrentMasters();
    grabActiveKeyboard();
    connectToXserver();
    assert(grabActiveKeyboard());
}END_TEST
START_TEST(test_grab_pointer){
    createContext(1);
    connectToXserver();
    initCurrentMasters();
    grabActivePointer();
    connectToXserver();
    assert(grabActivePointer());
}END_TEST
START_TEST(test_grab){
    char target[]={1,1,1,1};
    char count[]={0,0,0,0};
    void KP(){count[0]++;}
    void KR(){count[1]++;}
    void MP(){count[2]++;}
    void MR(){count[3]++;}
    createContext(1);
    Binding k={0,XK_A,BIND_TO_FUNC(KP)};
    Binding k2={0,XK_B,BIND_TO_FUNC(KR)};
    Binding m={0,Button1,BIND_TO_FUNC(MP)};
    Binding m2={0,Button2,BIND_TO_FUNC(MR)};
    deviceBindingLengths[0]=1; deviceBindings[0]=&k;
    deviceBindingLengths[1]=1; deviceBindings[1]=&k2;
    deviceBindingLengths[2]=1; deviceBindings[2]=&m;
    deviceBindingLengths[3]=1; deviceBindings[3]=&m2;
    DEVICE_EVENT_MASKS|=XI_KeyPressMask|XI_KeyReleaseMask|XI_ButtonPressMask|XI_ButtonReleaseMask;
    connectToXserver();
    registerForEvents();
    assert(k.keyCode);
    assert(k2.keyCode);
    connectToXserver();

    for(int i=0;i<4;i++)
        if(deviceBindings[i])
            assert(grabDetails(deviceBindings[i], deviceBindingLengths[i], bindingMasks[i], i>=2));

}END_TEST

Suite *devicesSuite() {
    Suite*s = suite_create("Context");

    TCase*tc_core = tc_core = tcase_create("Master detection");

    tcase_add_test(tc_core,test_create_remove_master);
    tcase_add_test(tc_core,test_get_slaves);
    tcase_add_test(tc_core,test_get_associated_device);
    tcase_add_test(tc_core,test_set_active_master_by_id);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Bindings");
    tcase_add_test(tc_core, test_grab);
    tcase_add_test(tc_core, test_grab_pointer);
    tcase_add_test(tc_core, test_grab_keyboard);


    suite_add_tcase(s, tc_core);

    return s;
}
