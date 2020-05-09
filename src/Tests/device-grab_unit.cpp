#include "test-x-helper.h"
#include "tester.h"
#include "test-event-helper.h"

#include "../bindings.h"
#include "../device-grab.h"
#include "../devices.h"
#include "../globals.h"
#include "../logger.h"
#include "../test-functions.h"
#include "../windows.h"
#include "../xsession.h"

using namespace TestGrabs;


SET_ENV(testBiningSetup, cleanupXServer);
MPX_TEST_ITER("test_init_binding", NUM_BINDINGS, {
    Binding& binding = getBinding(_i);
    assert(binding.getKeyBindings()[0]->getDetail() || binding.getKeyBindings()[0]->getButtonOrKey() == 0);
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
            if(getKeyboardMask(b->getMask()) == i)
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

MPX_TEST("replay_events", {
    grabDetail(2, 1, 0, XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS);
    getEventRules(XCB_GE_GENERIC).add(DEFAULT_EVENT([]{ loadGenericEvent((xcb_ge_generic_event_t*)getLastEvent()); }));
    getEventRules(XCB_GE_GENERIC).add(DEFAULT_EVENT(replayPointerEvent));
    getEventRules(XCB_GE_GENERIC).add(DEFAULT_EVENT(requestShutdown));
    Window win = mapArbitraryWindow();
    movePointer(1, 1, win);
    flush();
    if(!fork()) {
        saveXSession();
        openXDisplay();
        passiveGrab(win, XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS);
        clickButton(1);
        waitToReceiveInput(XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS, 0);
        closeConnection();
        exit(0);
    }
    runEventLoop();
    assertEquals(waitForChild(0), 0);
});


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
