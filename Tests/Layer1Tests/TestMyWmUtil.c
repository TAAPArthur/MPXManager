#include "../../mywm-util.h"
#include "../../mywm-util-private.h"
#include "../../util.h"
#include "../UnitTests.h"

int addWindow(unsigned int id){
    assert(id!=0);
    return addWindowInfo(createWindowInfo(id));
}
void testAddUnique(Node*head,int(*add)(unsigned int)){
    assert(getSize(head)==0);
    for(int i=2;i<=size*2;i++){
        add(i/2);
        assert(getSize(head)==i/2);
        assert(isInList(head, i/2));
    }
}
void testWindowAddRemove(Node*head,
        int(*add)(unsigned int),
        int(*remove)(unsigned int)){

    assert(head!=NULL);
    assert(getSize(head)==0);
    assert(remove(1)==0);
    assert(getSize(head)==0);
    int id=1;
    add(id);

    assert(getSize(head)==1);
    assert(getValue(head)!=NULL);
    assert(getIntValue(head)==id);
    add(id);
    assert(getSize(head)==1);
    remove(2);
    assert(getSize(head)==1);
    remove(1);
    assert(getSize(head)==0);

    for(int i=1;i<=size;i++){

        add(i);
        assert(head!=NULL);
        assert(getValue(head)!=NULL);
        assert(getSize(head)==i);
    }
    for(int i=1;i<=size;i++){
        remove(i);
        assert(getSize(head)==size-i);
        remove(i);
        assert(getSize(head)==size-i);
    }
    remove(1);
    remove(1);
}

START_TEST(test_get_time){
    int time=getTime();
    for(int i=0;i<100;i++){
        assert(time<=getTime());
        time=getTime();
    }
}END_TEST

START_TEST(test_init_context){
    createContext(size);
    assert(getAllWindows()->value==NULL);
    assert(getAllDocks()->value==NULL);
    assert(getAllMasters()->value==NULL);
    assert(getNumberOfWorkspaces()==size);
    for(int i=0;i<size;i++)
        assert(getWorkspaceByIndex(i));
    assert(getAllMonitors()->value==NULL);
}END_TEST

START_TEST(test_init_state){
    createContext(size);
    assert(getActiveMaster());
    assert(getSize(getAllWindows()));
    assert(getSize(getActiveWindowStack())==0);
    assert(getActiveWorkspaceIndex()==0);
    assert(getSize(getMasterWindowStack()));
    assert(getFocusedWindowNode()==getMasterWindowStack());

}END_TEST

START_TEST(test_destroy_context){
    //should not error if no context exists
    destroyContext();
    createContext(10);
    addMaster(1,1);
    addWindow(1);
    addDock(createWindowInfo(2));
    addMonitor(1, 1, (short[]){1,1,1,1});
    destroyContext();

    //actual checking of destroy is handled in mem test
}END_TEST

START_TEST(test_set_event){
    createContext(1);
    void *e;
    setLastEvent(e);
    assert(getLastEvent()==e);
}END_TEST

START_TEST(test_create_master){
    Master*m=createMaster(1,2);
    assert(m->id==1);
    assert(m->pointerId==2);
    assert(m->windowStack);
    assert(m->workspaceHistory);
    assert(m->focusedWindow);
}END_TEST

START_TEST(test_master_add_remove){
    createContext(1);
    assert(getSize(getAllMasters())==0);

    int fakeAddMaster(unsigned int id){
        return addMaster(id,id);
    }
    testWindowAddRemove(getAllMasters(),
            fakeAddMaster, removeMaster);
    testAddUnique(getAllMasters(),
            fakeAddMaster);

}END_TEST

START_TEST(test_master_stack_add_remove){
    createContext(1);
    assert(getSize(getAllMasters())==0);

    int fakeOnWindowFocus(unsigned int i){
        addWindow(i);
        int size=getSize(getMasterWindowStack());
        onWindowFocus(i);
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

START_TEST(test_set_last_window_clicked){
    createContext(1);
    addMaster(1,1);
    assert(getSize(getMasterWindowStack())==0);
    //does not have to be window in context
    int lastWindowClicked=1;
    setLastWindowClicked(lastWindowClicked);
    assert(getSize(getMasterWindowStack())==0);
    addWindow(1);
    addWindow(2);
    onWindowFocus(1);
    assert(getLastWindowClicked()==lastWindowClicked);
    onWindowFocus(2);
    assert(getLastWindowClicked()==lastWindowClicked);


}END_TEST

START_TEST(test_master_focus_stack_toggle){
    createContext(1);
    addMaster(1,1);
    //default value should be 0
    assert(isFocusStackFrozen()==0);
    addWindow(1);
    onWindowFocus(1);
    addWindow(2);
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
    createContext(1);
    addMaster(1,1);
    //default value should be 0
    assert(isFocusStackFrozen()==0);
    setFocusStackFrozen(frozen);
    assert(isFocusStackFrozen()==frozen);

    for(int i=1;i<=size;i++){
        if(i==size){
            Node*window=getMasterWindowStack();
            setActiveMaster(createMaster(2,3));
            setFocusStackFrozen(frozen);
            FOR_EACH(window,
                    onWindowFocus(getIntValue(window)))
        }
        assert(addWindow(i));
        onWindowFocus(i);
        Node*window=getMasterWindowStack();
        int count=0;
        if(frozen)
            assert(getSize(getMasterWindowStack())==0);
        else
            assert(getSize(getMasterWindowStack())==getSize(getAllWindows()));
        int numberOfWindows=getSize(getMasterWindowStack());
        FOR_EACH(window,
                Node*temp=window->next;
            count++;
            int originalHeadValue=getIntValue(window);
            onWindowFocus(getIntValue(window));
            if(frozen)
                assert(originalHeadValue==getIntValue(getAllWindows()));
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
    createContext(1);
    addMaster(1,1);
    for(int i=1;i<=size;i++){
        assert(addWindowInfo(createWindowInfo(i)));
        onWindowFocus(i);
    }
    setFocusStackFrozen(1);
    int currentFocusedWindow=getFocusedWindow()->id;
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
    createContext(1);
    addWindow(1);
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
    createContext(1);
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
        int nextWindow=getIntValue(getNextWindowInFocusStack(1));
        removeWindowFromMaster(getActiveMaster(), getFocusedWindow()->id);

        if(getFocusedWindow()){
            assert(getFocusedWindow()->id==nextWindow);
        }

        assert(getMasterWindowStack()==getFocusedWindowNode());
    }
    assert(getSize(getMasterWindowStack())==0);
}END_TEST

START_TEST(test_add_remove_master_window_cache){
    createContext(1);
    addMaster(1,1);
    int fakeUpdateWindowCache(unsigned int i){
        return updateWindowCache(createWindowInfo(i));
    }
    assert(getWindowCache());
    testAddUnique(getWindowCache(),
            fakeUpdateWindowCache);

    assert(getSize(getWindowCache()));
    clearWindowCache();
    assert(getSize(getWindowCache())==0);
}END_TEST

START_TEST(test_master_active){
    createContext(1);
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

START_TEST(test_create_window_info){
    WindowInfo*winInfo=createWindowInfo(1);
    assert(winInfo->id==1);
    assert(winInfo->workspaceIndex==NO_WORKSPACE);
}END_TEST

START_TEST(test_window_add_remove){
    createContext(1);
    testWindowAddRemove(getAllWindows(),addWindow,removeWindow);
    testAddUnique(getAllWindows(),addWindow);
}END_TEST
START_TEST(test_dock_add_remove){

    int addEmptyDock(unsigned int id){
        return addDock(createWindowInfo(id));
    }
    createContext(1);
    testWindowAddRemove(getAllDocks(),addEmptyDock,removeWindow);
    testAddUnique(getAllDocks(),addEmptyDock);
}END_TEST

START_TEST(test_window_workspace_add_remove){
    createContext(2);
    int fakeAdd(unsigned int id){
        addWindow(id);
        return addWindowToWorkspace(getWindowInfo(id), 1);
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
    createContext(10);
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

    createContext(size-1);
    addMaster(1,1);

    int func(unsigned int id){
        return addMonitor(id, 1, (short[]){0, 0, 100, 100});
    }

    testWindowAddRemove(getAllMonitors(),func,removeMonitor);
    for(int i=0;i<getNumberOfWorkspaces();i++)
        assert(getWorkspaceByIndex(i)->monitor==NULL);
    testAddUnique(getAllMonitors(),func);
    while(isNotEmpty(getAllMonitors()))
        removeMonitor(getIntValue(getAllMonitors()));
    for(int i=0;i<getNumberOfWorkspaces();i++)
        assert(getWorkspaceByIndex(i)->monitor==NULL);
}END_TEST
/*
START_TEST(test_avoid_struct){
    int dim=100;
    short arr[4][4]={
            {0, dim, dim,dim},//left
            {dim*2, dim, dim,dim},//right
            {dim*2, 0, dim,dim},//top
            {dim, dim*2, dim,dim},//botom
    };
    for(int i=0;i<4;i++)
        addMonitor(i+1, 0, arr[i]);
    WindowInfo*info=createWindowInfo(1);
    int properties[12];
    for(int i=0;i<4;i++){
        properties[i]=10;
        properties[i*2+4]=0;
        properties[i*2+4+1]=dim;
    }
    setDockArea(info, 12, properties);

}
*/
START_TEST(test_avoid_struct){
    createContext(1);
    addMaster(1,1);
    int dim=100;
    int dockSize=10;
    addMonitor(2, 0, (short[]){dim, 0, dim,dim});
    addMonitor(1, 1, (short[]){0, 0, dim,dim});

    int x[]={dockSize,0,0,0};
    int y[]={0,0,dockSize,0};
    int partialStruct=!_i;
    int arrSize=_i==0?12:4;
    Monitor* monitor=getValue(getAllMonitors());
    Monitor* sideMonitor=getValue(getAllMonitors()->next);
    assert(monitor!=NULL);
    assert(sideMonitor!=NULL);
    assert(sideMonitor->name==2);
    assert(monitor->name==1);

    short arr[4][4]={
            {0,0,dockSize,monitor->height},//left
            {monitor->width-dockSize,0,dockSize,monitor->height},
            {0,0,monitor->width,dockSize},//top
            {0,monitor->height-dockSize,monitor->width,dockSize}
    };

    for(int i=0;i<4;i++){
        WindowInfo*info=createWindowInfo(i+1);
        info->onlyOnPrimary=1;
        int properties[12]={};

        properties[i]=dockSize;
        properties[i*2+4]=0;
        properties[i*2+4+1]=dim;

        setDockArea(info, arrSize, properties);

        assert(addDock(info));
        assert(intersects(arr[i], &monitor->x));
        assert(intersects(arr[i], &monitor->viewX)==0);

        assert(!intersects(arr[i], &sideMonitor->x));
        for(int n=0;n<4;n++)
            assert((&sideMonitor->x)[n]==(&sideMonitor->viewX)[n]);


        removeWindow(info->id);
        assert(intersects(arr[i], &monitor->viewX));
    }
    for(int c=0;c<3;c++){
        WindowInfo*info=createWindowInfo(c+1);
        info->onlyOnPrimary=1;
        int properties[12];
        for(int i=0;i<4;i++){
            properties[i]=dockSize-c/2;
            properties[i*2+4]=0;
            properties[i*2+4+1]=dim;
        }
        setDockArea(info, arrSize, properties);
        assert(addDock(info));
    }
    assert(monitor->viewWidth*monitor->viewHeight == (dim-dockSize*2)*(dim-dockSize*2));
    assert(sideMonitor->viewWidth*sideMonitor->viewHeight == dim*dim);

}END_TEST

START_TEST(test_intersection){
    createContext(1);
    addMaster(1,1);
    int dimX=100;
    int dimY=200;
    int offsetX=10;
    int offsetY=20;
    short rect[]={offsetX, offsetY, dimX, dimY};

#define intersects(arr,x,y,w,h) intersects(arr,(short int *)(short int[]){x,y,w,h})
    //easy to see if fail
    assert(!intersects(rect, 0,0,offsetX,offsetY));
    assert(!intersects(rect, 0,offsetY,offsetX,offsetY));
    assert(!intersects(rect, offsetX,0,offsetX,offsetY));
    for(int x=0;x<dimX+offsetX*2;x++){
        assert(intersects(rect, x, 0, offsetX, offsetY)==0);
        assert(intersects(rect, x, offsetY+dimY, offsetX, offsetY)==0);
    }
    for(int y=0;y<dimY+offsetY*2;y++){
        assert(intersects(rect, 0, y, offsetX, offsetY)==0);
        assert(intersects(rect, offsetX+dimX,y, offsetX, offsetY)==0);
    }
    assert(intersects(rect, 0,offsetY, offsetX+1, offsetY));
    assert(intersects(rect, offsetX+1,offsetY, 0, offsetY));
    assert(intersects(rect, offsetX+1,offsetY, dimX, offsetY));

    assert(intersects(rect, offsetX,0, offsetX, offsetY+1));
    assert(intersects(rect, offsetX,offsetY+1, offsetX, 0));
    assert(intersects(rect, offsetX,offsetY+1, offsetX, dimY));

    assert(intersects(rect, offsetX+dimX/2,offsetY+dimY/2, 0, 0));

}END_TEST

START_TEST(test_init_workspace){
    createContext(1);
    Workspace*workspace=getWorkspaceByIndex(0);
    assert(workspace->id==0);
    for(int n=0;n<NUMBER_OF_LAYERS;n++){
        assert(getIntValue(workspace[0].windows[n])==0);
        assert(getValue(workspace[0].windows[n])==0);
        assert(getSize(workspace[0].windows[n])==0);
        assert(workspace[0].monitor==NULL);
        assert(!isWorkspaceVisible(0));
        assert(!isWorkspaceNotEmpty(0));
        assert(workspace[0].windows[n]!=NULL);
    }
    assert(getSize(workspace[0].windows[DEFAULT_LAYER])==0);
}END_TEST
//TODO! fix broken test
START_TEST(test_window_layers){
    createContext(size);
    addMaster(1,1);
    WindowInfo*info=createWindowInfo(1);
    addWindowInfo(info);

    for(int i=0;i<NUMBER_OF_LAYERS;i++){
        removeWindowFromWorkspace(info, 0);
        addWindowToWorkspaceAtLayer(info, 0, i);
        assert(getSize(getActiveWindowStack()) == (i== DEFAULT_LAYER));
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
    createContext(2);
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
    createContext(1);
    addMaster(1,1);
    assert(isWorkspaceNotEmpty(0)==0);
    addWindow(1);
    assert(isWorkspaceNotEmpty(0)==0);
    addWindowToWorkspace(getWindowInfo(1), 0);
    assert(isWorkspaceNotEmpty(0));
    removeWindow(1);
    assert(isWorkspaceNotEmpty(0)==0);
}END_TEST
START_TEST(test_next_workspace){
    createContext(4);
    addMaster(1,1);
    for(int i=0;i<2;i++){
        //will take the first n workspaces
        addMonitor(i, 1, (short[]){i,i,1,1});
        assert(isWorkspaceVisible(i));
        addWindow(i+1);
        addWindowToWorkspace(getWindowInfo(i+1), i*2);
        assert(getSize(getWindowStack(getWorkspaceByIndex(i*2)))==1);

    }

    int size=4;
    int hiddenNonEmptyIndex=5;
    int hiddenIndex=2;
    const int numberOfTargets=9;
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
                Workspace*w=getNextWorkspace(1, e,h);
                assert(w==getNextWorkspace(1, e, h));
                assert(w);
                if(count==hiddenIndex)
                    assert(w==getNextHiddenWorkspace());
                else if (count==hiddenNonEmptyIndex)
                    assert(w==getNextHiddenNonEmptyWorkspace());

                assert(w->id==target[count][n]);
                setActiveWorkspaceIndex(w->id);
            }
    }

    setActiveWorkspaceIndex(0);
    for(int i=-getSize(getAllWindows())*2;i<=getSize(getAllWindows())*2;i++){
        Workspace*w=getNextWorkspace(i, -1, -1);
        assert(w->id==(getActiveWorkspaceIndex()+i+getNumberOfWorkspaces())%getNumberOfWorkspaces());
    }

}END_TEST

START_TEST(test_window_cycle){
    createContext(1);
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
    for(int dir=-1;dir<=1;dir++){
        for(int i=1;i<=size;i++){
            //should not change anything
            assert(getNextWindowInStack(dir)==getNextWindowInStack(dir));
            onWindowFocus(getIntValue(getNextWindowInStack(dir)));
        }
        assert(currentFocusedWindow==getIntValue(node));
    }
}END_TEST
START_TEST(test_active){
    createContext(1);
    addMaster(1,1);
    assert(getActiveWorkspaceIndex()==0);
    assert(getActiveWorkspace()->id==getActiveWorkspaceIndex());
    assert(getMasterWindowStack()!=NULL);
    assert(getMasterWindowStack()->value==NULL);
    assert(getIntValue(getMasterWindowStack())==0);
    assert(getSize(getMasterWindowStack())==0);
    assert(isNotEmpty(getMasterWindowStack())==0);
    assert(getActiveWorkspaceIndex()==DEFAULT_WORKSPACE_INDEX);

}END_TEST
START_TEST(test_workspace_window_add_remove){
    createContext(1);
    addMaster(1,1);
    for(int i=1;i<=size;i++){
        addWindow(i);
    }
    assert(getActiveWorkspaceIndex()==DEFAULT_WORKSPACE_INDEX);
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
START_TEST(test_default_layouts){
    createContext(size);
    for(int i=0;i<getNumberOfWorkspaces();i++){
        assert(getSize(getWorkspaceByIndex(i)->layouts)==NUMBER_OF_DEFAULT_LAYOUTS);
    }
}END_TEST

START_TEST(test_monitor_workspace){
    createContext(size);
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
    createContext(2);
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
    createContext(size);
    addWindow(1);
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


Suite *mywmUtilSuite(void) {
    Suite*s = suite_create("Context");

    TCase*tc_core = tcase_create("MISC");
    tcase_add_test(tc_core,test_get_time);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Init");
    tcase_add_test(tc_core, test_create_window_info);
    tcase_add_test(tc_core, test_init_workspace);
    tcase_add_test(tc_core, test_create_master);
    tcase_add_test(tc_core, test_init_context);
    tcase_add_test(tc_core,test_init_state);

    tc_core = tcase_create("Context");
    tcase_add_test(tc_core, test_set_event);
    tcase_add_test(tc_core, test_destroy_context);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Window");
    tcase_add_test(tc_core, test_window_add_remove);
    tcase_add_test(tc_core, test_dock_add_remove);

    //tcase_add_test(tc_core, test_window_mask);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Master");
    tcase_add_loop_test(tc_core, test_master_focus_stack,0,2);
    tcase_add_test(tc_core, test_master_add_remove);
    tcase_add_test(tc_core, test_add_remove_master_window_cache);
    tcase_add_test(tc_core, test_master_active);
    tcase_add_test(tc_core, test_focus_delete);
    tcase_add_test(tc_core, test_focus_cycle);
    tcase_add_test(tc_core, test_master_focus_stack_toggle);
    tcase_add_test(tc_core, test_last_master_to_focus_window);
    tcase_add_test(tc_core, test_set_last_window_clicked);

    tcase_add_test(tc_core, test_master_stack_add_remove);
    suite_add_tcase(s, tc_core);


    tc_core = tcase_create("Workspace");

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
    tcase_add_test(tc_core, test_default_layouts);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Monitor");
    tcase_add_test(tc_core, test_monitor_init);
    tcase_add_test(tc_core,test_monitor_add_remove);
    tcase_add_loop_test(tc_core, test_avoid_struct,0,2);
    tcase_add_test(tc_core, test_intersection);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Combinations");
    tcase_add_test(tc_core, test_complete_window_remove);
    tcase_add_test(tc_core, test_monitor_workspace);
    tcase_add_test(tc_core, test_window_in_visible_worspace);

    suite_add_tcase(s, tc_core);

    return s;
}
