#ifndef MPX_TEST_EVENT_HELPER
#define MPX_TEST_EVENT_HELPER

#include <xcb/xcb.h>

#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>

#include <assert.h>
#include <err.h>
#include <math.h>
#include "test-mpx-helper.h"
#include "test-x-helper.h"
#include "../globals.h"
#include "../bindings.h"
#include "../test-functions.h"
#include "../logger.h"
#include "../xsession.h"
#include "../system.h"
#include "../masters.h"
#include "../default-rules.h"
#include "../device-grab.h"
#include "../user-events.h"
#include "../debug.h"
static inline void waitUntilIdle(void) {
    flush();
    static int idleCount;
    WAIT_UNTIL_TRUE(idleCount != getIdleCount());
    idleCount = getIdleCount();
}

static inline void startWM() {
    runInNewThread(runEventLoop, NULL, "event-loop");
}
static inline void fullCleanup() {
    LOG(0, "full cleanup\n");
    setLogLevel(LOG_LEVEL_NONE);
    requestShutdown();
    if(getNumberOfThreads() && dis) {
        registerForWindowEvents(root, ROOT_EVENT_MASKS);
        //wake up other thread
        createNormalWindow();
        flush();
    }
    waitForAllThreadsToExit();
    validate();
    getDeviceBindings().deleteElements();
    cleanupXServer();
}
static inline void triggerBinding(Binding* b, WindowID win = root) {
    if(b->getMask() & XCB_INPUT_XI_EVENT_MASK_MOTION) {
        movePointer(10, 10, getActiveMasterPointerID(), win);
        movePointer(0, 0, getActiveMasterPointerID(), win);
        return;
    }
    if(isKeyboardMask(b->getMask()))
        typeKey(b->getDetail(), 0, win);
    else
        clickButton(b->getDetail(), 0, win);
}
static inline void triggerAllBindings(int mask, WindowID win = root) {
    flush();
    for(Binding* b : getDeviceBindings()) {
        if(mask & b->getMask())
            triggerBinding(b, win);
    }
    xcb_flush(dis);
}


static inline void* getNextDeviceEvent() {
    xcb_flush(dis);
    xcb_generic_event_t* event = xcb_wait_for_event(dis);
    LOG(LOG_LEVEL_DEBUG, "Event received%d\n\n", ((xcb_generic_event_t*)event)->response_type);
    if(((xcb_generic_event_t*)event)->response_type == XCB_GE_GENERIC) {
        loadGenericEvent((xcb_ge_generic_event_t*)event);
        xcb_input_key_press_event_t* dEvent = (xcb_input_key_press_event_t*) event;
        LOG(LOG_LEVEL_DEBUG, "Detail %d type %s\n", dEvent->detail,
            eventTypeToString(GENERIC_EVENT_OFFSET + dEvent->event_type));
    }
    else if(((xcb_generic_event_t*)event)->response_type == 0) {
        LOG(LOG_LEVEL_ERROR, "error waiting on event\n");
        setLastEvent(event);
        applyEventRules(0, NULL);
    }
    return event;
}
static inline void waitToReceiveInput(int mask, int detailMask) {
    flush();
    LOG(LOG_LEVEL_ALL, "waiting for input %d\n\n", mask);
    while(mask || detailMask) {
        xcb_input_key_press_event_t* e = (xcb_input_key_press_event_t*)getNextDeviceEvent();
        LOG(LOG_LEVEL_ALL, "type %d (%d); detail %d remaining mask:%d %d\n", e->response_type, (1 << e->event_type), e->detail,
            mask, detailMask);
        mask &= ~(1 << e->event_type);
        detailMask &= ~(1 << e->detail);
        free(e);
        msleep(10);
    }
}

static inline int waitForNormalEvent(int type) {
    flush();
    while(type) {
        xcb_generic_event_t* e = xcb_wait_for_event(dis);
        LOG(LOG_LEVEL_ALL, "Found event %p\n", e);
        if(!e)
            return 0;
        LOG(LOG_LEVEL_ALL, "type %d %s\n", e->response_type, eventTypeToString(e->response_type));
        int t = e->response_type & 127;
        free(e);
        if(type == t)
            break;
    }
    return 1;
}
namespace TestGrabs {
#define ALL_MASKS KEYBOARD_MASKS|POINTER_MASKS
#include <X11/keysym.h>
static Binding input[] = {
    {0, XK_a, {}, {.mask = XCB_INPUT_XI_EVENT_MASK_KEY_PRESS}},
    {0, XK_a, {}, {.mask = XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE}},
    {0, Button1, {}, {.mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS}},
    {0, Button1, {}, {.mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE}},
    {0, 0, {}, {.mask = XCB_INPUT_XI_EVENT_MASK_MOTION}},
};
static inline const int NUM_ITER = LEN(input) * 3;
static inline const int NUM_BINDINGS = LEN(input);
static inline Binding& getBinding(int _i) {
    return input[_i % 5];
}
static inline int getID(int _i) {
    return _i / 5 == 0 ? XIAllMasterDevices :
           _i / 5 == 1 ? XIAllDevices :
           isKeyboardMask(getBinding(_i).getMask()) ? getActiveMasterKeyboardID() :
           getActiveMasterPointerID();
}

static inline void testBiningSetup() {
    ROOT_DEVICE_EVENT_MASKS = 0;
    NON_ROOT_DEVICE_EVENT_MASKS = 0;
    ROOT_EVENT_MASKS = 0;
    NON_ROOT_EVENT_MASKS = 0;
    openXDisplay();
    addDefaultMaster();
    for(int i = 0; i < LEN(input); i++) {
        getDeviceBindings().add(&input[i]);
    }
    triggerAllBindings(ALL_MASKS);
    consumeEvents();
    assert(getDeviceBindings().size() == LEN(input));
}
static inline void testGrabDetail(int _i, bool binding) {
    Binding& b = getBinding(_i);
    if(!b.getDetail())
        return;
    int id = getID(_i);
    if(binding)
        assert(!b.grab());
    else
        assert(!grabDetail(id, b.getDetail(), 0, b.getMask()));
    triggerBinding(&b);
    waitToReceiveInput(b.getMask(), 0);
}
static inline void testGrabDetailExclusive(int _i, bool binding) {
    Binding& b = getBinding(_i);
    if(!b.getDetail())
        return;
    int mask = b.getMask();
    int detail = b.getDetail();
    int id = getID(_i);
    if(binding)
        b.setTargetID(id);
    saveXSession();
    for(int mod = 0; mod < 3; mod++)
        for(int i = 0; i < 2; i++) {
            if(i == 0)
                if(binding)
                    assert(!b.grab());
                else
                    assert(!grabDetail(id, detail, mod, mask));
            else if(binding)
                assert(!b.ungrab());
            else
                assert(!ungrabDetail(id, detail, mod, isKeyboardMask(mask)));
            flush();
            if(!fork()) {
                openXDisplay();
                bool success = !(binding ? b.grab() : grabDetail(id, detail, mod, mask));
                assertEquals(i, success);
                if(success) {
                    if(binding)
                        assert(!b.ungrab());
                    else
                        assert(!ungrabDetail(id, detail, mod, isKeyboardMask(mask)));
                }
                closeConnection();
                exit(0);
            }
            assertEquals(waitForChild(0), 0);
        }
}
}
#endif
