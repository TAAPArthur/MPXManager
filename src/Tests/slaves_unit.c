#include "../slaves.h"
#include "test-mpx-helper.h"

SCUTEST_SET_ENV(addDefaultMaster, simpleCleanup);
SCUTEST(slaves) {
    newSlave(100, DEFAULT_KEYBOARD, 1, "", 0);
    assert(getAllSlaves()->size);
    assert(getSlaveByName(""));
}
SCUTEST(detect_test_slaves) {
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
    for(int i = 0; i < LEN(testDevices); i++)
        assert(isTestDevice(testDevices[i], strlen(testDevices[i])));
    for(int i = 0; i < LEN(nonTestDevices); i++)
        assert(!isTestDevice(nonTestDevices[i], strlen(nonTestDevices[i])));
}
