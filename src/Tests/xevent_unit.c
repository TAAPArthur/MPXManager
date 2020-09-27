
#include "tester.h"
#include "test-x-helper.h"
#include "test-event-helper.h"

#include <unistd.h>
static void setup() {
    openXDisplay();
    setLogLevel(LOG_LEVEL_TRACE);
    addShutdownOnIdleRule();
    addXIEventSupport();
    ROOT_DEVICE_EVENT_MASKS = XCB_INPUT_XI_EVENT_MASK_HIERARCHY;
    registerForWindowEvents(root, ROOT_EVENT_MASKS);
    passiveGrab(root, ROOT_DEVICE_EVENT_MASKS);
    POLL_COUNT = 1;
}
SCUTEST_SET_ENV(setup, simpleCleanup);

static void test_synthetic_event_helper(xcb_generic_event_t* event) {
    assert(isSyntheticEvent(event));
    incrementCount();
}

SCUTEST(test_synthetic_event) {
    xcb_generic_event_t event = {0};
    event.response_type = XCB_UNMAP_NOTIFY;
    addEvent(XCB_UNMAP_NOTIFY, DEFAULT_EVENT(test_synthetic_event_helper));
    assert(!catchError(xcb_send_event_checked(dis, 0, root, ROOT_EVENT_MASKS, (char*) &event)));
    runEventLoop();
    assert(getCount());
}

static void test_xi_event_helper(xcb_input_hierarchy_event_t* event) {
    assert(event);
    incrementCount();
    assertEquals(event->event_type, XCB_INPUT_HIERARCHY);
}

SCUTEST(test_xi_event) {
    createMasterDevice("test");
    addEvent(XCB_GE_GENERIC + XCB_INPUT_HIERARCHY, DEFAULT_EVENT(test_xi_event_helper));
    runEventLoop();
    assert(getCount());
}

static void resendEvent(xcb_generic_event_t* event) {
    assert(event);
    event->response_type++;
    xcb_send_event(dis, 0, root, ROOT_EVENT_MASKS, (char*) &event);
}

SCUTEST(test_backlog_of_events, .iter = 3, .timeout = 5) {
    int totalEvents = MPX_EVENT_QUEUE_SIZE * (_i + 1);
    addEvent(XCB_UNMAP_NOTIFY, DEFAULT_EVENT(incrementCount));
    xcb_generic_event_t event = {.response_type = XCB_UNMAP_NOTIFY};
    for(int i = 0; i < totalEvents ; i++)
        xcb_send_event(dis, 0, root, ROOT_EVENT_MASKS, (char*) &event);
    runEventLoop();
    assertEquals(totalEvents, getCount());
}
SCUTEST(test_idle) {
    xcb_generic_event_t event = {.response_type = XCB_UNMAP_NOTIFY};
    xcb_send_event(dis, 0, root, ROOT_EVENT_MASKS, (char*) &event);
    flush();
    addEvent(XCB_UNMAP_NOTIFY, DEFAULT_EVENT(resendEvent));
    addEvent(XCB_UNMAP_NOTIFY + 1, DEFAULT_EVENT(resendEvent));
    addEvent(XCB_UNMAP_NOTIFY + 2, DEFAULT_EVENT(requestShutdown));
    runEventLoop();
}
