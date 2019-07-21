#include "tester.h"
#include "test-mpx-helper.h"
#include "../logger.h"
#include "../system.h"
#include "../mywm-structs.h"
#include "../masters.h"
#include "../windows.h"

SET_ENV(addDefaultMaster, simpleCleanup);
MPX_TEST("slaves", {
    std::string keyboardName = "K";
    std::string pointerName = "P";
    assert(!getSlaveByName(keyboardName));
    assert(!getSlaveByName(pointerName));
    getAllSlaves().add(new Slave{100, DEFAULT_KEYBOARD, 1, keyboardName});
    assert(getAllSlaves()[0]->isKeyboardDevice());
    getAllSlaves().add(new Slave{101, DEFAULT_POINTER, 0, pointerName});
    assert(!getAllSlaves()[1]->isKeyboardDevice());
    assertEquals(getSlaveByName(keyboardName), getAllSlaves()[0]);
    assertEquals(getSlaveByName(pointerName), getAllSlaves()[1]);
});
MPX_TEST("detect_test_slaves", {
    const char* testDevices[] = {
        "XTEST pointer",
        "XTEST keyboard",
        "dummy XTEST pointer",
        "dummy XTEST keyboard",
    };
    const char* nonTestDevices[] = {
        "XTEST",
        "xtest keyboard",
        "xtest pointer",
        "XTEST pointer dummy",
        "TEST keyboard dummy",
    };
    for(auto name : testDevices)
        assert(Slave::isTestDevice(name));
    for(auto name : nonTestDevices)
        assert(!Slave::isTestDevice(name));
});
