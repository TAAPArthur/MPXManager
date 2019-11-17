#include <math.h>
#include <X11/keysym.h>
#include "test-x-helper.h"
#include "tester.h"
#include "test-event-helper.h"

#include "../devices.h"
#include "../device-grab.h"
#include "../bindings.h"
#include "../globals.h"
#include "../xsession.h"
#include "../logger.h"
#include "../windows.h"
#include "../test-functions.h"

using namespace TestGrabs;


SET_ENV(testBiningSetup, cleanupXServer);
MPX_TEST_ITER("test_init_binding", NUM_BINDINGS, {
    Binding& binding = getBinding(_i);
    assert(binding.getDetail() || binding.getButtonOrKey() == 0);
});

MPX_TEST_ITER("test_passive_grab_ungrab", 2, {

    // bug with xserver? sending keypress triggers event 85
    triggerAllBindings(ALL_MASKS, root);
    consumeEvents();
    WindowID wins[]{root, mapWindow(createNormalWindow())};
    WindowID win = wins[_i];
    WindowID other = wins[!_i];
    assertEquals(consumeEvents(), 0);
    triggerAllBindings(ALL_MASKS, win);
    triggerAllBindings(ALL_MASKS, other);
    assertEquals(consumeEvents(), 0);
    passiveGrab(win, ALL_MASKS);
    assertEquals(consumeEvents(), 0);

    triggerAllBindings(ALL_MASKS, win);
    waitToReceiveInput(ALL_MASKS, 0);
    consumeEvents();
    passiveUngrab(win);
    triggerAllBindings(ALL_MASKS, win);
    triggerAllBindings(ALL_MASKS, other);
    assert(!xcb_poll_for_event(dis));
});
MPX_TEST("passive_regrab", {
    passiveGrab(root, KEYBOARD_MASKS);
    flush();
    passiveGrab(root, ALL_MASKS);
    triggerAllBindings(ALL_MASKS, root);
    waitToReceiveInput(ALL_MASKS, 0);
    passiveGrab(root, POINTER_MASKS);
    for(int i = 0; i < 2; i++) {
        for(Binding* b : getDeviceBindings())
            if(isKeyboardMask(b->getMask()) == i)
                triggerBinding(b);
        if(i == 0)
            waitToReceiveInput(POINTER_MASKS, 0);
        else
            assert(!xcb_poll_for_event(dis));
        consumeEvents();
    }
});

MPX_TEST_ITER("test_grab_device", NUM_ITER, {
    Binding& b = getBinding(_i);
    int mask = b.getMask();
    int id = b.getTargetID();
    if(isSpecialID(id))
        return;
    assert(!grabDevice(id, mask));
    triggerBinding(&b);
    waitToReceiveInput(mask, 0);
});
MPX_TEST_ITER("test_grab_device_exclusive", NUM_ITER, {
    Binding& b = getBinding(_i);
    int mask = b.getMask();
    int id = b.getTargetID();
    if(isSpecialID(id))
        return;
    saveXSession();
    for(int mod = 0; mod < 3; mod++)
        for(int i = 0; i < 2; i++) {
            if(i == 0)
                assert(!grabDevice(id, mask));
            else
                assert(!ungrabDevice(id));
            flush();
            if(!fork()) {
                openXDisplay();
                bool success = grabDevice(id, mask) ? 0 : 1;
                assert(i == success);
                closeConnection();
                exit(0);
            }
            assert(waitForChild(0) == 0);
        }
});


MPX_TEST_ITER("test_grab_detail", NUM_ITER, {
    testGrabDetail(_i, 0);
});
MPX_TEST_ITER("test_grab_ungrab_detail_exclusive", NUM_ITER, {
    testGrabDetailExclusive(_i, 0);
});

char deviceBindingTriggerCount[] = {0, 0, 0, 0};

//Rule genericEventRule = CREATE_DEFAULT_EVENT_RULE(onGenericEvent);


MPX_TEST("test_register_events", {
    registerForWindowEvents(root, ROOT_EVENT_MASKS);
    xcb_get_window_attributes_reply_t* attr;
    attr = xcb_get_window_attributes_reply(dis, xcb_get_window_attributes(dis, root), NULL);
    assert(attr->your_event_mask == ROOT_EVENT_MASKS);
    free(attr);
});
MPX_TEST("test_unregister_events", {
    registerForWindowEvents(root, ROOT_EVENT_MASKS);
    unregisterForWindowEvents(root);
    xcb_get_window_attributes_reply_t* attr;
    attr = xcb_get_window_attributes_reply(dis, xcb_get_window_attributes(dis, root), NULL);
    assert(attr->your_event_mask == 0);
    free(attr);
});
