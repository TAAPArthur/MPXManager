#include "tester.h"
#include "test-mpx-helper.h"
#include "test-event-helper.h"
#include "test-x-helper.h"

#include "../bindings.h"
#include "../chain.h"
#include "../devices.h"
#include "../globals.h"

#include <stdlib.h>

static Binding* autoGrabBinding = new Binding {0, Button1};
static Binding* nonAutoGrabBinding = new Binding {0, Button1, {}, {.noGrab = 1}};
static Binding* nonAutoGrabBindingAbort = new Binding {0, Button2, {}, {.passThrough = NO_PASSTHROUGH, .noGrab = 1}};
static Chain sampleChain = {0, 0, {}, {autoGrabBinding, nonAutoGrabBinding, nonAutoGrabBindingAbort}};
static UserEvent event = {0, Button1};
static UserEvent eventNoPassthrough = {0, Button2};

SET_ENV(createXSimpleEnv, fullCleanup);

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
    assertEquals(getActiveChains().size(), 3);
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
    assertEquals(getActiveChains().size(), 2);
    assert(getActiveChain());
    assertEquals(count, 4);
    assertEquals(checkAllChainBindings({0, dangerDetail}), 0);
    assertEquals(count, 5);
    assertEquals(checkAllChainBindings({0, dangerDetail}), 0);
    assertEquals(count, 6);
    //exit second chain
    assertEquals(checkAllChainBindings({0, exitDetail}), 0);
    assertEquals(getActiveChains().size(), 1);
    assert(getActiveChain());
    assertEquals(count, 5);
    //exit fist chain
    assertEquals(checkAllChainBindings({outerChainEndMod, 0}), 0);
    assertEquals(count, 3);
    assertEquals(getActiveChains().size(), 0);
    assert(getActiveChain() == NULL);
    getDeviceBindings().deleteElements();
});
