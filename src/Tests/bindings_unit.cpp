#include <stdlib.h>
#include "../bindings.h"
#include "../devices.h"
#include "../globals.h"
#include "../windows.h"
#include "test-event-helper.h"
#include "test-mpx-helper.h"
#include "tester.h"


SET_ENV(createXSimpleEnv, cleanupXServer);

MPX_TEST("print", {
    UserEvent e;
    Binding b = {0, 0};
    BindingFlags f = {.noGrab = 1};
    assert(f.noGrab);
    suppressOutput();
    std::cout << e;
    std::cout << b;
});
MPX_TEST("getLastUserEvent", {
    UserEvent e = { 1, 2};
    setLastUserEvent(e);
    UserEvent& e2 = getLastUserEvent();
    assertEquals(e.detail, e2.detail);
    assertEquals(e.mod, e2.mod);
});

MPX_TEST("test_binding_mode_match", {
    getActiveMaster()->setCurrentMode(0);
    Binding binding = {WILDCARD_MODIFIER, 0, {}, {.mode = 0}};
    Binding bindingWrongMode = {WILDCARD_MODIFIER, 0, {}, {.mode = 1}};
    Binding bindingWildcard = {WILDCARD_MODIFIER, 0, {}, {.mode = ANY_MODE}};
    UserEvent nullEvent = {0};
    assert(binding.matches(nullEvent));
    assert(!bindingWrongMode.matches(nullEvent));
    assert(bindingWildcard.matches(nullEvent));
});

MPX_TEST_ITER("check_bindings", NO_PASSTHROUGH, {
    const PassThrough passThrough = (PassThrough) _i;
    static int count = 0;
    static void (*funcNoArg)() = []() {
        count++;
    };
    IGNORE_MASK = 16;
    uint32_t mask = 4;
    uint32_t badMask = 8;
    getActiveMaster()->setCurrentMode(0);
    const int GOOD = 1, BAD = 0;
    Detail detail = 1;
    Modifier mod = 2;
    uint32_t mods[][2] = {{(uint32_t) -1, 0}, {WILDCARD_MODIFIER, mod}};
    uint32_t* modsPointer = &mods[0][0];
    uint32_t details[][2] = {{2, 7}, {0, detail}};
    uint32_t* detailsPointer = &details[0][0];
    uint32_t masks[][2] = {{badMask, 1}, {mask, mask | 1}};
    uint32_t* masksPointer = &masks[0][0];
    void (*terminate)() = []() {exit(10);};

    const int N = 1;
    for(int n = 0; n < N; n++) {
        for(uint32_t badDetail : details[BAD])
            for(int n = 0; n < sizeof(masks) / sizeof(masks[0][0]); n++)
                for(int i = 0; i < sizeof(mods) / sizeof(mods[0][0]); i++)
                    getDeviceBindings().add(new Binding{modsPointer[i], badDetail, terminate, {.mask = masksPointer[n]}, .name = "badDetail"});
        for(uint32_t badMod : mods[BAD])
            for(int n = 0; n < sizeof(masks) / sizeof(masks[0][0]); n++)
                for(int i = 0; i < sizeof(details) / sizeof(details[0][0]); i++)
                    getDeviceBindings().add(new Binding{badMod, detailsPointer[i], terminate, {.mask = masksPointer[n]}, .name = "badMod"});
        for(uint32_t badMask : masks[BAD])
            for(int i = 0; i < sizeof(mods) / sizeof(mods[0][0]); i++)
                for(int i = 0; i < sizeof(details) / sizeof(details[0][0]); i++)
                    getDeviceBindings().add(new Binding{modsPointer[i], detailsPointer[i], terminate, {.mask = badMask}, .name = "badMask"});
        for(uint32_t goodDetail : details[GOOD])
            for(uint32_t goodMask : masks[GOOD])
                for(uint32_t goodMod : mods[GOOD])
                    getDeviceBindings().add(new Binding{goodMod, goodDetail, funcNoArg, { .passThrough = passThrough, .mask = goodMask}});
    }
    UserEvent events[] = {
        {mod, detail, mask},
        {mod | IGNORE_MASK, detail, mask}
    };
    for(UserEvent e : events)
        checkBindings(e);
    switch(passThrough) {
        case PASSTHROUGH_IF_FALSE:
        case NO_PASSTHROUGH:
            assertEquals(count, LEN(events));
            break;
        case ALWAYS_PASSTHROUGH:
        case PASSTHROUGH_IF_TRUE:
            assertEquals(count, N * LEN(events)*LEN(masks[GOOD])*LEN(details[GOOD])*LEN(mods[GOOD]));
            break;
    }
    getDeviceBindings().deleteElements();
});
MPX_TEST_ITER("check_binding_target", 4, {
    static int count = 0;
    static WindowInfo* dummy = new WindowInfo(1);
    static WindowInfo* focusDummy = new WindowInfo(2);
    static WindowInfo* target = new WindowInfo(3);
    focusDummy->addMask(FOCUSABLE_MASK);
    getActiveMaster()->setCurrentMode(0);
    getAllWindows().add(dummy);
    getAllWindows().add(focusDummy);
    getAllWindows().add(target);
    getActiveMaster()->onWindowFocus(focusDummy->getID());
    static WindowParamType i = (WindowParamType) _i;
    static bool targetSet = _i > TARGET_WINDOW;
    static void (*funcWinArg)(WindowInfo * winInfo) = [](WindowInfo * winInfo) {
        count++;
        switch(i) {
            case DEFAULT_WINDOW:
            case FOCUSED_WINDOW:
                assert(focusDummy == winInfo);
                return;
            case TARGET_WINDOW:
                if(targetSet)
                    assert(target == winInfo);
                else
                    assert(dummy == winInfo);
                return;
        }
        assert(0);
    };
    static WindowParamType windowTarget = (WindowParamType) _i;
    if(targetSet) {
        setActiveTarget(target->getID());
        assertEquals(getTarget(), target->getID());
        windowTarget = DEFAULT_WINDOW;
        i = TARGET_WINDOW;
    }

    Binding b = {WILDCARD_MODIFIER, 0, funcWinArg, {.passThrough = NO_PASSTHROUGH, .mask = KEYBOARD_MASKS, .windowTarget = windowTarget}};
    getDeviceBindings().add(&b);
    UserEvent event = {1, 1, KEYBOARD_MASKS, 0, 0, getActiveMaster(), dummy};
    assert(checkBindings(event) == 0);
    getDeviceBindings().clear();
    assert(count);
    if(targetSet) {
        clearTarget();
        assertEquals(getTarget(), 0);
        windowTarget = DEFAULT_WINDOW;
        i = TARGET_WINDOW;
    }
});
MPX_TEST("event_rules", {
    for(int i = 0; i < NUMBER_OF_MPX_EVENTS; i++) {
        getEventRules(i).add(new BoundFunction(incrementCount));
        assert(applyEventRules(i, NULL));
        getBatchEventRules(i).add(new BoundFunction(incrementCount));
        assert(getCount() == i + 1);
    }
    applyBatchEventRules();
    assert(getCount() == NUMBER_OF_BATCHABLE_EVENTS + NUMBER_OF_MPX_EVENTS);
});
MPX_TEST("event_rules_abort", {
    getEventRules(0).add(new BoundFunction(incrementCount, "test", NO_PASSTHROUGH));
    getEventRules(0).add(new BoundFunction(incrementCount, "test2", NO_PASSTHROUGH));
    assert(!applyEventRules(0, NULL));
    assert(getCount() == 1);
});


MPX_TEST("test_no_grab", {
    Binding b = {0, 0, {}, { .noGrab = 1}};
    assert(b.grab());
    assert(b.ungrab());
});

using namespace TestGrabs;


SET_ENV(testBiningSetup, cleanupXServer);

MPX_TEST_ITER("test_grab_binding", NUM_ITER, {
    testGrabDetail(_i, 1);
});
MPX_TEST_ITER("test_grab_ungrab_binding_exclusive", NUM_ITER, {
    testGrabDetailExclusive(_i, 1);
});

