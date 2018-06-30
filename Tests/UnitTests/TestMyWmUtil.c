#include "UnitTests.h"
#include "../../mywm-util.h"
#include "../../mywm-util-private.h"
#include "../../util.h"
#include "../../wmfunctions.h"

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

START_TEST(test_init_context){
    createContext(size);
    assert(getAllWindows()->value==NULL);
    assert(getAllDocks()->value==NULL);
    assert(getAllMasters()->value==NULL);
    assert(getNumberOfWorkspaces()==size);
    assert(getWorkspaceByIndex(0));
    assert(getAllMonitors()->value==NULL);
}END_TEST

START_TEST(test_set_event){
    createContext(1);
    XEvent e;
    setLastEvent(&e);
    assert(getLastEvent()==&e);

}END_TEST


START_TEST(test_create_master){
    for(int i=1;i<=size;i++)
        assert(createMaster(i)->id==i);
}END_TEST

START_TEST(test_master_add_remove){
    createContext(1);
    assert(getSize(getAllMasters())==0);

    testWindowAddRemove(getAllMasters(),
            addMaster, removeMaster);
    testAddUnique(getAllMasters(),
                addMaster);
}END_TEST

START_TEST(test_master_stack_add_remove){
    createContext(1);
    assert(getSize(getAllMasters())==0);

    int fakeOnWindowFocus(unsigned int i){
        addWindow(i);
        onWindowFocus(i);
        return 0;
    }
    for(int i=1;i<=size;i++){
        addMaster(i);
        assert(getIntValue(getAllMasters())==i);
        testAddUnique(getMasterWindowStack(),
                fakeOnWindowFocus);
    }
    Node*head=getAllMasters();
    FOR_EACH(head,
        Master*m=getValue(head);
        assert(getSize(m->windowStack)==size);
    )

}END_TEST

/**
 * Test window changes stack change or lack thereof
 */
START_TEST(test_master_focus_stack){

    for(int frozen=0;frozen<2;frozen++){
        createContext(1);
        addMaster(1);
        //default value should be 0
        assert(isFocusStackFrozen()==0);
        setFocusStackFrozen(frozen);
        assert(isFocusStackFrozen()==frozen);

        for(int i=1;i<=size;i++){

            addWindow(i);

            for(int n=1;n<=i;n++){
                Node*head=isInList(getAllWindows(), n);
                int originalHeadValue=getIntValue(head);
                onWindowFocus(getIntValue(head));
                if(frozen)
                    assert(originalHeadValue==getIntValue(getAllWindows()));
                else{
                    assert(getIntValue(getFocusedWindow())==originalHeadValue);
                    assert(getFocusedWindow()==getMasterWindowStack());
                    assert(getIntValue(getMasterWindowStack())==getIntValue(getAllWindows()));
                }
            }
        }
    }
}END_TEST


START_TEST(test_master_window_cache){
    createContext(1);
    addMaster(1);
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
    addMaster(1);
    assert(getActiveMaster());
    for(int i=1;i<=size;i++){
        addMaster(i);
        setActiveMasterNodeById(getMasterNodeById(i));
        Node*master=isInList(getAllMasters(), i);
        assert(getIntValue(master)==getActiveMaster()->id);
    }
}END_TEST

START_TEST(test_create_window_info){
    for(int i=1;i<=size;i++)
        assert(createWindowInfo(i)->id==i);
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

    int size=2;
    int count=0;

    for(int x=0;x<size;x++)
        for(int y=0;y<size;y++)
            for(int w=0;w<size;w++)
                for(int h=0;h<size;h++){
                    createContext(10);
                    addMaster(1);
                    int width=size*size;
                    int height=size*2;
                    assert(addMonitor(count++, 0, x, y, width, height));
                    Monitor *m=getValue(getAllMonitors());
                    assert(m->x == x);
                    assert(m->y == y);
                    assert(m->width == width);
                    assert(m->height == height);
                    assert(m->viewWidth == width);
                    assert(m->viewHeight == height);
                    assert(m->viewX == x);
                    assert(m->viewY == y);
                    assert(m->workspaceIndex==getActiveWorkspaceIndex());
                    assert(m==getActiveWorkspace()->monitor);
                }


}END_TEST
START_TEST(test_monitor_add_remove){

    createContext(size);
    addMaster(1);
    int func(unsigned int id){
        return addMonitor(id, 1, 0, 0, 100, 100);
    }
    testWindowAddRemove(getAllMonitors(),func,removeMonitor);
    testAddUnique(getAllMonitors(),func);
}END_TEST

START_TEST(test_avoid_struct){
    createContext(1);
    addMaster(1);
    int dim=100;
    int dockSize=10;
    addMonitor(1, 1, 0, 0, dim, dim);
    int x[]={dockSize,0,0,0};
    int y[]={0,0,dockSize,0};
    Monitor* monitor=getValue(getAllMonitors());
    assert(monitor!=NULL);

    for(int c=0;c<2;c++){
        WindowInfo*info=createWindowInfo(c+1);
        for(int i=0;i<4;i++){
            info->properties[i]=dockSize;
            info->properties[i*2+4]=0;
            info->properties[i*2+4+1]=dim;
        }
        assert(addDock(info));
    }
    assert(monitor->viewWidth*monitor->viewHeight == (dim-dockSize*2)*(dim-dockSize*2));

}END_TEST

START_TEST(test_intersection){
    createContext(1);
    addMaster(1);
    int dimX=100;
    int dimY=200;
    int offsetX=10;
    int offsetY=20;
    addMonitor(1,1,offsetX, offsetY, dimX, dimY);

#define intersects(arr,x,y,w,h) intersects(arr,(short int *)(short int[]){x,y,w,h})
    Monitor*m=getValue(getAllMonitors());
    short int rect[]={m->x,m->y,m->width,m->height};
    for(int x=0;x<dimX+offsetX*2;x++){
        assert(intersects(rect, x, 0, offsetX, offsetY)==0);
        assert(intersects(rect, x, offsetY+dimY, offsetX, offsetY)==0);
    }
    for(int y=0;y<dimY+offsetY*2;y++){
        assert(intersects(rect, 0, y, offsetX, offsetY)==0);
        assert(intersects(rect, offsetX+dimX,y, offsetX, offsetY)==0);
    }
    assert(intersects(rect, 0,offsetY, offsetX+1, offsetY)==1);
    assert(intersects(rect, offsetX+1,offsetY, 0, offsetY)==1);
    assert(intersects(rect, offsetX+1,offsetY, dimX, offsetY)==1);

    assert(intersects(rect, offsetX,0, offsetX, offsetY+1)==1);
    assert(intersects(rect, offsetX,offsetY+1, offsetX, 0)==1);
    assert(intersects(rect, offsetX,offsetY+1, offsetX, dimY)==1);


    assert(intersects(rect, offsetX+dimX/2,offsetY+dimY/2, 0, 0)==1);

}END_TEST


/*
START_TEST(test_focus_delete){
    createContext(1);
    Master*master=createMaster(1);
    addWindow(1);
    addWindow(2);
    onWindowFocus(getAllWindows()->next);
    onWindowFocus(getAllWindows()->next);
    assert(getSize(master->topWindow)==2);
    //TODO implement cycle focus
    removeWindow(getNodeValue(master->focusedWindow));
}END_TEST
*/

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
START_TEST(test_active){
    createContext(1);
    addMaster(1);
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
    addMaster(1);
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
        Node*node=getWorkspaceByIndex(i)->layouts;
        FOR_EACH_CIRCULAR(node,
                Layout* l=getValue(node);
            assert(l->name);
            assert(l->layoutFunction);
        )
    }
}END_TEST

START_TEST(test_monitor_workspace){
    createContext(size);
    addMaster(1);
    addMonitor(1, 0, 0, 0, 0, 0);
    addMonitor(2, 0, 1, 0, 0, 0);
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

START_TEST(test_complete_window_remove){
    createContext(size);
    addWindow(1);
    for(int i=1;i<=size;i++){
        addMaster(i);
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

    TCase*tc_core = tc_core = tcase_create("Context");
    tcase_add_test(tc_core, test_init_context);
    tcase_add_test(tc_core, test_set_event);

    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Window");
    tcase_add_test(tc_core, test_create_window_info);
    tcase_add_test(tc_core, test_window_add_remove);
    tcase_add_test(tc_core, test_dock_add_remove);

    //tcase_add_test(tc_core, test_window_mask);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Master");
    tcase_add_test(tc_core, test_create_master);
    tcase_add_test(tc_core, test_master_focus_stack);
    tcase_add_test(tc_core, test_master_add_remove);
    tcase_add_test(tc_core, test_master_window_cache);
    tcase_add_test(tc_core, test_master_active);
    suite_add_tcase(s, tc_core);


    tc_core = tcase_create("Workspace");
    tcase_add_test(tc_core, test_init_workspace);
    tcase_add_test(tc_core, test_active);

    tcase_add_test(tc_core, test_workspace_window_add_remove);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Layouts");
    tcase_add_test(tc_core, test_default_layouts);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Monitor");
    tcase_add_test(tc_core, test_monitor_init);
    tcase_add_test(tc_core,test_monitor_add_remove);
    tcase_add_test(tc_core, test_avoid_struct);
    tcase_add_test(tc_core, test_intersection);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Combinations");
    tcase_add_test(tc_core, test_master_stack_add_remove);

    tcase_add_test(tc_core, test_complete_window_remove);
    tcase_add_test(tc_core, test_monitor_workspace);

    suite_add_tcase(s, tc_core);

    return s;
}
