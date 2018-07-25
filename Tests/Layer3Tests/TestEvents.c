
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

#include "../../wmfunctions.h"
#include "../../defaults.h"
#include "../../devices.h"
#include "../../default-rules.h"
#include "../../event-loop.h"
#include "../../mywm-structs.h"

#include "../UnitTests.h"
#include "../TestX11Helper.h"
//#include "TestHelper.c"

START_TEST(test_regular_events){
    createContext(1);
    onStartup();
    clearAllRules();
    connectToXserver();
    registerForEvents();

    START_MY_WM

    assert(LEN(eventRules)==NUMBER_OF_EVENT_RULES);
    int count=-1;
    int lastType=-1;

    void func(){
        lastType=((xcb_generic_event_t*)getLastEvent())->response_type & 127;
        count++;
    }

    IGNORE_SEND_EVENT=0;
    for(char i=count+1;i<XCB_GE_GENERIC;i++){
        if(i==1){
            count++;
            continue;
        }
        xcb_generic_event_t* e=calloc(32, 1);
        LOG(LOG_LEVEL_DEBUG,"sending event %d\n",i);
        e->response_type=i;

        Rules r=CREATE_DEFAULT_EVENT_RULE(func);
        insertHead(eventRules[i],&r);

        xcb_void_cookie_t cookie;
        if(i){
            if(i==XCB_CLIENT_MESSAGE){
                //cookie=xcb_ewmh_send_client_message(dis, root, root, 0, 0, NULL);
                cookie=xcb_ewmh_request_change_current_desktop(ewmh, defaultScreenNumber, 0, 0);
            }
            else cookie=xcb_send_event_checked(dis, 0, root, ROOT_EVENT_MASKS,(char*) e);
            xcb_generic_error_t *error=xcb_request_check(dis,cookie);
            if(error){
                logError(error);
                assert(0);
            }
        }
        else{
            xcb_map_window(dis, -1);
        }

        xcb_flush(dis);
        WAIT_UNTIL_TRUE(count==i);
        assert(lastType==i);
        //assert(count==i);
    }

}END_TEST


Suite *eventSuite(void) {
    Suite*s = suite_create("My Window Manager");
    TCase* tc_core = tcase_create("Events");
    tcase_add_test(tc_core, test_regular_events);
    suite_add_tcase(s, tc_core);
    return s;
}
