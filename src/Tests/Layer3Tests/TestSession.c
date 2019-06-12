#include "../UnitTests.h"
#include "../TestX11Helper.h"
#include "../../layouts.h"
#include "../../wmfunctions.h"
#include "../../functions.h"
#include "../../devices.h"
#include "../../masters.h"
#include "../../session.h"
#include "../../spawn.h"

START_TEST(test_sync_state){
    activateWorkspace(0);
    setShowingDesktop(0);
    assert(!isShowingDesktop());
    assert(getActiveWorkspaceIndex() == 0);
    if(!fork()){
        openXDisplay();
        activateWorkspace(1);
        setShowingDesktop(1);
        assert(isShowingDesktop());
        flush();
        closeConnection();
        exit(0);
    }
    waitForCleanExit();
    syncState();
    assert(getActiveWorkspaceIndex() == 1);
    assert(isShowingDesktop());
}
END_TEST
START_TEST(test_invalid_state){
    WindowID win = mapArbitraryWindow();
    xcb_ewmh_set_wm_desktop(ewmh, win, getNumberOfWorkspaces() + 1);
    xcb_ewmh_set_current_desktop(ewmh, defaultScreenNumber, getNumberOfWorkspaces() + 1);
    flush();
    addDefaultRules();
    syncState();
    scan(root);
    assert(getActiveWorkspaceIndex() < getNumberOfWorkspaces());
    assert(getWindowInfo(win)->workspaceIndex < getNumberOfWorkspaces());
}
END_TEST

START_TEST(test_restore_state){
    CRASH_ON_ERRORS = -1;
    void serializeState(ArrayList * list, bool shuffle){
        ArrayList listOfLists = {0};
        addToList(&listOfLists, getAllMasters());
        for(int i = 0; i < getNumberOfWorkspaces(); i++){
            addToList(&listOfLists, getWindowStack(getWorkspaceByIndex((i))));
            addToList(list, getActiveLayoutOfWorkspace(i));
            addToList(list, (void*)(long)getWorkspaceByIndex(i)->layoutOffset);
            addToList(list, 0);
        }
        for(int i = 0; i < getNumberOfWorkspaces(); i++){
            Monitor* m = getMonitorFromWorkspace(getWorkspaceByIndex(i));
            addToList(list, (void*)(long)(m ? m->id : 0));
        }
        addToList(list, 0);
        FOR_EACH(Master*, master, getAllMasters())addToList(&listOfLists, getWindowStackByMaster(master));
        FOR_EACH(ArrayList*, l, &listOfLists){
            if(shuffle && isNotEmpty(l))
                shiftToHead(l, getSize(l) - 1);
            FOR_EACH(int*, i, l) addToList(list, (void*)(long)*i);
            addToList(list, 0);
        }
        addToList(list, (void*)(long)getActiveMaster()->id);
        clearList(&listOfLists);
    }
    saveCustomState();
    flush();
    loadCustomState();
    createMasterDevice("test");
    createMasterDevice("test2");
    initCurrentMasters();
    MONITOR_DUPLICATION_POLICY = 0;
    createMonitor(getMonitorFromWorkspace(getActiveWorkspace())->base);
    createMonitor(getMonitorFromWorkspace(getActiveWorkspace())->base);
    detectMonitors();
    swapWorkspaces(0, 2);
    swapWorkspaces(1, 4);
    for(int i = 0; i < 10; i++)
        createNormalWindow();
    scan(root);
    FOR_EACH(WindowInfo*, winInfo, getAllWindows())onWindowFocus(winInfo->id);
    setActiveMaster(getLast(getAllMasters()));
    onWindowFocus(((WindowInfo*)getHead(getAllWindows()))->id);
    moveWindowToWorkspace(getLast(getAllWindows()), !getActiveWorkspaceIndex());
    moveWindowToWorkspace(getLast(getAllWindows()), getActiveWorkspaceIndex());
    moveWindowToWorkspace(getHead(getAllWindows()), !getActiveWorkspaceIndex());
    char* longName = "VERY_VERY_VERY_VERY_VERY_VERY_VERY_VER_LONG_NAME";
    getActiveLayout()->name = longName;
    setActiveLayout(&DEFAULT_LAYOUTS[GRID]);
    setActiveWorkspaceIndex(1);
    setActiveLayout(NULL);
    ArrayList list = {0};
    serializeState(&list, 0);
    saveCustomState();
    flush();
    if(!fork()){
        RUN_AS_WM = 0;
        ArrayList list2 = {0};
        resetContext();
        connectToXserver();
        loadCustomState();
        serializeState(&list2, 0);
        assert(getSize(&list) == getSize(&list2));
        assert(memcmp(list.arr, list2.arr, sizeof(long)*getSize(&list2)) == 0);
        clearList(&list2);
        quit(0);
    }
    waitForCleanExit();
    clearList(&list);
    resetContext();
    destroyAllNonDefaultMasters();
    clearFakeMonitors();
    initCurrentMasters();
    detectMonitors();
    loadCustomState();
    saveCustomState();
}
END_TEST



Suite* sessionSuite(){
    Suite* s = suite_create("Session");
    TCase* tc_core;
    tc_core = tcase_create("Sync_State");
    tcase_add_checked_fixture(tc_core, createContextAndSimpleConnection, destroyContextAndConnection);
    tcase_add_test(tc_core, test_sync_state);
    tcase_add_test(tc_core, test_invalid_state);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Restore_State");
    tcase_add_checked_fixture(tc_core, onStartup, destroyContextAndConnection);
    tcase_add_test(tc_core, test_restore_state);
    suite_add_tcase(s, tc_core);
    return s;
}
