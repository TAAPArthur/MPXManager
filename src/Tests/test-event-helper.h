#ifndef MPX_TEST_EVENT_HELPER
#define MPX_TEST_EVENT_HELPER

#include <assert.h>
#include "../bindings.h"
#include "../xutil/device-grab.h"
#include "../globals.h"
#include "../masters.h"
#include "../system.h"
#include "../xevent.h"
#include "../xutil/test-functions.h"
#include "../user-events.h"
#include "../util/time.h"
#include "test-mpx-helper.h"
#include "test-x-helper.h"
#include "tester.h"

static inline void waitUntilIdle() {
    flush();
    static int idleCount;
    WAIT_UNTIL_TRUE(idleCount != getIdleCount());
    idleCount = getIdleCount();
}

static inline void exitFailure() {
    quit(10);
}

static inline void startWM() {
    spawnThread(runEventLoop);
}
/*
static inline void triggerBinding(Binding* b, WindowID win = root) {
    if(b->getMask() & XCB_INPUT_XI_EVENT_MASK_MOTION) {
        movePointer(10, 10, win);
        movePointer(0, 0, win);
        return;
    }
    if(getKeyboardMask(b->getMask()))
        typeKey(b->getKeyBindings()[0]->getDetail());
    else
        clickButton(b->getKeyBindings()[0]->getDetail());
    flush();
}
static inline void triggerAllBindings(int mask, WindowID win = root) {
    flush();
    for(Binding* b : getDeviceBindings()) {
        if(mask & b->getMask())
            triggerBinding(b, win);
    }
    xcb_flush(dis);
}
*/


static inline void* getNextDeviceEvent() {
    flush();
    xcb_generic_event_t* event = xcb_wait_for_event(dis);
    DEBUG("Event received %d", ((xcb_generic_event_t*)event)->response_type);
    if(((xcb_generic_event_t*)event)->response_type == XCB_GE_GENERIC) {
        int type = loadGenericEvent((xcb_ge_generic_event_t*)event);
        xcb_input_key_press_event_t* dEvent = (xcb_input_key_press_event_t*) event;
        assertEquals(type, GENERIC_EVENT_OFFSET + dEvent->event_type);
        DEBUG("Detail %d Device type %d (%d) %s", dEvent->detail, dEvent->deviceid, dEvent->sourceid,
            eventTypeToString(GENERIC_EVENT_OFFSET + dEvent->event_type));
    }
    else if(((xcb_generic_event_t*)event)->response_type == 0) {
        ERROR("error waiting on event");
        setLastEvent(event);
        applyEventRules(0, NULL);
    }
    return event;
}
static inline void waitToReceiveInput(int mask, int detailMask) {
    flush();
    TRACE("waiting for input %d\n\n", mask);
    while(mask || detailMask) {
        xcb_input_key_press_event_t* e = (xcb_input_key_press_event_t*)getNextDeviceEvent();
        TRACE("type %d (%d); detail %d remaining mask:%d %d\n", e->response_type, (1 << e->event_type),
            e->detail,
            mask, detailMask);
        mask &= ~(1 << e->event_type);
        detailMask &= ~(1 << e->detail);
        free(e);
        msleep(10);
    }
}

static inline int waitForNormalEvent(int type) {
    flush();
    TRACE("Waiting for event of type %d\n", type);
    while(type) {
        xcb_generic_event_t* e = xcb_wait_for_event(dis);
        TRACE("Found event %p\n", e);
        if(!e)
            return 0;
        TRACE("type %d (%d) %s\n", e->response_type, e->response_type & 127,
            eventTypeToString(e->response_type & 127));
        int t = e->response_type & 127;
        free(e);
        if(type == t)
            break;
    }
    return 1;
}
static inline void generateMotionEvents(int num) {
    for(int i = 0; i < num; i++) {
        movePointerRelative(1, 1);
        movePointerRelative(-1, -1);
    }
    flush();
}
/*
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
    if(!b.getKeyBindings()[0]->getDetail())
        return;
    int id = b.getTargetID();
    if(binding)
        assert(!b.grab());
    else
        assert(!grabDetail(id, b.getKeyBindings()[0]->getDetail(), 0, b.getMask()));
    triggerBinding(&b);
    waitToReceiveInput(b.getMask(), 0);
}
static inline void testGrabDetailExclusive(int _i, bool binding) {
    Binding& b = getBinding(_i);
    if(!b.getKeyBindings()[0]->getDetail())
        return;
    int mask = b.getMask();
    int detail = b.getKeyBindings()[0]->getDetail();
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
                assert(!ungrabDetail(id, detail, mod, getKeyboardMask(mask)));
            flush();
            if(!fork()) {
                openXDisplay();
                bool failure = (binding ? b.grab() : grabDetail(id, detail, mod, mask));
                assertEquals(!i, failure);
                if(!failure) {
                    if(binding)
                        assert(!b.ungrab());
                    else
                        assert(!ungrabDetail(id, detail, mod, getKeyboardMask(mask)));
                }
                closeConnection();
                exit(0);
            }
            assertEquals(waitForChild(0), 0);
        }
}
}
*/
#endif
