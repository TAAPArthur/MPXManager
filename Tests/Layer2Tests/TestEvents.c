
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/randr.h>

#include "../../wmfunctions.h"
#include "../../defaults.h"
#include "../../devices.h"
#include "../../default-rules.h"
#include "../../events.h"
#include "../../mywm-structs.h"
#include "../../test-functions.h"

#include "../UnitTests.h"
#include "../TestX11Helper.h"
//#include "TestHelper.c"
extern char deviceBindingTriggerCount[4];


static int monitorRuleApplied=0;
static void onMonitorDetectionTest(){
    monitorRuleApplied++;
}

/**
 * Tests to see if events received actually make it to call back function
 * @param START_TEST(test_regular_events)
 */
START_TEST(test_regular_events){
    assert(LEN(eventRules)==NUMBER_OF_EVENT_RULES);
    int count=-1;
    int lastType=-1;

    void func(){
        lastType=((xcb_generic_event_t*)getLastEvent())->response_type & 127;
        count++;
    }
    //setLogLevel(0);
    IGNORE_SEND_EVENT=0;
    for(int i=count+1;i<XCB_GE_GENERIC;i++){
        if(i==1){
            count++;
            continue;
        }
        xcb_generic_event_t* event=calloc(32, 1);
        LOG(LOG_LEVEL_DEBUG,"sending event %d\n",i);
        event->response_type=i;

        Rules r=CREATE_DEFAULT_EVENT_RULE(func);
        insertHead(eventRules[i],&r);

        xcb_void_cookie_t cookie;
        if(i){
            if(i==XCB_CLIENT_MESSAGE){
                cookie=xcb_ewmh_send_client_message(dis, root, root, 1, 0, 0);
                //cookie=xcb_ewmh_send_client_message(dis, root, root, 0, 0, NULL);
                //cookie=xcb_ewmh_request_change_current_desktop_checked(ewmh, defaultScreenNumber, 0, 0);
            }
            else cookie=xcb_send_event_checked(dis, 0, root, ROOT_EVENT_MASKS,(char*) event);
            xcb_generic_error_t *error=xcb_request_check(dis,cookie);
            if(error){
                logError(error);
                assert(0);
            }
        }
        else{
            //generate error
            xcb_map_window(dis, -1);
        }

        xcb_flush(dis);
        WAIT_UNTIL_TRUE(count==i);
        assert(lastType==i);
        free(event);
        //assert(count==i);
    }

}END_TEST

void triggerAllBindings(int allBindings){
    for(int i=0;i<4;i++){
        int type=i+2;
        for(int n=0;n<=allBindings;n++){
            sendDeviceAction(i<2?getActiveMasterKeyboardID():getActiveMasterPointerID(),
                deviceBindings[i][n].detail,type);
            xcb_flush(dis);
        }

    }

   for(int i=0;i<4;i++){
       WAIT_UNTIL_TRUE(deviceBindingTriggerCount[i]>allBindings);
   }
}
START_TEST(test_binding_grab){
    triggerAllBindings(0);
}END_TEST
START_TEST(test_trigger_binding_active_grabbed){
    grabActiveKeyboard();
    grabActivePointer();
    triggerAllBindings(1);
}END_TEST
START_TEST(test_trigger_binding_passive){
    passiveGrab(root,
            XCB_INPUT_XI_EVENT_MASK_KEY_PRESS|XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE|
            XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS|XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE);
    triggerAllBindings(1);
}END_TEST
START_TEST(test_register_events){
    registerForEvents();
    xcb_get_window_attributes_reply_t *attr;
    attr=xcb_get_window_attributes_reply(dis,xcb_get_window_attributes(dis, root),NULL);
    assert(attr->your_event_mask==ROOT_EVENT_MASKS);
    free(attr);
}END_TEST

START_TEST(test_monitors){
    while(isNotEmpty(getAllMonitors()))
        removeMonitor(getIntValue(getAllMonitors()));
    //setLogLevel(0);
    xcb_randr_select_input(dis, root, XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
    if(!fork()){
        system("xrandr --output default --rotate left");
    }
    //checkError(xcb_randr_set_monitor_checked(dis, root, monitorInfo),0,"");
    xcb_flush(dis);
   WAIT_UNTIL_TRUE(monitorRuleApplied);
}END_TEST

Rules genericEventRule = CREATE_DEFAULT_EVENT_RULE(onGenericEvent);

Rules monitorEventRule = CREATE_DEFAULT_EVENT_RULE(onMonitorDetectionTest);
static void setup(){
    createContextAndConnection();
    registerForEvents();
    insertHead(eventRules[XCB_GE_GENERIC],&genericEventRule);
    insertHead(eventRules[MONITOR_EVENT_OFFSET+1],&monitorEventRule);
    addDefaultDeviceRules();
    START_MY_WM
}
static void teardown(){
    //wake up other thread
    xcb_map_window(dis, -1);
    xcb_flush(dis);
    close(xcb_get_file_descriptor(dis));
    pthread_join(pThread,((void *)0));
    xcb_disconnect(dis);
    destroyContext();
}
Suite *eventSuite(void) {
    Suite*s = suite_create("My Window Manager");
    TCase* tc_core = tcase_create("Events loop");
    tcase_add_checked_fixture(tc_core, setup, teardown);

    tcase_add_test(tc_core, test_regular_events);
    tcase_add_test(tc_core,test_binding_grab);
    tcase_add_test(tc_core,test_trigger_binding_active_grabbed);
    tcase_add_test(tc_core,test_trigger_binding_passive);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Events Registration");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_register_events);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Monitors");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core,test_monitors);
    suite_add_tcase(s, tc_core);

    return s;
}
