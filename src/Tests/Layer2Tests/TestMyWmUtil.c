#include "../UnitTests.h"
#include "../TestX11Helper.h"
#include "../../mywm-util.h"

static int size=10;



static void loadArr (int autoAdd,WindowInfo* arr[size]){
    for(int i=0;i<size;i++){
        int value=i+1;
        arr[i]=getWindowInfo(value);
        if(!arr[i])arr[i]=createWindowInfo(i+1);
        if(autoAdd)
            addWindowInfo(arr[i]);
        else if(i%2)markAsDock(arr[i]);
    }
}


void testAddUnique(ArrayList*head,
        int(*add)(WindowInfo*)){
    assert(getSize(head)==0);
    assert(add);
    WindowInfo* arr[size];
    loadArr(head!=getAllWindows(),arr);

    for(int i=2;i<=size*2;i++){
        add(arr[i/2-1]);
        assert(getSize(head)==i/2);
        assert(isInList(head, i/2));
    }
}
void testWindowAddRemove(ArrayList*head,
        int(*add)(WindowInfo*),
        int(*remove)(unsigned int)){

    assert(head!=NULL);
    assert(getSize(head)==0);

    WindowInfo* arr[size];
    loadArr(head!=getAllWindows(),arr);
    int count=0;
    void dummyCount(){count++;}

    for(int i=1;i<=size;i++){
        add(arr[i-1]);
        assert(getSize(head)==i);
        assert(isInList(head,i));
    }
    for(int i=1;i<=size;i++){
        remove(i);
        assert(getSize(head)==size-i);
        remove(i);
    }
    assert(getSize(head)==0);
}


START_TEST(test_init_context){
    assert(!isNotEmpty(getAllWindows()));
    assert(!isNotEmpty(getAllMasters()));
    assert(getActiveMaster()==NULL);
    assert(!isNotEmpty(getAllMonitors()));
    assert(getNumberOfWorkspaces()==size);
    for(int i=0;i<size;i++)
        assert(getWorkspaceByIndex(i));
}END_TEST


START_TEST(test_destroy_context){
    resetContext();
    resetContext();
    addFakeMaster(1,1);
    WindowInfo*winInfo=createWindowInfo(1);
    loadSampleProperties(winInfo);
    addWindowInfo(winInfo);
    addFakeMonitor(1);
    resetContext();
}END_TEST


START_TEST(test_create_master){
    addFakeMaster(1, 2);
    Master*m=getMasterById(1);
    assert(getSize(getAllMasters())==1);
    assert(m->id==1);
    assert(m->pointerId==2);
    assert(getSize(&m->windowStack)==0);
    assert(m->name && m->name[0]+1);
    assert(m->focusedWindowIndex==0);
    assert(getActiveMaster()==m);
    assert(!getSize(getActiveMasterWindowStack()));
    assert(getActiveWorkspaceIndex()==0);

}END_TEST

START_TEST(test_master_add_remove){
    assert(getSize(getAllMasters())==0);
    for(int n=0;n<2;n++){
        for(int i=1;i<=size;i++)
            addFakeMaster(i,size+i);
        assert(getSize(getAllMasters())==size);
        assert(getActiveMaster()!=NULL);
    }
    for(int i=1;i<=size;i++)
        assert(removeMaster(i));
    assert(!removeMaster(0));
    assert(getSize(getAllMasters())==0);
    assert(getActiveMaster()==NULL);

}END_TEST

START_TEST(test_master_stack_add_remove){
    assert(getSize(getAllMasters())==0);

    int fakeOnWindowFocus(WindowInfo*winInfo){

        addWindowInfo(winInfo);
        int size=getSize(getActiveMasterWindowStack());
        onWindowFocus(winInfo->id);
        return size!=getSize(getActiveMasterWindowStack());
    }
    int fakeRemove(unsigned int i){
        return removeWindowFromMaster(getActiveMaster(),i);
    }
    for(int i=1;i<=size;i++){
        addFakeMaster(i,i);
        setActiveMaster(getMasterById(i));
        testWindowAddRemove(getActiveMasterWindowStack(), fakeOnWindowFocus, fakeRemove);
        testAddUnique(getActiveMasterWindowStack(),
                fakeOnWindowFocus);
    }
    FOR_EACH(Master*m,getAllMasters(),assert(getSize(getWindowStackByMaster(m))==size);)
}END_TEST
/*
START_TEST(test_set_last_window_clicked){
    addFakeMaster(1,1);
    assert(getSize(getActiveMasterWindowStack())==0);
    //does not have to be window in context
    int lastWindowClicked=1;
    setLastWindowClicked(lastWindowClicked);
    assert(getSize(getActiveMasterWindowStack())==0);
    addWindowInfo(createWindowInfo(1));
    addWindowInfo(createWindowInfo(2));
    onWindowFocus(1);
    assert(getLastWindowClicked()==lastWindowClicked);
    onWindowFocus(2);
    assert(getLastWindowClicked()==lastWindowClicked);
}END_TEST
*/
/**
 * Test window changes stack change or lack thereof
 * LOOP test
 */
START_TEST(test_master_focus_stack){
/*
    int frozen=_i;
    addFakeMaster(1,1);
    //default value should be 0
    assert(isFocusStackFrozen()==0);
    setFocusStackFrozen(frozen);
    assert(isFocusStackFrozen()==frozen);

    for(int i=1;i<=size;i++){
        if(i==size){
            ArrayList*window=getActiveMasterWindowStack();
            addFakeMaster(2, 3);
            setActiveMaster(getMasterById(2));
            setFocusStackFrozen(frozen);
            FOR_EACH(WindowInfo*winInfo,window,onWindowFocus(winInfo->id))
        }
        assert(addWindowInfo(createWindowInfo(i)));
        unsigned int time=getTime();
        onWindowFocus(i);

        assert(getTime()>=getFocusedTime(getActiveMaster()));
        assert(time<=getFocusedTime(getActiveMaster()));


        ArrayList*window=getActiveMasterWindowStack();
        int count=0;
        if(frozen){
            assert(getSize(getActiveMasterWindowStack())==0);
            assert(getLastMasterToFocusWindow(i)==NULL);
        }
        else{
            assert(getLastMasterToFocusWindow(i)==getActiveMaster());
            assert(getFocusedWindow()->id==(unsigned int)i);
            assert(getSize(getActiveMasterWindowStack())==getSize(getAllWindows()));
        }
        int numberOfWindows=getSize(getActiveMasterWindowStack());
        FOR_EACH(window,
                ArrayList*temp=window->next;
            count++;
            unsigned int originalHeadValue=getIntValue(window);
            onWindowFocus(getIntValue(window));
            if(frozen)
                assert(((int)originalHeadValue)==getIntValue(getAllWindows()));
            else{
                //focused window should be the set window
                assert(getFocusedWindow()->id==originalHeadValue);
                //focused window should be at top
                assert(getFocusedWindow()==getValue(getActiveMasterWindowStack()));
                //LRU window cache should be updated
                assert(getIntValue(getActiveMasterWindowStack())==getIntValue(getAllWindows()));
            }

            window=temp;
            if(!temp)
                break;
            else continue;
        )
        //workspace stack unchanged
        assert(numberOfWindows==getSize(getActiveMasterWindowStack()));
        assert(count==numberOfWindows);

        if(!frozen)
            assert(getSize(getAllWindows())==numberOfWindows);
        assert(getSize(getActiveMasterWindowStack())==numberOfWindows);
        assert(getSize(getActiveWindowStack())==0);

    }
*/
}END_TEST
START_TEST(test_last_master_to_focus_window){
    addWindowInfo(createWindowInfo(1));
    assert(getLastMasterToFocusWindow(1)==NULL);
    for(int i=1;i<=size*2;i++){
        addFakeMaster(i,i);
        if(i%2==0)
            continue;
        setActiveMaster(getMasterById(i));
        onWindowFocus(1);
        assert(getLastMasterToFocusWindow(1));
        assert(getLastMasterToFocusWindow(1)->id==i);
    }
    for(int i=size*2-1;i>2;i-=2){
        removeMaster(i);
        assert(getLastMasterToFocusWindow(1));
        assert(i-2==getLastMasterToFocusWindow(1)->id);
    }
    removeMaster(1);
    assert(getLastMasterToFocusWindow(1)==NULL);

}END_TEST
START_TEST(test_focus_delete){
    addFakeMaster(1,1);
    assert(addWindowInfo(createWindowInfo(3)));
    assert(addWindowInfo(createWindowInfo(2)));
    assert(addWindowInfo(createWindowInfo(1)));
    assert(getFocusedWindow()==NULL);

    FOR_EACH(WindowInfo*winInfo,getAllWindows(),onWindowFocus(winInfo->id))
    assert(getSize(getActiveMasterWindowStack())==3);
    assert(getFocusedWindow()==getWindowInfo(1));

    //head is 1
    setFocusStackFrozen(1);
    onWindowFocus(getWindowInfo(2)->id);

    assert(getFocusedWindow()==getWindowInfo(2));
    removeWindowFromMaster(getActiveMaster(), getFocusedWindow()->id);
    assert(getFocusedWindow()==getWindowInfo(3));
    removeWindowFromMaster(getActiveMaster(), getFocusedWindow()->id);
    assert(getFocusedWindow()==getWindowInfo(1));
    removeWindowFromMaster(getActiveMaster(), getFocusedWindow()->id);
    assert(getFocusedWindow()==0);
    assert(getSize(getActiveMasterWindowStack())==0);
}END_TEST

START_TEST(test_add_remove_master_window_cache){
    addFakeMaster(1,1);
    assert(getWindowCache());
    testAddUnique(getWindowCache(),
            updateWindowCache);

    assert(getSize(getWindowCache()));
    clearWindowCache();
    assert(getSize(getWindowCache())==0);
}END_TEST

START_TEST(test_master_active){
    assert(getActiveMaster()==NULL);
    addFakeMaster(1,3);
    assert(getActiveMasterKeyboardID()==1);
    assert(getActiveMasterPointerID()==3);
    assert(getActiveMaster());
    for(int i=2;i<=size;i++){
        assert(addFakeMaster(i,i));
        //non default masters are not auto set to active
        assert(getActiveMaster()->id!=i);
        setActiveMaster(getMasterById(i));
        Master*master=find(getAllMasters(), &i,sizeof(int));
        assert(master==getActiveMaster());
    }
}END_TEST
START_TEST(test_master_active_remove){
    assert(getActiveMaster()==NULL);
    addFakeMaster(1,2);
    addFakeMaster(3,4);
    removeMaster(1);
    assert(getActiveMaster()==getMasterById(3));
    assert(getActiveMasterKeyboardID()==3);
    assert(getActiveMasterPointerID()==4);
}END_TEST
START_TEST(test_create_window_info){
    WindowInfo*winInfo=createWindowInfo(1);
    assert(winInfo->id==1);
    assert(!isNotEmpty(getClonesOfWindow(winInfo)));
    addWindowInfo(winInfo);
}END_TEST

START_TEST(test_window_add_remove){
    testWindowAddRemove(getAllWindows(),addWindowInfo,removeWindow);
    testAddUnique(getAllWindows(),addWindowInfo);
}END_TEST


START_TEST(test_window_workspace_add_remove){
    int fakeAdd(WindowInfo*winInfo){
        addWindowInfo(winInfo);
        return addWindowToWorkspace(winInfo, 1);
    }
    testWindowAddRemove(getAllWindows(),fakeAdd,removeWindow);
    testAddUnique(getAllWindows(),fakeAdd);
}END_TEST

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
    for(int i=0;i<getNumberOfWorkspaces();i++){
        Workspace*workspace=getWorkspaceByIndex(i);
        assert(workspace->id==i);
        assert(getSize(getWindowStack(workspace))==0);
        assert(workspace->monitor==NULL);
        assert(workspace->activeLayout==NULL);
        assert(!isNotEmpty(&workspace->layouts));
        assert(!isWorkspaceVisible(i));
        assert(!isWorkspaceNotEmpty(i));
    }
}END_TEST


START_TEST(test_visible_workspace){
    addFakeMaster(1,1);
    assert(isWorkspaceVisible(getActiveWorkspaceIndex())==0);
    assert(isWorkspaceVisible(!getActiveWorkspaceIndex())==0);
    addFakeMonitor(1);
    assert(isWorkspaceVisible(getActiveWorkspaceIndex()));
    assert(isWorkspaceVisible(!getActiveWorkspaceIndex())==0);
    swapMonitors(0, 1);
    assert(isWorkspaceVisible(getActiveWorkspaceIndex())==0);
    assert(isWorkspaceVisible(!getActiveWorkspaceIndex()));
    addFakeMonitor(2);
    assert(isWorkspaceVisible(getActiveWorkspaceIndex()));
    assert(isWorkspaceVisible(!getActiveWorkspaceIndex()));
}END_TEST
START_TEST(test_empty_workspace){
    addFakeMaster(1,1);
    assert(isWorkspaceNotEmpty(0)==0);
    addWindowInfo(createWindowInfo(1));
    assert(isWorkspaceNotEmpty(0)==0);
    addWindowToWorkspace(getWindowInfo(1), 0);
    assert(isWorkspaceNotEmpty(0));
    removeWindow(1);
    assert(isWorkspaceNotEmpty(0)==0);
}END_TEST
START_TEST(test_next_workspace){
    int size=4;
    NUMBER_OF_WORKSPACES=size;
    resetContext();
    assert(getNumberOfWorkspaces()==size);
    addFakeMaster(1,1);
    for(int i=0;i<2;i++){
        //will take the first n workspaces
        addFakeMonitor(i);
        assert(isWorkspaceVisible(i));
        addWindowInfo(createWindowInfo(i+1));
        addWindowToWorkspace(getWindowInfo(i+1), i*2);
        assert(getSize(getWindowStack(getWorkspaceByIndex(i*2)))==1);

    }

    int target[][4]={
            {1,2,3,0},//any -1,-1
            {1,0,1,0},//visible -1,0
            {2,3,2,3},//hidden  -1,1
            {2,0,2,0},//non empty 0,-1
            {0,0,0,0},//nonempty,visible 0,0
            {2,2,2,2},//hidden nonempty 0,1
            {1,3,1,3},//empty   1,-1
            {1,1,1,1},//visible,empty   1,0
            {3,3,3,3},//hidden empty 1,1
    };
    for(int e=-1,count=0;e<2;e++)
        for(int h=-1;h<2;h++,count++){

            setActiveWorkspaceIndex(0);
            for(int n=0;n<size;n++){
                int mask=((e+1)<<2)+(h+1);
                Workspace*w=getNextWorkspace(1, mask);
                assert(w==getNextWorkspace(1, mask));
                assert(w);
                assert(w->id==target[count][n]);
                setActiveWorkspaceIndex(w->id);
            }
    }

    setActiveWorkspaceIndex(0);
    for(int i=-getSize(getAllWindows())*2;i<=getSize(getAllWindows())*2;i++){
        Workspace*w=getNextWorkspace(i, 0);
        assert(w->id==(getActiveWorkspaceIndex()+i+getNumberOfWorkspaces())%getNumberOfWorkspaces());
    }

}END_TEST

START_TEST(test_window_cycle){
    addFakeMaster(1,1);
    for(int i=1;i<=size;i++){
        WindowInfo*winInfo=createWindowInfo(i);
        assert(addWindowInfo(winInfo));
        addWindowToWorkspace(winInfo, 0);
    }
    ArrayList*node=getWindowStack(getWorkspaceByIndex(0));

    int currentFocusedWindow=((WindowInfo*)getHead(node))->id;
    onWindowFocus(currentFocusedWindow);
    assert(getFocusedWindow());
    FOR_EACH(WindowInfo*winInfo,node,
        int size=getSize(node);
        onWindowFocus(winInfo->id);
        assert(getFocusedWindow()==winInfo);
        assert(size==getSize(node));
    )
}END_TEST
START_TEST(test_active){
    addFakeMaster(1,1);
    assert(getActiveWorkspaceIndex()==0);
    assert(getActiveLayout()==0);
    void*dummy=(void*)1;
    setActiveLayout(dummy);
    assert(getActiveLayout()==dummy);
    assert(getActiveWorkspace()->id==getActiveWorkspaceIndex());
    assert(getSize(getActiveMasterWindowStack())==0);
    assert(isNotEmpty(getActiveMasterWindowStack())==0);

}END_TEST
START_TEST(test_workspace_window_add_remove){
    addFakeMaster(1,1);


    int remove(unsigned int win){
       WindowInfo*winInfo=getWindowInfo(win);
       int i= removeWindowFromWorkspace(winInfo);
       return i;
    }
    int add(WindowInfo* winInfo){
       int i= addWindowToWorkspace(winInfo, getActiveWorkspaceIndex());
       assert(getActiveWindowStack()==getWindowStackOfWindow(winInfo));
       return i;
    }

    testWindowAddRemove(getActiveWindowStack(),add,remove);

    
}END_TEST


START_TEST(test_monitor_workspace){
    addFakeMaster(1,1);
    addFakeMonitor(1);
    addFakeMonitor(2);
    FOR_EACH(Monitor*monitor,getAllMonitors(),assert(getWorkspaceFromMonitor(monitor)!=NULL))
    assert(getWorkspaceFromMonitor((void*)-1)==NULL);

    int count=0;
    for(int i=0;i<size;i++){
        if(isWorkspaceVisible(i)){
            count++;
            Monitor*m;
            m=getMonitorFromWorkspace(getWorkspaceByIndex(i));
            assert(m);
            Workspace*w=getWorkspaceFromMonitor(m);
            assert(w);
            assert(w->id==i);
        }
    }
    assert(count==2);

}END_TEST

START_TEST(test_window_in_visible_worspace){
    addFakeMaster(1,1);
    WindowInfo*winInfo=createWindowInfo(1);
    addWindowInfo(winInfo);
    assert(!isWindowInVisibleWorkspace(winInfo));
    addFakeMonitor(1);
    addWindowToWorkspace(winInfo, 0);
    assert(isWindowInVisibleWorkspace(winInfo));
    swapMonitors(0, 1);
    assert(!isWindowInVisibleWorkspace(winInfo));
    addWindowToWorkspace(winInfo, 1);
    assert(isWindowInVisibleWorkspace(winInfo));


}END_TEST
START_TEST(test_complete_window_remove){
    addWindowInfo(createWindowInfo(1));
    for(int i=1;i<=getNumberOfWorkspaces();i++){
        addFakeMaster(i,i);
        onWindowFocus(1);
    }
    addWindowToWorkspace(getWindowInfo(1), 0);
    assert(removeWindow(1));
    FOR_EACH(Master*m,getAllMasters(),assert(getSize(getWindowStackByMaster(m))==0);)
    for(int i=0;i<getNumberOfWorkspaces();i++){
        assert(getSize(getWindowStack(getWorkspaceByIndex(i)))==0);
    }

}END_TEST



Suite *mywmUtilSuite(void) {
    Suite*s = suite_create("Context");

    TCase*tc_core;

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

    tcase_add_loop_test(tc_core, test_master_focus_stack,0,2);

    tcase_add_test(tc_core, test_master_active);
    tcase_add_test(tc_core, test_master_active_remove);
    tcase_add_test(tc_core, test_focus_delete);
    tcase_add_test(tc_core, test_last_master_to_focus_window);
    //tcase_add_test(tc_core, test_set_last_window_clicked);

    tcase_add_test(tc_core, test_master_stack_add_remove);

    suite_add_tcase(s, tc_core);


    tc_core = tcase_create("Workspace");
    tcase_add_checked_fixture(tc_core, createSimpleContext, resetContext);
    tcase_add_test(tc_core, test_active);
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

    return s;
}
