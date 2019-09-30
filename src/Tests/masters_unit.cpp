#include "tester.h"
#include "test-mpx-helper.h"
#include "../logger.h"
#include "../system.h"
#include "../mywm-structs.h"
#include "../masters.h"
#include "../windows.h"

SET_ENV(addDefaultMaster, simpleCleanup);

MPX_TEST("default_master", {
    assert(getActiveMaster());
    Master* m = getMasterById(DEFAULT_KEYBOARD);
    assert(m);
    assertEquals(getActiveMaster(), m);
    assertEquals(m, getMasterById(DEFAULT_POINTER, 0));
    assert(getAllMasters().size() == 1);
    assert(m->getKeyboardID() == DEFAULT_KEYBOARD);
    assert(m->getPointerID() == DEFAULT_POINTER);
    assert(m->getWindowStack().empty());
    assert(!getMasterByName("__bad__name__"));
    assertEquals(getMasterByName(getActiveMaster()->getName()), getActiveMaster());
});


MPX_TEST("onWindowFocus", {
    for(int i = DEFAULT_KEYBOARD + DEFAULT_POINTER; i < 10; i += 2)
        getAllMasters().add(new Master(i, i + 1, ""));

    for(int i = 0; i < 5; i++) {
        assert(addWindowInfo(new WindowInfo(i)));
        for(Master* m : getAllMasters()) {
            assert(m->getWindowStack().size() == i);
            unsigned int time = getTime();
            m->onWindowFocus(i);
            assert(m->getFocusedTime() >= time);
            assert(m->getFocusedWindow() == getWindowInfo(i));
            time = getTime();
            m->onWindowFocus(0);
            assert(m->getFocusedTime() >= time);
            assert(m->getFocusedWindow() == getWindowInfo(0));
        }
    }
});
MPX_TEST("onWindowFocus_bad", {
    for(int i = 0; i < 10; i++)
        getActiveMaster()->onWindowFocus(i);

    assert(getActiveMaster()->getWindowStack().size() == 0);
});
MPX_TEST("getMostRecentlyFocusedWindow", {
    WindowInfo* winInfo;
    winInfo = getActiveMaster()->getMostRecentlyFocusedWindow([](WindowInfo * w)->bool{return w->getID() > 100;});
    assert(winInfo == NULL);
    for(int i = 0; i < 10; i++) {
        assert(addWindowInfo(new WindowInfo(i)));
        getActiveMaster()->onWindowFocus(i);
        WindowInfo* winInfo = getActiveMaster()->getMostRecentlyFocusedWindow([](WindowInfo * w)->bool{return w ? 1 : 0;});
        assert(winInfo == getWindowInfo(i));
    }
    winInfo = getActiveMaster()->getMostRecentlyFocusedWindow([](WindowInfo * w)->bool{return w->getID() == 1;});
    assert(winInfo == getWindowInfo(1));
    winInfo = getActiveMaster()->getMostRecentlyFocusedWindow([](WindowInfo * w)->bool{return w->getID() % 2 == 0;});
    assert(winInfo == getWindowInfo(8));
    winInfo = getActiveMaster()->getMostRecentlyFocusedWindow([](WindowInfo * w)->bool{return w->getID() > 100;});
    assert(winInfo == NULL);
});
MPX_TEST("masterWindowStackIter", {
    int i;
    for(i = 0; i < 5; i++) {
        assert(addWindowInfo(new WindowInfo(i)));
        getActiveMaster()->onWindowFocus(i);
    }
    for(WindowInfo* winInfo : getActiveMaster()->getWindowStack())
        assertEquals(--i, winInfo->getID());
    assert(*getActiveMaster()->getWindowStack().begin() != *getActiveMaster()->getWindowStack().rbegin());
});

MPX_TEST("freeze_focus_stack", {
    assert(addWindowInfo(new WindowInfo(3)));
    assert(addWindowInfo(new WindowInfo(2)));
    assert(addWindowInfo(new WindowInfo(1)));
    getActiveMaster()->onWindowFocus(1);
    getActiveMaster()->onWindowFocus(2);

    ArrayList<WindowInfo*> list = getActiveMaster()->getWindowStack();
    getActiveMaster()->setFocusStackFrozen(true);

    assert(getActiveMaster()->isFocusStackFrozen());

    assert(list == getActiveMaster()->getWindowStack());
    assert(getFocusedWindow() == getWindowInfo(2));
    getActiveMaster()->onWindowFocus(1);
    assert(getFocusedWindow() == getWindowInfo(1));
    assert(list == getActiveMaster()->getWindowStack());

    getActiveMaster()->onWindowFocus(3);
    assert(getActiveMaster()->getWindowStack().size() == 3);
    assert(getFocusedWindow() == getWindowInfo(3));
    assert(list != getActiveMaster()->getWindowStack());

    getActiveMaster()->onWindowFocus(1);
    getActiveMaster()->onWindowFocus(2);
    getActiveMaster()->setFocusStackFrozen(false);
    assert(!getActiveMaster()->isFocusStackFrozen());
    assert(getFocusedWindow() == getWindowInfo(2));

    ArrayList<WindowInfo*>expected = ArrayList<WindowInfo*>({getWindowInfo(1), getWindowInfo(3), getWindowInfo(2)});
    assert(expected == getActiveMaster()->getWindowStack());

});
MPX_TEST("test_focus_delete", {
    assert(addWindowInfo(new WindowInfo(3)));
    assert(addWindowInfo(new WindowInfo(2)));
    assert(addWindowInfo(new WindowInfo(1)));
    assert(getFocusedWindow() == NULL);
    for(WindowInfo* winInfo : getAllWindows())
        getActiveMaster()->onWindowFocus(winInfo->getID());
    assert(getFocusedWindow() == getWindowInfo(1));
    //head is 1
    // stack bottom -> top 3 2 1
    getActiveMaster()->setFocusStackFrozen(true);
    getActiveMaster()->onWindowFocus(2);
    assert(getFocusedWindow() == getWindowInfo(2));
    getActiveMaster()->getWindowStack().removeElement(getFocusedWindow());
    assert(getFocusedWindow() == getWindowInfo(1));
    getActiveMaster()->getWindowStack().removeElement(getFocusedWindow());
    assert(getFocusedWindow() == getWindowInfo(3));
    getActiveMaster()->getWindowStack().removeElement(getFocusedWindow());
    assert(getFocusedWindow() == NULL);
    assert(getActiveMaster()->getWindowStack().size() == 0);
});

MPX_TEST("test_master_active", {
    assert(getActiveMaster());
    for(int i = DEFAULT_KEYBOARD + DEFAULT_POINTER + 1; i <= 10; i += 2) {
        getAllMasters().add(new Master(i, i + 1, ""));
        //non default masters are not auto set to active
        assert(getActiveMaster()->getKeyboardID() != i);
        setActiveMaster(getMasterById(i));
        Master* master = getMasterById(i);
        assert(master == getActiveMaster());
    }
});

MPX_TEST("get_master_by_id", {
    assert(getActiveMaster());
    assert(getMasterById(DEFAULT_KEYBOARD, 1));
    assert(!getMasterById(DEFAULT_POINTER, 1));
    assert(!getMasterById(0, 1));

    assert(!getMasterById(DEFAULT_KEYBOARD, 0));
    assert(getMasterById(DEFAULT_POINTER, 0));
    assert(!getMasterById(0, 0));
});

MPX_TEST("test_master_active_remove", {
    assert(getActiveMaster());
    MasterID fakeID = 100;
    assert(DEFAULT_KEYBOARD != fakeID);
    getAllMasters().add(new Master(fakeID, fakeID + 1, ""));
    setActiveMaster(getMasterById(fakeID));
    assert(getActiveMaster()->getKeyboardID() == fakeID);
    delete getAllMasters().removeElement(fakeID);
    assert(getActiveMaster()->getKeyboardID() == DEFAULT_KEYBOARD);
});

MPX_TEST("slaves", {
    Slave* s = new Slave(10, getActiveMasterKeyboardID(), 1);
    getAllSlaves().add(s);
    assertEquals(s, getActiveMaster()->getSlaves()[0]);
});