#include "tester.h"
#include "test-mpx-helper.h"
#include "test-event-helper.h"
#include "test-x-helper.h"

#include "../bindings.h"
#include "../chain.h"
#include "../extra-rules.h"
#include "../devices.h"
#include "../globals.h"

#include <stdlib.h>

static Binding* autoGrabBinding = new Binding {0, Button1};
static Binding* nonAutoGrabBinding = new Binding {0, Button1, {incrementCount}, {.noGrab = 1}};
static Binding* nonAutoGrabBindingAbort = new Binding {0, Button2, {}, {.passThrough = NO_PASSTHROUGH, .noGrab = 1}};
static Chain sampleChain = {0, Button2, {}, {autoGrabBinding, nonAutoGrabBinding, nonAutoGrabBindingAbort}, {}, XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS };
static Chain sampleGlobalChain = {0, Button2, BoundFunction(NO_PASSTHROUGH), {}, {.passThrough = NO_PASSTHROUGH}, GLOBAL_CHAIN | XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS };
static UserEvent eventNoPassthrough;

static void setup() {
    createXSimpleEnv();
    eventNoPassthrough = {0, Button2};
}

SET_ENV(setup, fullCleanup);

MPX_TEST("auto_grab_detail", {
    addBasicRules();
    addApplyChainBindingsRule();
    sampleChain.start(eventNoPassthrough);
    getDeviceBindings().add(sampleChain);
    assertEquals(getCount(), 0);
    triggerBinding(nonAutoGrabBinding);
    addShutdownOnIdleRule();
    runEventLoop();
    assertEquals(getCount(), 1);
});
MPX_TEST_ITER("auto_grab_device", 3, {
    uint32_t masks[] = {XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS, XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE, XCB_INPUT_XI_EVENT_MASK_MOTION};
    uint32_t mask = masks[_i];
    Binding b = {0, Button1, {}, {.passThrough = NO_PASSTHROUGH, .noGrab = 1, .mask = mask}};
    Chain chain = Chain(0, 0, {
        b,
        {WILDCARD_MODIFIER, 0, {}, {.passThrough = NO_PASSTHROUGH, .noGrab = 1}},
    }, {.noGrab = 1}, mask);
    chain.trigger({0, Button1, .mask = mask});
    triggerBinding(&b);
    waitToReceiveInput(mask);
});

MPX_TEST_ITER("test_chain_grab", 3, {
    int32_t masks[] = {POINTER_MASKS, KEYBOARD_MASKS, POINTER_MASKS | KEYBOARD_MASKS};
    auto mask = masks[_i];
    static Chain* binding = new Chain(0, Button2, {}, {.passThrough = NO_PASSTHROUGH }, mask);
    binding->start({});
    if(!fork()) {
        saveXSession();
        openXDisplay();
        if(mask != POINTER_MASKS)
            assert(grabKeyboard());
        if(mask != KEYBOARD_MASKS)
            assert(grabPointer());
        quit(0);
    }
    assertEquals(waitForChild(0), 0);
    endActiveChain();
    assert(!getActiveChain());
});
MPX_TEST_ITER("test_start_end_grab", 2, {
    saveXSession();
    sampleChain.start(eventNoPassthrough);
    assert(getActiveChain() == &sampleChain);
    if(_i) {
        if(!fork()) {
            openXDisplay();
            assert(autoGrabBinding->grab());
            quit(0);
        }
        assertEquals(waitForChild(0), 0);
    }
    sampleChain.end();
    flush();
    if(_i) {
        if(!fork()) {
            openXDisplay();
            assert(autoGrabBinding->grab() == 0);
            quit(0);
        }
        assertEquals(waitForChild(0), 0);
    }
    assertEquals(waitForChild(0), 0);
    assert(getActiveChain() == NULL);
});
MPX_TEST_ITER("triple_start", 2, {
    for(int i = 0; i < 3; i++)
        if(_i)
            sampleChain.start(eventNoPassthrough);
        else
            sampleChain.trigger(eventNoPassthrough);
    assertEquals(getNumberOfActiveChains(), 3);
});
MPX_TEST_ITER("external_end", 2, {
    if(_i)
        sampleChain.start(eventNoPassthrough);
    else
        sampleGlobalChain.start(eventNoPassthrough);
    assert(getActiveChain());
    endActiveChain();
    assert(!getActiveChain());
});
MPX_TEST_ITER("chain_isolation", 2, {
    createMasterDevice("test");
    initCurrentMasters();
    setActiveMaster(getAllMasters()[0]);
    bool localChain = _i;
    if(localChain)
        sampleChain.start(eventNoPassthrough);
    else
        sampleGlobalChain.start(eventNoPassthrough);
    assert(getActiveChain());
    setActiveMaster(getAllMasters()[1]);
    assertEquals(!getActiveChain(), localChain);
    endActiveChain();
    assert(!getActiveChain());
});
MPX_TEST_ITER("chain_start_auto_passthrough", 2, {
    PassThrough p = _i ? ALWAYS_PASSTHROUGH : NO_PASSTHROUGH;
    static Chain sampleChain = {0, 0, {p}, {{0, 0, incrementCount}}};
    assertEquals(_i, sampleChain.start({0}));
    assertEquals(getCount(), _i);
    if(_i)
        endActiveChain();
});
MPX_TEST("chain_passthrough", {
    addBasicRules();
    addApplyChainBindingsRule();
    sampleChain.start(eventNoPassthrough);
    setLastUserEvent(eventNoPassthrough);
    getDeviceBindings().add({WILDCARD_MODIFIER, 0, {[]{assert(0);}}});
    applyEventRules(DEVICE_EVENT, NULL);
});

MPX_TEST("test_chain_bindings", {
    static int count = 0;
    unsigned int outerChainEndMod = 1;
    static void (*add)() = []{count++;};
    static void (*subtract)() = []{count--;};
    uint32_t dangerDetail = 2;
    uint32_t exitDetail = 3;
    void(*terminate)() = []() {exit(11);};
    Binding* dummy = new Binding{0, dangerDetail, terminate, {.passThrough = NO_PASSTHROUGH, .noGrab = 1}, .name = "dummy"};
    Binding* chain = new Chain(0, 1, add,
    {
        new Chain{
            0, 1, add,
            {
                new Chain(0, 1, add, {{0, 0, {}, {.passThrough = ALWAYS_PASSTHROUGH}}}),
                new Binding{0, 1, add, {.passThrough = NO_PASSTHROUGH}},
                new Binding{0, dangerDetail, add, {.passThrough = NO_PASSTHROUGH, .noGrab = 1}},
                new Binding{0, exitDetail, subtract, {.passThrough = ALWAYS_PASSTHROUGH, .noGrab = 1}}
            },
            {.passThrough = NO_PASSTHROUGH, .noGrab = 1},
        },
        new Binding{0, dangerDetail, terminate, {.passThrough = NO_PASSTHROUGH, .noGrab = 1}},
        new Binding{outerChainEndMod, 0, subtract, {.passThrough = ALWAYS_PASSTHROUGH, .noGrab = 1}},
        new Binding{WILDCARD_MODIFIER, 0, subtract, {.passThrough = ALWAYS_PASSTHROUGH}},
    },
    {.passThrough = NO_PASSTHROUGH, .noGrab = 1}
    );
    getDeviceBindings().add(dummy);
    getDeviceBindings().add(chain);
    assert(getActiveChain() == NULL);
    //enter first chain & second chain
    assertEquals(checkBindings({0, 1}), 0);
    assertEquals(getNumberOfActiveChains(), 2);
    assert(getActiveChain());
    assertEquals(count, 4);
    assertEquals(checkAllChainBindings({0, dangerDetail}), 0);
    assertEquals(count, 5);
    assertEquals(checkAllChainBindings({0, dangerDetail}), 0);
    assertEquals(count, 6);
    //exit second chain
    assertEquals(checkAllChainBindings({0, exitDetail}), 0);
    assertEquals(getNumberOfActiveChains(), 1);
    assert(getActiveChain());
    assertEquals(count, 5);
    //exit fist chain
    assertEquals(checkAllChainBindings({outerChainEndMod, 0}), 0);
    assertEquals(count, 3);
    assertEquals(getNumberOfActiveChains(), 0);
    assert(getActiveChain() == NULL);
    getDeviceBindings().deleteElements();
});


MPX_TEST("chain_get_name", {
    std::string name = "name";
    Chain chain = Chain(0, 0, {}, {.noGrab = 1,}, 0, name);
    assert(chain.getName() == name);
});
