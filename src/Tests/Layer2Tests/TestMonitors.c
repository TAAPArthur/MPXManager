#include "../UnitTests.h"
#include "../TestX11Helper.h"
#include "../../monitors.h"
#include "../../globals.h"

START_TEST(test_detect_monitors){
    detectMonitors();
    int num = getSize(getAllMonitors());
    assert(isNotEmpty(getAllMonitors()));
    updateMonitor(0, 1, (Rect){
        0, 0, 100, 100
    });
    assert(getSize(getAllMonitors()) == num + 1);
    detectMonitors();
    assert(getSize(getAllMonitors()) == num);
    assert(num == 2);
}
END_TEST
START_TEST(test_avoid_struct){
    int dim = 100;
    int dockSize = 10;
    int monitorIndex = 1, sideMonitorIndex = 2;
    updateMonitor(sideMonitorIndex, 0, (Rect){
        dim, 0, dim, dim
    });
    updateMonitor(monitorIndex, 1, (Rect){
        0, 0, dim, dim
    });
    int arrSize = _i == 0 ? 12 : 4;
    Monitor* monitor = find(getAllMonitors(), &monitorIndex, sizeof(int));
    Monitor* sideMonitor = find(getAllMonitors(), &sideMonitorIndex, sizeof(int));
    assert(monitor != NULL);
    assert(sideMonitor != NULL);
    Rect arr[4] = {
        {0, 0, dockSize, monitor->base.height}, //left
        {monitor->base.width - dockSize, 0, dockSize, monitor->base.height},
        {0, 0, monitor->base.width, dockSize}, //top
        {0, monitor->base.height - dockSize, monitor->base.width, dockSize}
    };
    for(int i = 0; i < 4; i++){
        WindowInfo* info = createWindowInfo(i + 1);
        markAsDock(info);
        info->onlyOnPrimary = 1;
        int properties[12] = {0};
        properties[i] = dockSize;
        properties[i * 2 + 4] = 0;
        properties[i * 2 + 4 + 1] = dim;
        setDockArea(info, arrSize, properties);
        assert(addWindowInfo(info));
        assert(intersects(arr[i], monitor->base));
        assert(intersects(arr[i], monitor->view) == 0);
        assert(!intersects(arr[i], sideMonitor->base));
        for(int n = 0; n < 4; n++)
            assert((&sideMonitor->base.x)[n] == (&sideMonitor->view.x)[n]);
        removeWindow(info->id);
        assert(intersects(arr[i], monitor->view));
    }
    for(int c = 0; c < 3; c++){
        WindowInfo* info = createWindowInfo(c + 1);
        info->onlyOnPrimary = 1;
        int properties[12];
        for(int i = 0; i < 4; i++){
            properties[i] = dockSize - c / 2;
            properties[i * 2 + 4] = 0;
            properties[i * 2 + 4 + 1] = dim;
        }
        markAsDock(info);
        setDockArea(info, arrSize, properties);
        assert(addWindowInfo(info));
    }
    assert(monitor->view.width * monitor->view.height == (dim - dockSize * 2) * (dim - dockSize * 2));
    assert(sideMonitor->view.width * sideMonitor->view.height == dim * dim);
}
END_TEST

START_TEST(test_intersection){
    int dimX = 100;
    int dimY = 200;
    int offsetX = 10;
    int offsetY = 20;
    Rect rect = {offsetX, offsetY, dimX, dimY};
#define _intersects(arr,x,y,w,h) intersects(arr,(Rect){x,y,w,h})
    //easy to see if fail
    assert(!_intersects(rect, 0, 0, offsetX, offsetY));
    assert(!_intersects(rect, 0, offsetY, offsetX, offsetY));
    assert(!_intersects(rect, offsetX, 0, offsetX, offsetY));
    for(int x = 0; x < dimX + offsetX * 2; x++){
        assert(_intersects(rect, x, 0, offsetX, offsetY) == 0);
        assert(_intersects(rect, x, offsetY + dimY, offsetX, offsetY) == 0);
    }
    for(int y = 0; y < dimY + offsetY * 2; y++){
        assert(_intersects(rect, 0, y, offsetX, offsetY) == 0);
        assert(_intersects(rect, offsetX + dimX, y, offsetX, offsetY) == 0);
    }
    assert(_intersects(rect, 0, offsetY, offsetX + 1, offsetY));
    assert(_intersects(rect, offsetX + 1, offsetY, 0, offsetY));
    assert(_intersects(rect, offsetX + 1, offsetY, dimX, offsetY));
    assert(_intersects(rect, offsetX, 0, offsetX, offsetY + 1));
    assert(_intersects(rect, offsetX, offsetY + 1, offsetX, 0));
    assert(_intersects(rect, offsetX, offsetY + 1, offsetX, dimY));
    assert(_intersects(rect, offsetX + dimX / 2, offsetY + dimY / 2, 0, 0));
}
END_TEST
START_TEST(test_fake_dock){
    WindowInfo* winInfo = createWindowInfo(1);
    markAsDock(winInfo);
    loadDockProperties(winInfo);
    addWindowInfo(winInfo);
    for(int i = 0; i < LEN(winInfo->properties); i++)
        assert(winInfo->properties[i] == 0);
}
END_TEST
START_TEST(test_avoid_docks){
    int size = 10;
    for(int i = 0; i < 4; i++){
        WindowInfo* winInfo = createWindowInfo(createDock(i, size, _i));
        loadDockProperties(winInfo);
        if(i < 2)
            markAsDock(winInfo);
        addWindowInfo(winInfo);
        if(i >= 2)
            markAsDock(winInfo);
    }
    assert(getSize(getAllDocks()) == 4);
    Monitor* m = getHead(getAllMonitors());
    Rect arr[4] = {
        {0, 0, size, getRootHeight()}, //left
        {getRootWidth() - size, 0, size, getRootHeight()},
        {0, 0, getRootWidth(), size}, //top
        {0, getRootHeight() - size, getRootWidth(), size}
    };
    int index = 0;
    FOR_EACH(WindowInfo*, winInfo, getAllDocks()){
        int type = index++;
        assert(winInfo->properties[type]);
        for(int j = 0; j < 4; j++)
            if(j != type)
                assert(winInfo->properties[j] == 0);
    }
    for(int i = 0; i < 4; i++)
        assert(intersects(m->view, arr[i]) == 0);
}
END_TEST






START_TEST(test_monitor_init){
    int size = 5;
    int count = 0;
    addFakeMaster(1, 1);
    for(int x = 0; x < 2; x++)
        for(int y = 0; y < 2; y++){
            short dim[4] = {x, y, size* size + y, size * 2 + x};
            assert(updateMonitor(count, !count, *(Rect*)dim));
            Monitor* m = find(getAllMonitors(), &count, sizeof(int));
            assert(m->id == count);
            assert(isPrimary(m) == !count);
            for(int i = 0; i < 4; i++){
                assert((&m->base.x)[i] == (&m->view.x)[i]);
                assert(dim[i] == (&m->base.x)[i]);
            }
            assert(getWorkspaceFromMonitor(m));
            assert(getWorkspaceFromMonitor(m)->id == count++);
        }
}
END_TEST
START_TEST(test_monitor_add_remove){
    addFakeMaster(1, 1);
    int size = getNumberOfWorkspaces();
    for(int n = 0; n < 2; n++){
        for(int i = 1; i <= size + 1; i++){
            updateMonitor(i, 1, (Rect){
                0, 0, 100, 100
            });
            Workspace* w = getWorkspaceFromMonitor(find(getAllMonitors(), &i, sizeof(int)));
            if(i > size)
                assert(!w);
            else assert(w);
        }
        assert(getSize(getAllMonitors()) == size + 1);
    }
    //for(int i=0;i<getNumberOfWorkspaces();i++)
    //assert(getWorkspaceByIndex(i)->monitor==NULL);
    while(isNotEmpty(getAllMonitors()))
        assert(removeMonitor(((Monitor*)getLast(getAllMonitors()))->id));
    for(int i = 0; i < getNumberOfWorkspaces(); i++)
        assert(getWorkspaceByIndex(i)->monitor == NULL);
    assert(!removeMonitor(0));
}
END_TEST


Suite* monitorsSuite(){
    Suite* s = suite_create("Monitors");
    TCase* tc_core;
    tc_core = tcase_create("Monitor");
    tcase_add_checked_fixture(tc_core, createSimpleContext, resetContext);
    tcase_add_test(tc_core, test_monitor_init);
    tcase_add_test(tc_core, test_monitor_add_remove);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Detect monitors");
    tcase_add_checked_fixture(tc_core, createContextAndSimpleConnection, destroyContextAndConnection);
    tcase_add_test(tc_core, test_detect_monitors);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Docks");
    tcase_add_checked_fixture(tc_core, createContextAndSimpleConnection, destroyContextAndConnection);
    tcase_add_test(tc_core, test_intersection);
    tcase_add_loop_test(tc_core, test_avoid_struct, 0, 2);
    tcase_add_loop_test(tc_core, test_avoid_docks, 0, 2);
    tcase_add_test(tc_core, test_fake_dock);
    suite_add_tcase(s, tc_core);
    return s;
}
