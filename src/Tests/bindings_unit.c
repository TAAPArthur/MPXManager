#include "../bindings.h"
#include "../devices.h"
#include "../globals.h"
#include "../windows.h"
#include "../xutil/test-functions.h"
#include "test-event-helper.h"
#include "test-mpx-helper.h"
#include "tester.h"
#include <stdlib.h>


SCUTEST_SET_ENV(createXSimpleEnv, cleanupXServer);

SCUTEST_ITER(test_grab_ungrab_bindings, 2) {
    Binding sampleBinding = {0, XK_A, {incrementCount}, .flags = {.noGrab = 1}};
    initSingleBinding(&sampleBinding);
    assertEquals(0, grabBinding(&sampleBinding, 0));
    assert(sampleBinding.detail);
    assertEquals(0, grabBinding(&sampleBinding, 1));
}


SCUTEST(test_grab_ungrab_device) {
    Binding sampleBinding = {0, 1, {incrementCount}, .flags = {.grabDevice= 1},
        .chainMembers = CHAIN_MEM(
                {0, 2, {incrementCount}},
                {0, 3, {incrementCount}, .flags={.popChain=1}}
                )};
    initSingleBinding(&sampleBinding);
    addBindings(&sampleBinding, 1);
    BindingEvent event = {0, 1};
    BindingEvent eventInner = {0, 2};
    BindingEvent eventOther = {0, 3};
    checkBindings(&event);
    assertEquals(1, getCount());
    consumeEvents();
    checkBindings(&eventInner);
    clickButton(5, getActiveMasterPointerID());
    waitToReceiveInput(XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS | XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE, 0);
    assertEquals(2, getCount());
    checkBindings(&eventOther);
    assertEquals(3, getCount());
    consumeEvents();
    clickButton(5, getActiveMasterPointerID());
    assert(!consumeEvents());
}

#define OTHER_MODE 1
static Binding sampleBindings[] = {
    {0, 1, {incrementCount}},
    {0, 3, {fail} , .flags.mode = OTHER_MODE | 2},
    {0, 3, {incrementCount}, .flags.noShortCircuit = 1, .flags.mode = OTHER_MODE},
    {0, 3, {incrementCount}, .flags.mode = ANY_MODE},
    {0, 1, {incrementCount}},
    {
        0, 2, {incrementCount}, .flags.noShortCircuit = 1, .chainMembers = CHAIN_MEM(
        {0, 2, {incrementCount}},
        {0, 3, {incrementCount}, .flags.popChain = 1, },
        )
    },
    {
        0, 4, {incrementCount}, .flags.noShortCircuit = 1, .flags.mask = 1, .chainMembers = CHAIN_MEM(
        {0, 4, {incrementCount}},
        {0, 3, {incrementCount}, .flags.popChain = 1},
        )
    },
};

static void setupBindings() {
    createXSimpleEnv();
    addBindings(sampleBindings, LEN(sampleBindings));
    initGlobalBindings();
    grabAllBindings(NULL, 0, 0);
}
SCUTEST_SET_ENV(setupBindings, cleanupXServer);
SCUTEST_ITER(test_addBindings, 2) {
    for(int i = 0; i < LEN(sampleBindings); i++)
        assert(sampleBindings[i].detail);
}

SCUTEST(test_check_bindings) {
    BindingEvent event = {0, 1};
    sampleBindings[0].flags.noShortCircuit = 0;
    checkBindings(&event);
    assertEquals(1, getCount());
}

SCUTEST(test_check_bindings_short_cricuit) {
    BindingEvent event = {0, 1};
    sampleBindings[0].flags.noShortCircuit = 1;
    checkBindings(&event);
    assertEquals(2, getCount());
}

SCUTEST(test_check_bindings_mode) {
    setActiveMode(OTHER_MODE);
    dumpMaster(getActiveMaster());
    BindingEvent event = {0, 3};
    checkBindings(&event);
    assertEquals(2, getCount());
}

SCUTEST(test_check_bindings_global_chain) {
    createMasterDevice("test");
    BindingEvent event = {0, 4};
    BindingEvent eventEnd = {0, 3};
    FOR_EACH(Master*, master, getAllMasters()) {
        setActiveMaster(master);
        checkBindings(&event);
    }
    assertEquals(2, getCount());
    checkBindings(&eventEnd);
    assertEquals(3, getCount());
}

SCUTEST(test_check_bindings_chain) {
    BindingEvent event = {0, 2};
    BindingEvent eventEnd = {0, 3};
    checkBindings(&event);
    assertEquals(2, getCount());
    assertEquals(1, getActiveMaster()->bindings.size);
    checkBindings(&event);
    assertEquals(3, getCount());
    assertEquals(1, getActiveMaster()->bindings.size);
    checkBindings(&eventEnd);
    assertEquals(4, getCount());
    assertEquals(0, getActiveMaster()->bindings.size);
}

SCUTEST(test_check_bindings_chain_external_event) {
    BindingEvent event = {0, 2};
    BindingEvent outOfChainEvent = {0, 1};
    checkBindings(&event);
    assertEquals(2, getCount());
    assertEquals(1, getActiveMaster()->bindings.size);
    checkBindings(&outOfChainEvent);
    assertEquals(2, getCount());
    assertEquals(1, getActiveMaster()->bindings.size);
}

