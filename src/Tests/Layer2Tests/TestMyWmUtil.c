#include "../UnitTests.h"
#include "../TestX11Helper.h"
#include "../../mywm-util.h"
#include "../../spawn.h"

static int size = 10;



static void loadArr(int autoAdd, WindowInfo* arr[size]){
    for(int i = 0; i < size; i++){
        int value = i + 1;
        arr[i] = getWindowInfo(value);
        if(!arr[i])arr[i] = createWindowInfo(i + 1);
        if(autoAdd)
            addWindowInfo(arr[i]);
        else if(i % 2)markAsDock(arr[i]);
    }
}


void testAddUnique(ArrayList* head,
                   int(*add)(WindowInfo*)){
    assert(getSize(head) == 0);
    assert(add);
    WindowInfo* arr[size];
    loadArr(head != getAllWindows(), arr);
    for(int i = 2; i <= size * 2; i++){
        add(arr[i / 2 - 1]);
        assert(getSize(head) == i / 2);
        assert(isInList(head, i / 2));
    }
}
void testWindowAddRemove(ArrayList* head,
                         int(*add)(WindowInfo*),
                         int(*remove)(WindowID)){
    assert(head != NULL);
    assert(getSize(head) == 0);
    WindowInfo* arr[size];
    loadArr(head != getAllWindows(), arr);
    int count = 0;
    void dummyCount(){
        count++;
    }
    for(int i = 1; i <= size; i++){
        add(arr[i - 1]);
        assert(getSize(head) == i);
        assert(isInList(head, i));
    }
    for(int i = 1; i <= size; i++){
        remove(i);
        assert(getSize(head) == size - i);
        remove(i);
    }
    assert(getSize(head) == 0);
}


START_TEST(test_init_context){
    assert(!isNotEmpty(getAllWindows()));
    assert(!isNotEmpty(getAllMasters()));
    assert(getActiveMaster() == NULL);
    assert(!isNotEmpty(getAllMonitors()));
    assert(getNumberOfWorkspaces() == size);
    for(int i = 0; i < size; i++)
        assert(getWorkspaceByIndex(i));
}
END_TEST
START_TEST(test_init_pipe){
    pipe(statusPipeFD);
    resetPipe();
    pipe(statusPipeFD);
    assert(STATUS_FD);
}
END_TEST


START_TEST(test_destroy_context){
    resetContext();
    resetContext();
    addFakeMaster(1, 1);
    WindowInfo* winInfo = createWindowInfo(1);
    loadSampleProperties(winInfo);
    addWindowInfo(winInfo);
    addFakeMonitor(1);
    resetContext();
}
END_TEST


START_TEST(test_create_master){
    int size = NAME_BUFFER + 10;
    char* name = malloc(size);
    for(int i = 0; i < size; i++)
        name[i] = 'A';
    addMaster(1, 2, name, 'B');
    Master* m = getMasterById(1);
    assert(getSize(getAllMasters()) == 1);
    assert(m->id == 1);
    assert(m->pointerId == 2);
    assert(getSize(&m->windowStack) == 0);
    assert(m->focusedWindowIndex == 0);
    assert(getActiveMaster() == m);
    assert(!getSize(getActiveMasterWindowStack()));
    assert(getActiveWorkspaceIndex() == 0);
    assert(strncmp(m->name, name, NAME_BUFFER - 1) == 0);
    assert(m->name[NAME_BUFFER - 1] == 0);
    free(name);
    assert(m->name && m->name[0] + 1);
}
END_TEST

START_TEST(test_master_add_remove){
    assert(getSize(getAllMasters()) == 0);
    for(int n = 0; n < 2; n++){
        for(int i = 1; i <= size; i++)
            addFakeMaster(i, size + i);
        assert(getSize(getAllMasters()) == size);
        assert(getActiveMaster() != NULL);
    }
    for(int i = 1; i <= size; i++)
        assert(removeMaster(i));
    assert(!removeMaster(0));
    assert(getSize(getAllMasters()) == 0);
    assert(getActiveMaster() == NULL);
}
END_TEST

START_TEST(test_master_stack_add_remove){
    assert(getSize(getAllMasters()) == 0);
    int fakeOnWindowFocus(WindowInfo * winInfo){
        addWindowInfo(winInfo);
        int size = getSize(getActiveMasterWindowStack());
        onWindowFocus(winInfo->id);
        return size != getSize(getActiveMasterWindowStack());
    }
    int fakeRemove(WindowID i){
        return removeWindowFromMaster(getActiveMaster(), i);
    }
    for(int i = 1; i <= size; i++){
        addFakeMaster(i, i);
        setActiveMaster(getMasterById(i));
        testWindowAddRemove(getActiveMasterWindowStack(), fakeOnWindowFocus, fakeRemove);
        testAddUnique(getActiveMasterWindowStack(),
                      fakeOnWindowFocus);
    }
    FOR_EACH(Master*, m, getAllMasters()) assert(getSize(getWindowStackByMaster(m)) == size);
}
END_TEST
START_TEST(test_focus_time){
    addFakeMaster(1, 1);
    assert(addWindowInfo(createWindowInfo(3)));
    WindowInfo* winInfo = getHead(getAllWindows());
    unsigned int time = 0;
    for(int i = 0; i < 5; i++){
        onWindowFocus(winInfo->id);
        unsigned int newTime = getFocusedTime(getActiveMaster());
        assert(newTime > time);
        msleep(5);
    }
}
END_TEST
START_TEST(test_focus_delete){
    addFakeMaster(1, 1);
    assert(addWindowInfo(createWindowInfo(3)));
    assert(addWindowInfo(createWindowInfo(2)));
    assert(addWindowInfo(createWindowInfo(1)));
    assert(getFocusedWindow() == NULL);
    FOR_EACH(WindowInfo*, winInfo, getAllWindows()) onWindowFocus(winInfo->id);
    assert(getSize(getActiveMasterWindowStack()) == 3);
    assert(getFocusedWindow() == getWindowInfo(1));
    //head is 1
    setFocusStackFrozen(1);
    onWindowFocus(getWindowInfo(2)->id);
    assert(getFocusedWindow() == getWindowInfo(2));
    removeWindowFromMaster(getActiveMaster(), getFocusedWindow()->id);
    assert(getFocusedWindow() == getWindowInfo(3));
    removeWindowFromMaster(getActiveMaster(), getFocusedWindow()->id);
    assert(getFocusedWindow() == getWindowInfo(1));
    removeWindowFromMaster(getActiveMaster(), getFocusedWindow()->id);
    assert(getFocusedWindow() == 0);
    assert(getSize(getActiveMasterWindowStack()) == 0);
}
END_TEST

START_TEST(test_add_remove_master_window_cache){
    addFakeMaster(1, 1);
    assert(getWindowCache(getActiveMaster()));
    testAddUnique(getWindowCache(getActiveMaster()),
                  updateWindowCache);
    assert(getSize(getWindowCache(getActiveMaster())));
    clearWindowCache();
    assert(getSize(getWindowCache(getActiveMaster())) == 0);
}
END_TEST

START_TEST(test_master_active){
    assert(getActiveMaster() == NULL);
    addFakeMaster(1, 3);
    assert(getActiveMasterKeyboardID() == 1);
    assert(getActiveMasterPointerID() == 3);
    assert(getActiveMaster());
    for(int i = 2; i <= size; i++){
        assert(addFakeMaster(i, i));
        //non default masters are not auto set to active
        assert(getActiveMaster()->id != i);
        setActiveMaster(getMasterById(i));
        Master* master = find(getAllMasters(), &i, sizeof(int));
        assert(master == getActiveMaster());
    }
}
END_TEST
START_TEST(test_master_active_remove){
    assert(getActiveMaster() == NULL);
    addFakeMaster(1, 2);
    addFakeMaster(3, 4);
    removeMaster(1);
    assert(getActiveMaster() == getMasterById(3));
    assert(getActiveMasterKeyboardID() == 3);
    assert(getActiveMasterPointerID() == 4);
}
END_TEST
START_TEST(test_create_window_info){
    WindowInfo* winInfo = createWindowInfo(1);
    assert(winInfo->id == 1);
    addWindowInfo(winInfo);
}
END_TEST

START_TEST(test_window_add_remove){
    testWindowAddRemove(getAllWindows(), addWindowInfo, removeWindow);
    testAddUnique(getAllWindows(), addWindowInfo);
}
END_TEST


START_TEST(test_window_workspace_add_remove){
    int fakeAdd(WindowInfo * winInfo){
        addWindowInfo(winInfo);
        return addWindowToWorkspace(winInfo, 1);
    }
    testWindowAddRemove(getAllWindows(), fakeAdd, removeWindow);
    testAddUnique(getAllWindows(), fakeAdd);
}
END_TEST

/*
START_TEST(test_window_mask){
    WindowInfo* winInfo=createWindowInfo(1);

    assert(winInfo->mask==NO_MASK);
    for(unsigned int i=0;i<NUMBER_OF_MASKS;i++){
        unsigned int mask= 1<<i;
        setWindowMask(winInfo, mask);
        assert(getWindowMask(winInfo) == mask);
        removeWindowMask(winInfo, mask);
        assert(getWindowMask(winInfo) == 0 );
        mask--;
        setWindowMask(winInfo, mask);
        assert(getWindowMask(winInfo) == mask );
        removeWindowMask(winInfo, mask);
        assert(getWindowMask(winInfo) == 0 );
    }
    assert(winInfo->mask==NO_MASK);
    for(unsigned int i=0;i<NUMBER_OF_MASKS;i++){

        int mask=1<<(i);
        addWindowMask(winInfo, mask);
        assert(getWindowMask(winInfo) == (1<<(i+1)) -1 );
    }
    removeWindowMask(winInfo,NO_MASK);
    assert(getWindowMask(winInfo) & ALL_MASKS == ALL_MASKS);
}END_TEST
*/



START_TEST(test_init_workspace){
    for(int i = 0; i < getNumberOfWorkspaces(); i++){
        Workspace* workspace = getWorkspaceByIndex(i);
        assert(workspace->id == i);
        assert(getSize(getWindowStack(workspace)) == 0);
        assert(workspace->monitor == NULL);
        assert(workspace->activeLayout == NULL);
        assert(!isNotEmpty(&workspace->layouts));
        assert(!isWorkspaceVisible(i));
        assert(!isWorkspaceNotEmpty(i));
    }
}
END_TEST


START_TEST(test_visible_workspace){
    addFakeMaster(1, 1);
    assert(isWorkspaceVisible(getActiveWorkspaceIndex()) == 0);
    assert(isWorkspaceVisible(!getActiveWorkspaceIndex()) == 0);
    addFakeMonitor(1);
    assert(isWorkspaceVisible(getActiveWorkspaceIndex()));
    assert(isWorkspaceVisible(!getActiveWorkspaceIndex()) == 0);
    swapMonitors(0, 1);
    assert(isWorkspaceVisible(getActiveWorkspaceIndex()) == 0);
    assert(isWorkspaceVisible(!getActiveWorkspaceIndex()));
    addFakeMonitor(2);
    assert(isWorkspaceVisible(getActiveWorkspaceIndex()));
    assert(isWorkspaceVisible(!getActiveWorkspaceIndex()));
}
END_TEST
START_TEST(test_empty_workspace){
    addFakeMaster(1, 1);
    assert(isWorkspaceNotEmpty(0) == 0);
    addWindowInfo(createWindowInfo(1));
    assert(isWorkspaceNotEmpty(0) == 0);
    assert(getWorkspaceOfWindow(getHead(getAllWindows())) == NULL);
    addWindowToWorkspace(getWindowInfo(1), 0);
    assert(isWorkspaceNotEmpty(0));
    removeWindow(1);
    assert(isWorkspaceNotEmpty(0) == 0);
}
END_TEST
START_TEST(test_next_workspace){
    int size = 4;
    DEFAULT_NUMBER_OF_WORKSPACES = size;
    resetContext();
    assert(getNumberOfWorkspaces() == size);
    addFakeMaster(1, 1);
    for(int i = 0; i < 2; i++){
        //will take the first n workspaces
        addFakeMonitor(i);
        assert(isWorkspaceVisible(i));
        addWindowInfo(createWindowInfo(i + 1));
        addWindowToWorkspace(getWindowInfo(i + 1), i * 2);
        assert(getSize(getWindowStack(getWorkspaceByIndex(i * 2))) == 1);
    }
    int target[][4] = {
        {1, 2, 3, 0}, //any -1,-1
        {1, 0, 1, 0}, //visible -1,0
        {2, 3, 2, 3}, //hidden  -1,1
        {2, 0, 2, 0}, //non empty 0,-1
        {0, 0, 0, 0}, //nonempty,visible 0,0
        {2, 2, 2, 2}, //hidden nonempty 0,1
        {1, 3, 1, 3}, //empty   1,-1
        {1, 1, 1, 1}, //visible,empty   1,0
        {3, 3, 3, 3}, //hidden empty 1,1
    };
    for(int e = -1, count = 0; e < 2; e++)
        for(int h = -1; h < 2; h++, count++){
            setActiveWorkspaceIndex(0);
            for(int n = 0; n < size; n++){
                int mask = ((e + 1) << 2) + (h + 1);
                Workspace* w = getNextWorkspace(1, mask);
                assert(w == getNextWorkspace(1, mask));
                assert(w);
                assert(w->id == target[count][n]);
                setActiveWorkspaceIndex(w->id);
            }
        }
    setActiveWorkspaceIndex(0);
    for(int i = -getSize(getAllWindows()) * 2; i <= getSize(getAllWindows()) * 2; i++){
        Workspace* w = getNextWorkspace(i, 0);
        assert(w->id == (getActiveWorkspaceIndex() + i + getNumberOfWorkspaces()) % getNumberOfWorkspaces());
    }
}
END_TEST

START_TEST(test_window_cycle){
    addFakeMaster(1, 1);
    for(int i = 1; i <= size; i++){
        WindowInfo* winInfo = createWindowInfo(i);
        assert(addWindowInfo(winInfo));
        addWindowToWorkspace(winInfo, 0);
    }
    ArrayList* node = getWindowStack(getWorkspaceByIndex(0));
    int currentFocusedWindow = ((WindowInfo*)getHead(node))->id;
    onWindowFocus(currentFocusedWindow);
    assert(getFocusedWindow());
    FOR_EACH(WindowInfo*, winInfo, node){
        int size = getSize(node);
        onWindowFocus(winInfo->id);
        assert(getFocusedWindow() == winInfo);
        assert(size == getSize(node));
    }
}
END_TEST
START_TEST(test_active){
    addFakeMaster(1, 1);
    assert(getActiveWorkspaceIndex() == 0);
    assert(getActiveLayout() == 0);
    void* dummy = (void*)1;
    setActiveLayout(dummy);
    assert(getActiveLayout() == dummy);
    assert(getActiveWorkspace()->id == getActiveWorkspaceIndex());
    assert(getSize(getActiveMasterWindowStack()) == 0);
    assert(isNotEmpty(getActiveMasterWindowStack()) == 0);
}
END_TEST
START_TEST(test_workspace_add_remove){
    int starting = getNumberOfWorkspaces();
    for(int i = 1; i < 10; i++){
        addWorkspaces(1);
        assert(getNumberOfWorkspaces() == starting + i);
    }
    starting = getNumberOfWorkspaces();
    for(int i = 1; i < 10; i++){
        removeWorkspaces(1);
        assert(getNumberOfWorkspaces() == starting - i);
    }
}
END_TEST
START_TEST(test_workspace_window_add_remove){
    addFakeMaster(1, 1);
    int remove(WindowID win){
        WindowInfo* winInfo = getWindowInfo(win);
        int i = removeWindowFromWorkspace(winInfo);
        return i;
    }
    int add(WindowInfo * winInfo){
        int i = addWindowToWorkspace(winInfo, getActiveWorkspaceIndex());
        assert(getActiveWindowStack() == getWindowStackOfWindow(winInfo));
        return i;
    }
    testWindowAddRemove(getActiveWindowStack(), add, remove);
}
END_TEST


START_TEST(test_monitor_workspace){
    addFakeMaster(1, 1);
    for(int i=0;i<size+2;i++)
        addFakeMonitor(i);
    while(1){
        int count = 0;
        for(int i = 0; i < getNumberOfWorkspaces(); i++){
            if(isWorkspaceVisible(i)){
                count++;
                Monitor* m = getMonitorFromWorkspace(getWorkspaceByIndex(i));
                assert(m);
                Workspace* w = getWorkspaceFromMonitor(m);
                assert(w);
                assert(w->id == i);
            }
        }
        assert(count == MIN(getNumberOfWorkspaces(),getSize(getAllMonitors())));
        if(getSize(getAllMonitors()) > 1)
            assert(removeMonitor(((Monitor*)getElement(getAllMonitors(),getSize(getAllMonitors())/2))->id));
        else break;
    }
}
END_TEST

START_TEST(test_window_in_visible_worspace){
    addFakeMaster(1, 1);
    WindowInfo* winInfo = createWindowInfo(1);
    addWindowInfo(winInfo);
    assert(isWindowNotInInvisibleWorkspace(winInfo));
    addFakeMonitor(1);
    addWindowToWorkspace(winInfo, 0);
    assert(isWindowNotInInvisibleWorkspace(winInfo));
    swapMonitors(0, 1);
    assert(!isWindowNotInInvisibleWorkspace(winInfo));
    addWindowToWorkspace(winInfo, 1);
    assert(isWindowNotInInvisibleWorkspace(winInfo));
}
END_TEST
START_TEST(test_complete_window_remove){
    addWindowInfo(createWindowInfo(1));
    for(int i = 1; i <= getNumberOfWorkspaces(); i++){
        addFakeMaster(i, i);
        onWindowFocus(1);
    }
    addWindowToWorkspace(getWindowInfo(1), 0);
    assert(removeWindow(1));
    FOR_EACH(Master*, m, getAllMasters()) assert(getSize(getWindowStackByMaster(m)) == 0);
    for(int i = 0; i < getNumberOfWorkspaces(); i++){
        assert(getSize(getWindowStack(getWorkspaceByIndex(i))) == 0);
    }
}
END_TEST


START_TEST(test_spawn_env_vars){
    char* vars[2] = {getenv(CLIENT[0]), getenv(CLIENT[1])};
    spawn("exit 0");
    assert(getExitStatusOfFork() == 0);
    char* newVars[2] = {getenv(CLIENT[0]), getenv(CLIENT[1])};
    assert(memcmp(vars, newVars, sizeof(vars)) == 0);
    for(int i = 0; i < LEN(vars); i++){
        char buffer[64] = "exit $";
        strcat(buffer, CLIENT[i]);
        spawn(buffer);
        assert(getExitStatusOfFork() == (i == 0 ? getActiveMasterKeyboardID() : getActiveMasterPointerID()));
    }
}
END_TEST
START_TEST(test_spawn){
    spawn("exit 122");
    assert(getExitStatusOfFork() == 122);
}
END_TEST

START_TEST(test_spawn_wait){
    assert(waitForChild(spawn("exit 0")));
    assert(waitForChild(spawn("exit 122")));
}
END_TEST

Suite* mywmUtilSuite(void){
    Suite* s = suite_create("Context");
    TCase* tc_core;
    tc_core = tcase_create("Init");
    tcase_add_checked_fixture(tc_core, createSimpleContext, resetContext);
    tcase_add_test(tc_core, test_init_context);
    tcase_add_test(tc_core, test_create_window_info);
    tcase_add_test(tc_core, test_create_master);
    tcase_add_test(tc_core, test_init_workspace);
    tcase_add_test(tc_core, test_destroy_context);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Window");
    tcase_add_checked_fixture(tc_core, createSimpleContext, resetContext);
    tcase_add_test(tc_core, test_window_add_remove);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Master");
    tcase_add_checked_fixture(tc_core, createSimpleContext, resetContext);
    tcase_add_test(tc_core, test_master_add_remove);
    tcase_add_test(tc_core, test_add_remove_master_window_cache);
    tcase_add_test(tc_core, test_master_active);
    tcase_add_test(tc_core, test_master_active_remove);
    tcase_add_test(tc_core, test_focus_time);
    tcase_add_test(tc_core, test_focus_delete);
    //tcase_add_test(tc_core, test_set_last_window_clicked);
    tcase_add_test(tc_core, test_master_stack_add_remove);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Workspace");
    tcase_add_checked_fixture(tc_core, createSimpleContext, resetContext);
    tcase_add_test(tc_core, test_active);
    tcase_add_test(tc_core, test_workspace_add_remove);
    tcase_add_test(tc_core, test_workspace_window_add_remove);
    tcase_add_test(tc_core, test_window_workspace_add_remove);
    tcase_add_test(tc_core, test_visible_workspace);
    tcase_add_test(tc_core, test_empty_workspace);
    tcase_add_test(tc_core, test_next_workspace);
    //tcase_add_test(tc_core, test_window_cycle);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Combinations");
    tcase_add_checked_fixture(tc_core, createSimpleContext, resetContext);
    tcase_add_test(tc_core, test_complete_window_remove);
    tcase_add_test(tc_core, test_monitor_workspace);
    tcase_add_test(tc_core, test_window_in_visible_worspace);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Spawn_Functions");
    tcase_add_checked_fixture(tc_core, createContextAndSimpleConnection, fullCleanup);
    tcase_add_test(tc_core, test_spawn);
    tcase_add_test(tc_core, test_spawn_wait);
    tcase_add_test(tc_core, test_spawn_env_vars);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Misc");
    tcase_add_checked_fixture(tc_core, createSimpleContext, resetPipe);
    tcase_add_test(tc_core, test_init_pipe);
    suite_add_tcase(s, tc_core);
    return s;
}
