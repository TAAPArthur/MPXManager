#include "../UnitTests.h"
#include "../TestX11Helper.h"
#include "../../monitors.h"
#include "../../globals.h"

START_TEST(test_detect_monitors){
    detectMonitors();
    assert(isNotEmpty(getAllMonitors()));
    LOG(LOG_LEVEL_NONE,"Detected %d monitor\n",getSize(getAllMonitors()));
}END_TEST
START_TEST(test_dock_add_remove){
    int id=1;
    WindowInfo*winInfo=createWindowInfo(id);
    assert(addDock(winInfo));
    assert(!addDock(winInfo));
    assert(removeDock(id));
    assert(!removeDock(id));
}END_TEST
START_TEST(test_avoid_struct){
    int dim=100;
    int dockSize=10;
    addMonitor(2, 0, (short[]){dim, 0, dim,dim});
    addMonitor(1, 1, (short[]){0, 0, dim,dim});

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
        int properties[12]={0};

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


        removeDock(info->id);
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
    int dimX=100;
    int dimY=200;
    int offsetX=10;
    int offsetY=20;
    short rect[]={offsetX, offsetY, dimX, dimY};

#define _intersects(arr,x,y,w,h) intersects(arr,(short int *)(short int[]){x,y,w,h})
    //easy to see if fail
    assert(!_intersects(rect, 0,0,offsetX,offsetY));
    assert(!_intersects(rect, 0,offsetY,offsetX,offsetY));
    assert(!_intersects(rect, offsetX,0,offsetX,offsetY));
    for(int x=0;x<dimX+offsetX*2;x++){
        assert(_intersects(rect, x, 0, offsetX, offsetY)==0);
        assert(_intersects(rect, x, offsetY+dimY, offsetX, offsetY)==0);
    }
    for(int y=0;y<dimY+offsetY*2;y++){
        assert(_intersects(rect, 0, y, offsetX, offsetY)==0);
        assert(_intersects(rect, offsetX+dimX,y, offsetX, offsetY)==0);
    }
    assert(_intersects(rect, 0,offsetY, offsetX+1, offsetY));
    assert(_intersects(rect, offsetX+1,offsetY, 0, offsetY));
    assert(_intersects(rect, offsetX+1,offsetY, dimX, offsetY));

    assert(_intersects(rect, offsetX,0, offsetX, offsetY+1));
    assert(_intersects(rect, offsetX,offsetY+1, offsetX, 0));
    assert(_intersects(rect, offsetX,offsetY+1, offsetX, dimY));

    assert(_intersects(rect, offsetX+dimX/2,offsetY+dimY/2, 0, 0));

}END_TEST
START_TEST(test_fake_dock){
    WindowInfo*winInfo=createWindowInfo(1);
    addDock(winInfo);
    for(int i=0;i<LEN(winInfo->properties);i++)
        assert(winInfo->properties[i]==0);
}END_TEST
START_TEST(test_avoid_docks){
    int size=10;
    for(int i=3;i>=0;i--){
        addDock(createWindowInfo(createDock(i,size,_i)));
    }

    assert(getSize(getAllDocks())==4);
    Monitor*m=getValue(getAllMonitors());

    short arr[4][4]={
            {0,0,size,m->height},//left
            {m->width-size,0,size,m->height},
            {0,0,m->width,size},//top
            {0,m->height-size,m->width,size}
    };
    Node*n=getAllDocks();
    int index=0;
    FOR_EACH(n,
        WindowInfo*winInfo=getValue(n);

        int type=index++;
        assert(winInfo->properties[type]);
        for(int j=0;j<4;j++)
            if(j!=type)
                assert(winInfo->properties[j]==0);

    );

    for(int i=0;i<4;i++)
        assert(intersects(&m->viewX,arr[i])==0);

}END_TEST

START_TEST(test_root_dim){
    int width=0,height=0;
    Node*n=getAllMonitors();
    FOR_EACH(n,
        Monitor*m=getValue(n);
        width+=m->width;
        height+=m->height;
    )
    assert(width==getRootWidth());
    assert(height==getRootHeight());
}END_TEST
Suite *monitorsSuite() {
    Suite*s = suite_create("Monitors");

    TCase* tc_core = tcase_create("Detect monitors");
    tcase_add_checked_fixture(tc_core, createContextAndSimpleConnection, destroyContextAndConnection);
    tcase_add_test(tc_core, test_detect_monitors);
    tcase_add_test(tc_core, test_root_dim);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Docks");
    tcase_add_checked_fixture(tc_core, createContextAndSimpleConnection, destroyContextAndConnection);
    tcase_add_test(tc_core, test_intersection);
    tcase_add_loop_test(tc_core, test_avoid_struct,0,2);
    tcase_add_loop_test(tc_core, test_avoid_docks,0,2);
    tcase_add_test(tc_core,test_fake_dock);
    tcase_add_test(tc_core,test_dock_add_remove);




    suite_add_tcase(s, tc_core);
    return s;
}
