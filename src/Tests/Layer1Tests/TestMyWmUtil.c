#include "../UnitTests.h"
#include "../../mywm-util.h"

extern int size;
//int size=10;

#define CREATE_WIN_INFO_ARR WindowInfo* arr[size]; \
for(int i=0;i<size;i++){ \
    int value=i+1; \
    arr[i]=getWindowInfo(value); \
    if(!arr[i])arr[i]=createWindowInfo(i+1); \
    if(head!=getAllWindows()) \
        addWindowInfo(arr[i]); \
}


void testAddUnique(Node*head,
        int(*add)(WindowInfo*)){
    assert(getSize(head)==0);
    assert(add);

    CREATE_WIN_INFO_ARR

    for(int i=2;i<=size*2;i++){
        add(arr[i/2-1]);
        assert(getSize(head)==i/2);
        assert(isInList(head, i/2));
    }
}
void testWindowAddRemove(Node*head,
        int(*add)(WindowInfo*),
        int(*remove)(unsigned int)){

    assert(head!=NULL);
    assert(getSize(head)==0);

    CREATE_WIN_INFO_ARR

    for(int i=1;i<=size;i++){
        add(arr[i-1]);
        assert(getSize(head)==i);
        assert(getIntValue(head)==i);
    }
    for(int i=1;i<=size;i++){
        remove(i);
        assert(getSize(head)==size-i);
        remove(i);
    }
}

START_TEST(test_get_time){
    unsigned int time=getTime();
    for(int i=0;i<100;i++){
        assert(time<=getTime());
        time=getTime();
    }
}END_TEST

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
    destroyContext();
    createContext(10);
    addMaster(1,1);
    WindowInfo*winInfo=createWindowInfo(1);
    addWindowInfo(winInfo);
    char*str="dummy";
    winInfo->typeName=calloc(strlen(str)+1, sizeof(char));
    memcpy(winInfo->typeName, str, strlen(str)*sizeof(char));
    addMonitor(1, 1, (short[]){1,1,1,1});
    destroyContext();
}END_TEST


START_TEST(test_create_master){
    addMaster(1, 2);
    Master*m=getMasterById(1);
    assert(getSize(getAllMasters())==1);
    assert(m->id==1);
    assert(m->pointerId==2);
    assert(m->windowStack);
    assert(m->workspaceHistory);
    assert(m->focusedWindow);
    assert(getActiveMaster()==m);
    assert(!getSize(getMasterWindowStack()));
    assert(getFocusedWindowNode()==getMasterWindowStack());
    assert(getActiveWorkspaceIndex()==0);

}END_TEST

START_TEST(test_master_add_remove){
    assert(getSize(getAllMasters())==0);
    for(int n=0;n<2;n++){
        for(int i=1;i<=size;i++)
            addMaster(i,i);
        assert(getSize(getAllMasters())==size);
    }
    for(int i=1;i<=size;i++)
        assert(removeMaster(i));
    assert(!removeMaster(0));
    assert(getActiveMaster()==NULL);

}END_TEST

START_TEST(test_master_stack_add_remove){
    assert(getSize(getAllMasters())==0);

    int fakeOnWindowFocus(WindowInfo*winInfo){

        addWindowInfo(winInfo);
        int size=getSize(getMasterWindowStack());
        onWindowFocus(winInfo->id);
        return size!=getSize(getMasterWindowStack());
    }
    int fakeRemove(unsigned int i){
        return removeWindowFromMaster(getActiveMaster(),i);
    }
    for(int i=1;i<=size;i++){
        addMaster(i,i);
        setActiveMaster(getMasterById(i));
        assert(getIntValue(getAllMasters())==i);
        testWindowAddRemove(getMasterWindowStack(), fakeOnWindowFocus, fakeRemove);
        testAddUnique(getMasterWindowStack(),
                fakeOnWindowFocus);
    }
    Node*head=getAllMasters();
    FOR_EACH(head,
        Master*m=getValue(head);
        assert(getSize(m->windowStack)==size);
    )

}END_TEST
/*
START_TEST(test_set_last_window_clicked){
    addMaster(1,1);
    assert(getSize(getMasterWindowStack())==0);
    //does not have to be window in context
    int lastWindowClicked=1;
    setLastWindowClicked(lastWindowClicked);
    assert(getSize(getMasterWindowStack())==0);
    addWindowInfo(createWindowInfo(1));
    addWindowInfo(createWindowInfo(2));
    onWindowFocus(1);
    assert(getLastWindowClicked()==lastWindowClicked);
    onWindowFocus(2);
    assert(getLastWindowClicked()==lastWindowClicked);
}END_TEST
*/
START_TEST(test_master_focus_stack_toggle){
    addMaster(1,1);
    //default value should be 0
    assert(isFocusStackFrozen()==0);
    addWindowInfo(createWindowInfo(1));
    onWindowFocus(1);
    addWindowInfo(createWindowInfo(2));
    onWindowFocus(2);
    setFocusStackFrozen(1);
    onWindowFocus(getIntValue(getNextWindowInFocusStack(1)));
    assert(getFocusedWindow()!=getValue(getMasterWindowStack()));
    setFocusStackFrozen(0);
    //should always be true when not frozen
    assert(getFocusedWindow()==getValue(getMasterWindowStack()));
}END_TEST
/**
 * Test window changes stack change or lack thereof
 * LOOP test
 */
START_TEST(test_master_focus_stack){

    int frozen=_i;
    addMaster(1,1);
    //default value should be 0
    assert(isFocusStackFrozen()==0);
    setFocusStackFrozen(frozen);
    assert(isFocusStackFrozen()==frozen);

    for(int i=1;i<=size;i++){
        if(i==size){
            Node*window=getMasterWindowStack();
            addMaster(2, 3);
            setActiveMaster(getMasterById(2));
            setFocusStackFrozen(frozen);
            FOR_EACH(window,
                    onWindowFocus(getIntValue(window)))
        }
        assert(addWindowInfo(createWindowInfo(i)));
        unsigned int time=getTime();
        onWindowFocus(i);

        assert(getTime()>=getFocusedTime(getActiveMaster()));
        assert(time<=getFocusedTime(getActiveMaster()));


        Node*window=getMasterWindowStack();
        int count=0;
        if(frozen){
            assert(getSize(getMasterWindowStack())==0);
            assert(getLastMasterToFocusWindow(i)==NULL);
        }
        else{
            assert(getLastMasterToFocusWindow(i)==getActiveMaster());
            assert(getFocusedWindow()->id==(unsigned int)i);
            assert(getSize(getMasterWindowStack())==getSize(getAllWindows()));
        }
        int numberOfWindows=getSize(getMasterWindowStack());
        FOR_EACH(window,
                Node*temp=window->next;
            count++;
            unsigned int originalHeadValue=getIntValue(window);
            onWindowFocus(getIntValue(window));
            if(frozen)
                assert(((int)originalHeadValue)==getIntValue(getAllWindows()));
            else{
                //focused window should be the set window
                assert(getFocusedWindow()->id==originalHeadValue);
                //focused window should be at top
                assert(getFocusedWindow()==getValue(getMasterWindowStack()));
                //LRU window cache should be updated
                assert(getIntValue(getMasterWindowStack())==getIntValue(getAllWindows()));
            }

            window=temp;
            if(!temp)
                break;
            else continue;
        )
        //workspace stack unchanged
        assert(numberOfWindows==getSize(getMasterWindowStack()));
        assert(count==numberOfWindows);

        if(!frozen)
            assert(getSize(getAllWindows())==numberOfWindows);
        assert(getSize(getMasterWindowStack())==numberOfWindows);
        assert(getSize(getActiveWindowStack())==0);

    }

}END_TEST
START_TEST(test_focus_cycle){
    addMaster(1,1);
    for(int i=1;i<=size;i++){
        assert(addWindowInfo(createWindowInfo(i)));
        onWindowFocus(i);
    }
    setFocusStackFrozen(1);
    unsigned int currentFocusedWindow=getFocusedWindow()->id;
    for(int dir=-1;dir<=1;dir++){
        for(int i=1;i<=size;i++){
            //should not change anything
            assert(getNextWindowInFocusStack(dir)==getNextWindowInFocusStack(dir));
            onWindowFocus(getIntValue(getNextWindowInFocusStack(dir)));
        }
        assert(currentFocusedWindow==getFocusedWindow()->id);
    }
}END_TEST
START_TEST(test_last_master_to_focus_window){
    addWindowInfo(createWindowInfo(1));
    assert(getLastMasterToFocusWindow(1)==NULL);
    for(int i=1;i<=size*2;i++){
        addMaster(i,i);
        if(i%2==0)
            continue;
        setActiveMaster(getMasterById(i));
        onWindowFocus(1);
        assert(getLastMasterToFocusWindow(1));
        assert(getLastMasterToFocusWindow(1)->id==i);
        msleep(1);
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
    addMaster(1,1);
    assert(addWindowInfo(createWindowInfo(1)));
    assert(addWindowInfo(createWindowInfo(2)));
    assert(addWindowInfo(createWindowInfo(3)));

    onWindowFocus(getIntValue(getAllWindows()));
    onWindowFocus(getIntValue(getAllWindows()->next));
    onWindowFocus(getIntValue(getAllWindows()->next->next));

    //head is 1
    setFocusStackFrozen(1);
    //focused on 3
    onWindowFocus(getIntValue(getMasterWindowStack()->next->next));
    assert(getSize(getMasterWindowStack())==3);

    removeWindowFromMaster(getActiveMaster(), getFocusedWindow()->id);
    assert(getMasterWindowStack()==getFocusedWindowNode());

    while(getFocusedWindow()){
        unsigned int nextWindow=getIntValue(getNextWindowInFocusStack(1));

        assert(isInList(getMasterWindowStack(),nextWindow));
        assert(isInList(getMasterWindowStack(),getFocusedWindow()->id));
        assert(removeWindowFromMaster(getActiveMaster(), getFocusedWindow()->id));

        if(getFocusedWindow()){
            assert(getFocusedWindow()->id==nextWindow);
        }

        assert(getMasterWindowStack()==getFocusedWindowNode());
    }
    assert(getSize(getMasterWindowStack())==0);
}END_TEST

START_TEST(test_add_remove_master_window_cache){
    addMaster(1,1);
    assert(getWindowCache());
    testAddUnique(getWindowCache(),
            updateWindowCache);

    assert(getSize(getWindowCache()));
    clearWindowCache();
    assert(getSize(getWindowCache())==0);
}END_TEST

START_TEST(test_master_active){
    assert(getActiveMaster()==NULL);
    addMaster(1,3);
    assert(getActiveMasterKeyboardID()==1);
    assert(getActiveMasterPointerID()==3);
    assert(getActiveMaster());
    for(int i=2;i<=size;i++){
        assert(addMaster(i,i));
        //non default masters are not auto set to active
        assert(getActiveMaster()->id!=i);
        setActiveMaster(getMasterById(i));
        Node*master=isInList(getAllMasters(), i);
        assert(getIntValue(master)==getActiveMaster()->id);
    }
}END_TEST
START_TEST(test_master_active_remove){
    assert(getActiveMaster()==NULL);
    addMaster(1,2);
    addMaster(3,4);
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

START_TEST(test_monitor_init){

    int count=0;
    addMaster(1,1);
    for(int x=0;x<2;x++)
        for(int y=0;y<2;y++){
            short dim[4]={x,y,size*size+y,size*2+x};
            assert(addMonitor(count, !count, dim));
            assert(isPrimary(getValue(isInList(getAllMonitors(),count)))==!count);
            Monitor *m=getValue(getAllMonitors());
            for(int i=0;i<4;i++){
                assert((&m->x)[i]==(&m->viewX)[i]);
                assert(dim[i]==(&m->x)[i]);
            }
            assert(getWorkspaceFromMonitor(m));
            assert(getWorkspaceFromMonitor(m)->id==count++);
        }
}END_TEST
START_TEST(test_monitor_add_remove){

    addMaster(1,1);

    for(int n=0;n<2;n++){
        for(int i=1;i<=size+1;i++){
            addMonitor(i, 1, (short[]){0, 0, 100, 100});
            Workspace *w=getWorkspaceFromMonitor(getValue(isInList(getAllMonitors(), i)));
            if(i>size)
                assert(!w);
            else assert(w);
        }
        assert(getSize(getAllMonitors())==size+1);
    }
    //for(int i=0;i<getNumberOfWorkspaces();i++)
        //assert(getWorkspaceByIndex(i)->monitor==NULL);
    while(isNotEmpty(getAllMonitors()))
        assert(removeMonitor(getIntValue(getAllMonitors())));
    for(int i=0;i<getNumberOfWorkspaces();i++)
        assert(getWorkspaceByIndex(i)->monitor==NULL);
    assert(!removeMonitor(0));
}END_TEST


START_TEST(test_init_workspace){
    Workspace*workspace=getWorkspaceByIndex(0);
    assert(workspace->id==0);
    for(int i=0;i<getNumberOfWorkspaces();i++){
        for(int n=0;n<NUMBER_OF_LAYERS;n++){
            assert(getIntValue(workspace[i].windows[n])==0);
            assert(getValue(workspace[i].windows[n])==0);
            assert(getSize(workspace[i].windows[n])==0);
            assert(workspace[i].monitor==NULL);
            assert(workspace[i].activeLayout==NULL);
            assert(!isNotEmpty(workspace[i].layouts));
            assert(workspace[i].layouts->next==workspace[i].layouts->prev && workspace[i].layouts->next==workspace[i].layouts);
            assert(!isWorkspaceVisible(i));
            assert(!isWorkspaceNotEmpty(i));
            assert(workspace[i].windows[n]!=NULL);
        }
        assert(getSize(workspace[i].windows[NORMAL_LAYER])==0);
        assert(workspace[i].layouts);
        assert(workspace[i].layouts->next==workspace[i].layouts);
        assert(workspace[i].layouts->next==workspace[i].layouts->prev);

    }
    assert(workspace[0].layouts!=workspace[1].layouts);
}END_TEST
//TODO! fix broken test
START_TEST(test_window_layers){
    addMaster(1,1);
    WindowInfo*info=createWindowInfo(1);
    addWindowInfo(info);

    for(int i=0;i<NUMBER_OF_LAYERS;i++){
        removeWindowFromWorkspace(info, 0);
        addWindowToWorkspaceAtLayer(info, 0, i);
        assert(getSize(getActiveWindowStack()) == (i== NORMAL_LAYER));
        assert(getSize(getWindowStackAtLayer(getActiveWorkspace(),i)));
    }

}END_TEST

/*
START_TEST(test_window_in_multiple_workspaces){
    size=10;
    createContext(size);
    addMaster(1,1);
    WindowInfo*info=createWindowInfo(1);
    addWindow(info);
    addWindow(100);

    int activeWorkspaceIndex=getActiveWorkspaceIndex();
    assert(getSize(getActiveWindowStack())==2);
    for(int n=0;n<size;n++){
        moveWindowToWorkspace(info, n);
        assert(activeWorkspaceIndex==getActiveWorkspaceIndex());
        assert(getSize(getAllWindows())==2);
        for(int i=0;i<size;i++)
            if(i!=getActiveWorkspaceIndex())
                assert(getSize(getWindowStack(getWorkspaceByIndex(i)))==0);
            else
                assert(getSize(getActiveWindowStack)==(n==0?2:1));

    }
}
*/
START_TEST(test_visible_workspace){
    addMaster(1,1);
    assert(isWorkspaceVisible(getActiveWorkspaceIndex())==0);
    assert(isWorkspaceVisible(!getActiveWorkspaceIndex())==0);
    addMonitor(1, 1, (short[]){1,1,1,1});
    assert(isWorkspaceVisible(getActiveWorkspaceIndex()));
    assert(isWorkspaceVisible(!getActiveWorkspaceIndex())==0);
    swapMonitors(0, 1);
    assert(isWorkspaceVisible(getActiveWorkspaceIndex())==0);
    assert(isWorkspaceVisible(!getActiveWorkspaceIndex()));
    addMonitor(2, 1, (short[]){1,1,1,1});
    assert(isWorkspaceVisible(getActiveWorkspaceIndex()));
    assert(isWorkspaceVisible(!getActiveWorkspaceIndex()));
}END_TEST
START_TEST(test_empty_workspace){
    addMaster(1,1);
    assert(isWorkspaceNotEmpty(0)==0);
    addWindowInfo(createWindowInfo(1));
    assert(isWorkspaceNotEmpty(0)==0);
    addWindowToWorkspace(getWindowInfo(1), 0);
    assert(isWorkspaceNotEmpty(0));
    removeWindow(1);
    assert(isWorkspaceNotEmpty(0)==0);
}END_TEST
START_TEST(test_next_workspace){
    destroyContext();
    createContext(4);
    addMaster(1,1);
    for(int i=0;i<2;i++){
        //will take the first n workspaces
        addMonitor(i, 1, (short[]){i,i,1,1});
        assert(isWorkspaceVisible(i));
        addWindowInfo(createWindowInfo(i+1));
        addWindowToWorkspace(getWindowInfo(i+1), i*2);
        assert(getSize(getWindowStack(getWorkspaceByIndex(i*2)))==1);

    }

    int size=4;
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
    addMaster(1,1);
    for(int i=1;i<=size;i++){
        WindowInfo*winInfo=createWindowInfo(i);
        assert(addWindowInfo(winInfo));
        addWindowToWorkspace(winInfo, 0);
    }
    Node*node=getWindowStack(getWorkspaceByIndex(0));

    int currentFocusedWindow=getIntValue(node);
    onWindowFocus(currentFocusedWindow);
    assert(getFocusedWindow());
    FOR_EACH(node,
        int size=getSize(node);
        onWindowFocus(getIntValue(node));
        assert(getFocusedWindow()==getValue(node));
        assert(size==getSize(node));

    )
}END_TEST
START_TEST(test_active){
    addMaster(1,1);
    assert(getActiveWorkspaceIndex()==0);
    assert(getActiveWorkspace()->id==getActiveWorkspaceIndex());
    assert(getMasterWindowStack()!=NULL);
    assert(getMasterWindowStack()->value==NULL);
    assert(getIntValue(getMasterWindowStack())==0);
    assert(getSize(getMasterWindowStack())==0);
    assert(isNotEmpty(getMasterWindowStack())==0);

}END_TEST
START_TEST(test_workspace_window_add_remove){
    addMaster(1,1);
    for(int i=1;i<=size;i++){
        addWindowInfo(createWindowInfo(i));
    }
    assert(getValue(getMasterWindowStack())==NULL);
    assert(getSize(getMasterWindowStack())==0);
/*
    testWindowAddRemove(
            getMasterWindowStack(),addWindowToActiveWorkspace,
            removeWindowFromActiveWorkspace);

    int layer = DESKTOP_LAYER;
    int addAtLayer(xcb_window_t win){
       int i= addWindowToWorkspaceAtLayer(win, getActiveWorkspaceIndex(), layer);
       assert(getActiveWorkspaceIndex()==((WindowInfo*)getValue(isInList(getAllWindows(), win)))->workspaceIndex);
       return i;
    }

    for(;layer<NUMBER_OF_LAYERS;layer++)
        testWindowAddRemove(
                getActiveWindowsAtLayer(layer),addAtLayer,
                removeWindowFromActiveWorkspace);

    */
}END_TEST


START_TEST(test_monitor_workspace){
    addMaster(1,1);
    addMonitor(1, 0, (short[]){0, 0, 1, 1});
    addMonitor(2, 0, (short[]){1, 1, 1, 1});
    Node*n=getAllMonitors();
    FOR_EACH(n,
            assert(getWorkspaceFromMonitor(getValue(n))!=NULL)
            )

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
    addMaster(1,1);
    WindowInfo*winInfo=createWindowInfo(1);
    addWindowInfo(winInfo);
    assert(!isWindowInVisibleWorkspace(winInfo));
    addMonitor(1, 1, (short[]){1,1,1,1});
    addWindowToWorkspace(winInfo, 0);
    assert(isWindowInVisibleWorkspace(winInfo));
    swapMonitors(0, 1);
    assert(!isWindowInVisibleWorkspace(winInfo));
    addWindowToWorkspace(winInfo, 1);
    assert(isWindowInVisibleWorkspace(winInfo));


}END_TEST
START_TEST(test_complete_window_remove){
    addWindowInfo(createWindowInfo(1));
    for(int i=1;i<=size;i++){
        addMaster(i,i);
        onWindowFocus(1);
        addWindowToWorkspace(getValue(getAllWindows()), i-1);
    }
    removeWindow(1);
    Node*head=getAllMasters();
    FOR_EACH(head,
            Master*m=getValue(head);
        assert(getSize(m->windowStack)==0);
    )
    for(int i=1;i<=size;i++){
        assert(getSize(getWindowStack(getWorkspaceByIndex(i-1)))==0);
    }

}END_TEST

START_TEST(test_layout_add){
    addMaster(1, 1);
    Layout* l=calloc(1, sizeof(Layout));
    for(int n=0;n<getNumberOfWorkspaces();n++){
        setActiveWorkspaceIndex(n);
        for(int i=1;i<=size;i++){
            addLayoutsToWorkspace(n, l, 1);
            assert(getSize(getActiveWorkspace()->layouts)==i);
            if(i)
                assert(getValue(getActiveWorkspace()->layouts->prev)==l);
            else
                assert(getValue(getActiveWorkspace()->layouts)==l);
        }
    }
    for(int n=0;n<getNumberOfWorkspaces();n++){
        clearLayoutsOfWorkspace(n);
    }
    free(l);
}END_TEST
START_TEST(test_layout_add_multiple){
    addMaster(1, 1);
    int size=10;
    Layout* l=calloc(size, sizeof(Layout));
    addLayoutsToWorkspace(getActiveWorkspaceIndex(), l, size);
    assert(getSize(getActiveWorkspace()->layouts)==size);
    Node*n=getActiveWorkspace()->layouts;
    int i=0;
    FOR_AT_MOST(n,size,assert(getValue(n)==&l[i++]))
    free(l);
}END_TEST
START_TEST(test_active_layout){
    addMaster(1, 1);
    Layout* l=calloc(size, sizeof(Layout));
    for(int i=0;i<size;i++){
        setActiveLayout(&l[i]);
        assert(getActiveLayout()==&l[i]);
    }
    free(l);
}END_TEST

START_TEST(test_layout_name){
    addMaster(1, 1);
    Layout* l=calloc(size, sizeof(Layout));
    getNameOfLayout(NULL);//should not crash
    char*name="test";
    l->name=name;
    assert(strcmp(getNameOfLayout(l),name)==0);
    free(l);
}END_TEST

void setup(){
    createContext(size);
}
void teardown(){
    destroyContext();
}

Suite *mywmUtilSuite(void) {
    Suite*s = suite_create("Context");

    TCase*tc_core = tcase_create("MISC");
    tcase_add_test(tc_core,test_get_time);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Init");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_init_context);
    tcase_add_test(tc_core, test_create_window_info);
    tcase_add_test(tc_core, test_create_master);
    tcase_add_test(tc_core, test_init_workspace);
    suite_add_tcase(s, tc_core);


    tc_core = tcase_create("Context");
    tcase_add_test(tc_core, test_destroy_context);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Window");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_window_add_remove);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Master");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_master_add_remove);
    tcase_add_test(tc_core, test_add_remove_master_window_cache);
    tcase_add_loop_test(tc_core, test_master_focus_stack,0,2);

    tcase_add_test(tc_core, test_master_active);
    tcase_add_test(tc_core, test_master_active_remove);
    tcase_add_test(tc_core, test_focus_delete);
    tcase_add_test(tc_core, test_focus_cycle);
    tcase_add_test(tc_core, test_master_focus_stack_toggle);
    tcase_add_test(tc_core, test_last_master_to_focus_window);
    //tcase_add_test(tc_core, test_set_last_window_clicked);

    tcase_add_test(tc_core, test_master_stack_add_remove);
    suite_add_tcase(s, tc_core);


    tc_core = tcase_create("Workspace");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_active);
    tcase_add_test(tc_core, test_workspace_window_add_remove);
    tcase_add_test(tc_core, test_window_workspace_add_remove);
    tcase_add_test(tc_core, test_window_layers);
    tcase_add_test(tc_core, test_visible_workspace);
    tcase_add_test(tc_core, test_empty_workspace);
    tcase_add_test(tc_core, test_next_workspace);
    tcase_add_test(tc_core, test_window_cycle);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Layouts");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_layout_add);
    tcase_add_test(tc_core, test_layout_add_multiple);
    tcase_add_test(tc_core, test_active_layout);
    tcase_add_test(tc_core, test_layout_name);
    suite_add_tcase(s, tc_core);


    tc_core = tcase_create("Monitor");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_monitor_init);
    tcase_add_test(tc_core,test_monitor_add_remove);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Combinations");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_complete_window_remove);
    tcase_add_test(tc_core, test_monitor_workspace);
    tcase_add_test(tc_core, test_window_in_visible_worspace);

    suite_add_tcase(s, tc_core);

    return s;
}
