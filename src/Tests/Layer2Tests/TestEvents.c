
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/randr.h>

#include "../../bindings.h"
#include "../../devices.h"
#include "../../default-rules.h"
#include "../../events.h"
#include "../../globals.h"

#include "../../util.h"

#include "../UnitTests.h"
#include "../TestX11Helper.h"

char deviceBindingTriggerCount[] = {0, 0, 0, 0};

static Binding BINDINGS[] = {
    {0, XK_A, .mask = XCB_INPUT_XI_EVENT_MASK_KEY_PRESS},
    {0, XK_A, .mask = XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE},
    {0, Button2, .mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS},
    {0, Button2, .mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE},
};
static void bindingSetup(){
    createContextAndSimpleConnection();
    for(int i = 0; i < LEN(BINDINGS); i++)
        addBinding(&BINDINGS[i]);
}
//Rule genericEventRule = CREATE_DEFAULT_EVENT_RULE(onGenericEvent);

static volatile int monitorRuleApplied = 0;
static void onMonitorDetectionTest(void){
    monitorRuleApplied++;
}
static void setup(){
    createContextAndSimpleConnection();
    registerForEvents();
    START_MY_WM
}


/**
 * Tests to see if events received actually make it to call back function
 * @param START_TEST(test_regular_events)
 */
START_TEST(test_regular_events){
    int count = -1;
    int i;
    //int lastType=-1;
    void func(void){
        count++;
        if(i==ExtraEvent)
            return;
        assert(i == (((xcb_generic_event_t*)getLastEvent())->response_type & 127));
        if(i > 2)
            assert(isSyntheticEvent());
    }
    IGNORE_SEND_EVENT = 0;
    POLL_COUNT = 1;
    int idleCount = -1;
    for(i = count + 1; i < XCB_GE_GENERIC; i++){
        xcb_generic_event_t* event = calloc(32, 1);
        event->response_type = i;
        Rule r = CREATE_DEFAULT_EVENT_RULE(func);
        assert(!isNotEmpty(getEventRules(i)));
        appendRule(i, &r);
        if(i){
            if(i == XCB_CLIENT_MESSAGE){
                assert(!catchError(xcb_ewmh_send_client_message(dis, root, root, 1, 0, 0)));
                //cookie=xcb_ewmh_send_client_message(dis, root, root, 0, 0, NULL);
                //cookie=xcb_ewmh_request_change_current_desktop_checked(ewmh, defaultScreenNumber, 0, 0);
            }
            else if(i == XCB_GE_GENERIC){
                ROOT_DEVICE_EVENT_MASKS = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS;
                registerForDeviceEvents();
                sendButtonPress(1);
            }
            else if(i == ExtraEvent){
                // will cause a 'unknown' ExtraEvent(1) as a side effect
                system("xsane-xrandr --reset &>/dev/null");
            }
            else assert(!catchError(xcb_send_event_checked(dis, 0, root, ROOT_EVENT_MASKS, (char*) event)));
        }
        else {
            //generate error
            xcb_send_event(dis, 0, root, ROOT_EVENT_MASKS, (char*) event);
            xcb_map_window(dis, -1);
            flush();
            WAIT_UNTIL_TRUE(count >= i);
            consumeEvents();
            count = i;
        }
        xcb_flush(dis);
        WAIT_UNTIL_TRUE(count == i);
        //assert(lastType==i);
        free(event);
        //assert(count==i);
        WAIT_UNTIL_TRUE(getIdleCount() > idleCount);
        idleCount = getIdleCount();
    }
}
END_TEST

START_TEST(test_event_spam){
    ROOT_DEVICE_EVENT_MASKS = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS;
    registerForDeviceEvents();
    Rule exitRule = CREATE_WILDCARD(BIND(requestShutdown));
    POLL_COUNT = 10;
    POLL_INTERVAL = 10;
    appendRule(Idle, &exitRule);
    appendRule(Periodic, &exitRule);
    xcb_generic_event_t event = {.response_type = 2};
    while(!isShuttingDown()){
        sendButtonPress(1);
        flush();
        xcb_send_event(dis, 0, root, ROOT_EVENT_MASKS, (char*) &event);
        flush();
    }
}
END_TEST

START_TEST(test_register_events){
    registerForEvents();
    xcb_get_window_attributes_reply_t* attr;
    attr = xcb_get_window_attributes_reply(dis, xcb_get_window_attributes(dis, root), NULL);
    assert(attr->your_event_mask == ROOT_EVENT_MASKS);
    free(attr);
}
END_TEST
START_TEST(test_register_for_device_events){
    ROOT_DEVICE_EVENT_MASKS = XI_KeyPressMask | XI_KeyReleaseMask |
                              XI_ButtonPressMask | XI_ButtonReleaseMask;
    registerForDeviceEvents();
    assert(getSize(getDeviceBindings()));
    FOR_EACH(Binding*, binding, getDeviceBindings()) assert(binding->detail);
    triggerAllBindings(ROOT_DEVICE_EVENT_MASKS);
    waitToReceiveInput(ROOT_DEVICE_EVENT_MASKS);
}
END_TEST

START_TEST(test_monitors){
    registerForMonitorChange();
    static Rule r = CREATE_DEFAULT_EVENT_RULE(onMonitorDetectionTest);
    appendRule(onScreenChange, &r);
    if(!fork()){
        close(1);
        close(2);
        system("xrandr --output screen --primary");
        exit(0);
    }
    START_MY_WM;
    wait(NULL);
    WAIT_UNTIL_TRUE(monitorRuleApplied);
}
END_TEST

static void* testFunction(void* p  __attribute__((unused))){
    return NULL;
}
START_TEST(test_threads){
    char c;
    pthread_t thread = runInNewThread(testFunction, &c, _i);
    assert(thread);
    if(_i == 0)
        pthread_join(thread, NULL);
}
END_TEST
START_TEST(test_load_generic_events){
    xcb_ge_generic_event_t event = {0};
    assert(loadGenericEvent(&event) == 0);
}
END_TEST
Suite* eventSuite(void){
    Suite* s = suite_create("Test Events");
    TCase* tc_core;
    tc_core = tcase_create("Threads");
    tcase_add_loop_test(tc_core, test_threads, 0, 2);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Events");
    tcase_add_checked_fixture(tc_core, setup, fullCleanup);
    tcase_add_test(tc_core, test_regular_events);
    tcase_add_test(tc_core, test_event_spam);
    tcase_add_test(tc_core, test_register_events);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Register for device events");
    tcase_add_checked_fixture(tc_core, bindingSetup, destroyContextAndConnection);
    tcase_add_test(tc_core, test_register_for_device_events);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("OtherEvents");
    tcase_add_checked_fixture(tc_core, createContextAndSimpleConnection, fullCleanup);
    tcase_add_test(tc_core, test_monitors);
    tcase_add_test(tc_core, test_load_generic_events);
    suite_add_tcase(s, tc_core);
    return s;
}
