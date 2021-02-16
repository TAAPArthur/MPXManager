#include <scutest/tester.h>
#include "test-mpx-helper.h"
#include "tester.h"
#include "../masters.h"
#include "../util/time.h"

SCUTEST_SET_ENV(addDefaultMaster, simpleCleanup);

SCUTEST(default_master) {
    assert(getActiveMaster());
    Master* m = getMasterByID(DEFAULT_KEYBOARD);
    assert(m == getActiveMaster());
    assert(m == getMasterByID(DEFAULT_POINTER));
    assert(getAllMasters()->size == 1);
    assert(getKeyboardID(m) == DEFAULT_KEYBOARD);
    assert(getPointerID(m) == DEFAULT_POINTER);
    assert(!getMasterWindowStack(m)->size);
    assert(!getMasterByName("__bad__name__"));
}

SCUTEST(test_onWindowFocus) {
    int NUM_MASTERS = 10;
    for(int i = DEFAULT_KEYBOARD + DEFAULT_POINTER; i < NUM_MASTERS; i++)
        addFakeMaster(i * 2, i * 2 + 1);
    for(int i = 1; i < 5; i++) {
        addFakeWindowInfo(i);
        FOR_EACH(Master*, m, getAllMasters()) {
            setActiveMaster(m);
            assert(i - 1 == getMasterWindowStack(m)->size);
            onWindowFocus(i);
            assertEquals(getFocusedWindow(), getWindowInfo(i));
            onWindowFocus(1);
            assertEquals(getFocusedWindow(), getWindowInfo(1));
        }
    }
}
SCUTEST(test_focus_dups) {
    addFakeWindowInfo(1);
    onWindowFocus(1);
    onWindowFocus(1);
    assertEquals(getActiveMasterWindowStack()->size, 1);
    setActiveMaster(addFakeMaster(4, 5));
    assertEquals(getActiveMasterWindowStack()->size, 0);
}
SCUTEST(onWindowFocus_bad) {
    for(int i = 0; i < 10; i++)
        onWindowFocus(i);
    assert(!getMasterWindowStack(getActiveMaster())->size);
}
SCUTEST(onWindowFocusNull) {
    assert(!getFocusedWindow());
}

SCUTEST(freeze_focus_stack) {
    addFakeWindowInfo(3);
    addFakeWindowInfo(2);
    addFakeWindowInfo(1);
    onWindowFocus(1);
    onWindowFocus(2);
    setFocusStackFrozen(true);
    assert(isFocusStackFrozen());
    assertEquals(getFocusedWindow(), getWindowInfo(2));
    onWindowFocus(1);
    assertEquals(getFocusedWindow(), getWindowInfo(1));
    onWindowFocus(3);
    assertEquals(getActiveMasterWindowStack()->size, 3);
    assertEquals(getFocusedWindow(), getWindowInfo(3));
    onWindowFocus(1);
    onWindowFocus(2);
    setFocusStackFrozen(false);
    assert(!isFocusStackFrozen());
    assert(getFocusedWindow() == getWindowInfo(2));
    assertEquals(getElement(getActiveMasterWindowStack(), 0), getWindowInfo(2));
    assertEquals(getElement(getActiveMasterWindowStack(), 1), getWindowInfo(3));
    assertEquals(getElement(getActiveMasterWindowStack(), 2), getWindowInfo(1));
}
SCUTEST(test_focus_delete) {
    for(int i = 3; i > 0; --i) {
        addFakeWindowInfo(i);
        onWindowFocus(i);
    }
    assert(getFocusedWindow() == getWindowInfo(1));
    //head is 1
    // stack top -> bottom 1 2 3
    setFocusStackFrozen(true);
    onWindowFocus(2);
    assert(getFocusedWindow() == getWindowInfo(2));
    freeWindowInfo(getFocusedWindow());
    assert(getFocusedWindow() == getWindowInfo(1));
    freeWindowInfo(getFocusedWindow());
    assert(getFocusedWindow() == getWindowInfo(3));
    freeWindowInfo(getFocusedWindow());
    assert(getFocusedWindow() == NULL);
    assertEquals(getActiveMasterWindowStack()->size, 0);
    assertEquals(getActiveMasterWindowStack()->size, 0);
}

SCUTEST(test_master_active) {
    assert(getActiveMaster());
    for(int i = DEFAULT_KEYBOARD + DEFAULT_POINTER + 1; i <= 10; i += 2) {
        addFakeMaster(i, i + 1);
        //non default masters are not auto set to active
        assert(getActiveMasterKeyboardID() != i);
        setActiveMaster(getMasterByID(i));
        Master* master = getMasterByID(i);
        assert(master == getActiveMaster());
    }
}

SCUTEST(get_master_by_id) {
    assert(getActiveMaster());
    assert(getMasterByID(DEFAULT_KEYBOARD));
    assert(getMasterByID(DEFAULT_KEYBOARD) == getMasterByID(DEFAULT_KEYBOARD));
    assert(!getMasterByID(1));
}

SCUTEST(test_master_active_remove) {
    assert(getActiveMaster());
    MasterID fakeID = 100;
    assert(DEFAULT_KEYBOARD != fakeID);
    addFakeMaster(fakeID, fakeID);
    setActiveMaster(getMasterByID(fakeID));
    assert(getActiveMasterKeyboardID() == fakeID);
    freeMaster(getActiveMaster());
    assert(getActiveMasterKeyboardID() == DEFAULT_KEYBOARD);
}

SCUTEST(test_slaves) {
    Slave* s = newSlave(10, getActiveMasterKeyboardID(), 1, "name");
    assertEquals(s, getElement(getSlaves(getActiveMaster()), 0));
}
