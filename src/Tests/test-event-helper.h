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
#include "../wm-rules.h"
#include "../device-grab.h"
#include "../user-events.h"
#include "../debug.h"
static inline void waitUntilIdle(bool safe = 0) {
    flush();
    static int idleCount;
    if(safe && idleCount == getIdleCount()) {
        msleep(100);
        if(idleCount == getIdleCount())
            return;
    }
    WAIT_UNTIL_TRUE(idleCount != getIdleCount());
    idleCount = getIdleCount();
}
static inline void onStartup() {
    addDieOnIntegrityCheckFailRule();
    addBasicRules();
    addWorkspaces(DEFAULT_NUMBER_OF_WORKSPACES);
    addDefaultMaster();
    openXDisplay();
}

static inline void exitFailure() {
    exit(10);
}

static inline void startWM() {
    runInNewThread(runEventLoop, NULL, "event-loop");
}
static inline void fullCleanup() {
    LOG(0, "full cleanup\n");
    setLogLevel(LOG_LEVEL_NONE);
    requestShutdown();
    if(getNumberOfThreads() && ewmh) {
        registerForWindowEvents(root, ROOT_EVENT_MASKS);
        //wake up other thread
        createNormalWindow();
        flush();
    }
    waitForAllThreadsToExit();
    LOG(0, "validating state\n");
    validate();
    getDeviceBindings().deleteElements();
    for(int i = 0; i < MPX_LAST_EVENT; i++) {
        for(const BoundFunction* b : getEventRules(i))
            if(b->func)
                assert(b->getName() != "");
        for(const BoundFunction* b : getBatchEventRules(i))
            if(b->func)
                assert(b->getName() != "");
    }
    LOG(0, "cleaning up xserver\n");
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
static inline void waitToReceiveInput(int mask, int detailMask = 0) {
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
    LOG(LOG_LEVEL_ALL, "Waiting for event of type %d\n", type);
    while(type) {
        xcb_generic_event_t* e = xcb_wait_for_event(dis);
        LOG(LOG_LEVEL_ALL, "Found event %p\n", e);
        if(!e)
            return 0;
        LOG(LOG_LEVEL_ALL, "type %d (%d) %s\n", e->response_type, e->response_type & 127,
            eventTypeToString(e->response_type & 127));
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
static inline const int NUM_BINDINGS = 5;
static inline const int NUM_ITER = NUM_BINDINGS * 3;
static inline Binding& getBinding(int _i) {
    uint32_t keyboardID = _i / 5 == 0 ? getActiveMasterKeyboardID() :
        _i / 5 == 1 ? XIAllDevices :  XIAllMasterDevices ;
    uint32_t pointerID = _i / 5 == 0 ? getActiveMasterPointerID() :
        _i / 5 == 1 ? XIAllDevices :  XIAllMasterDevices ;
    static Binding input[] = {
        {0, XK_a, {}, {.mask = XCB_INPUT_XI_EVENT_MASK_KEY_PRESS, .targetID = keyboardID}},
        {0, XK_a, {}, {.mask = XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE, .targetID = keyboardID}},
        {0, Button1, {}, {.mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS, .targetID = pointerID}},
        {0, Button1, {}, {.mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE, .targetID = pointerID}},
        {0, 0, {}, {.mask = XCB_INPUT_XI_EVENT_MASK_MOTION, .targetID = pointerID}},
    };
    return input[_i % 5];
}

static inline void testBiningSetup() {
    ROOT_DEVICE_EVENT_MASKS = 0;
    NON_ROOT_DEVICE_EVENT_MASKS = 0;
    ROOT_EVENT_MASKS = 0;
    NON_ROOT_EVENT_MASKS = 0;
    addDefaultMaster();
    for(int i = 0; i < NUM_BINDINGS; i++) {
        getDeviceBindings().add(getBinding(i));
    }
    openXDisplay();
    consumeEvents();
    assert(getDeviceBindings().size() == NUM_BINDINGS);
}
static inline void testGrabDetail(int _i, bool binding) {
    Binding& b = getBinding(_i);
    if(!b.getDetail())
        return;
    int id = b.getTargetID();
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
    int id = b.getTargetID();
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
