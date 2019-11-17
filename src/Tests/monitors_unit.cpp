#include "../monitors.h"
#include "tester.h"
#include "test-mpx-helper.h"
#include "../logger.h"
#include "../system.h"
#include "../mywm-structs.h"
#include "../masters.h"
#include "../windows.h"

#include <iostream>
// we do a lot of short arthimatic which get promoted to an int
// which causes narrowing we we try to store it back into a short
// ignore
#pragma GCC diagnostic ignored "-Wnarrowing"
SET_ENV(addDefaultMaster, simpleCleanup);

MPX_TEST("addRootMonitor", {
    addRootMonitor();
    assert(getAllMonitors().size() == 1);
    getAllMonitors().deleteElements();
    assert(getAllMonitors().size() == 0);
    uint16_t rootBounds[2] = {100, 10};
    setRootDims(rootBounds);
    addRootMonitor();
    assert(getAllMonitors().size() == 1);
    assert(getAllMonitors()[0]->getBase().height == rootBounds[1]);
    assert(getAllMonitors()[0]->getBase().width == rootBounds[0]);
    assert(getAllMonitors()[0]->getBase().x == 0);
    assert(getAllMonitors()[0]->getBase().y == 0);
    // should be able to call more than once
    addRootMonitor();
});

MPX_TEST_ITER("test_avoid_docks", 4 * 2 + 2, {
    const short dim = 100;
    const short dockSize = 10;
    static Rect base = {0, 0, dim, dim};
    static Rect other = {dim, 0, dim, dim};
    static Rect bottom = {0, dim, dim, dim};
    uint16_t rootBounds[2] = {dim * 2, dim * 2};
    setRootDims(rootBounds);
    static Monitor* monitor = new Monitor(1, base);
    static Monitor* otherMonitor = new Monitor(2, other);
    static Monitor* bottomMonitor = new Monitor(3, bottom);
    getAllMonitors().add(monitor);
    getAllMonitors().add(otherMonitor);
    getAllMonitors().add(bottomMonitor);
    bool notFull = _i % 2;
    int index = _i / 2;
    for(int i = 0; i < 4; i++) {
        if(index < 4 && i != index)
            continue;
        WindowInfo* info = new WindowInfo(i);
        assert(addWindowInfo(info));
        info->setDock();
        int properties[12] = {0};
        properties[i] = dockSize;
        properties[i * 2 + 4] = 0;
        properties[i * 2 + 4 + 1] = notFull ? dim : 0;
        info->setDockProperties(properties, 12);
        resizeAllMonitorsToAvoidAllDocks();
        assert(monitor->getBase() == base);
        assert(otherMonitor->getBase() == other);
        assert(bottomMonitor->getBase() == bottom);
    }
    const Rect bounds[2][5][3] = {
        {
            {{dockSize, 0, base.width - dockSize, base.height}, other, {bottom.x + dockSize, bottom.y, bottom.width - dockSize, bottom.height}},
            {base, {other.x, other.y, other.width - dockSize, other.height}, bottom},
            {{base.x, base.y + dockSize, base.width, base.height - dockSize}, {other.x, other.y + dockSize, other.width, other.height - dockSize}, bottom},
            {base, other, {bottom.x, bottom.y, bottom.width, bottom.height - dockSize}},
            {{base.x + dockSize, base.y + dockSize, base.width - dockSize, base.height - dockSize}, {other.x, other.y + dockSize, other.width - dockSize, other.height - dockSize}, {bottom.x, bottom.y, bottom.width, bottom.height - dockSize}},
        },
        {
            {{dockSize, 0, base.width - dockSize, base.height}, other, bottom},
            {base, {other.x, other.y, other.width - dockSize, other.height}, bottom},
            {{0, dockSize, base.width, base.height - dockSize}, other, bottom},
            {base, other, {bottom.x, bottom.y, bottom.width, bottom.height - dockSize}},
            {{dockSize, dockSize, base.width - dockSize, base.height - dockSize}, {other.x, other.y, other.width - dockSize, other.height}, {bottom.x, bottom.y, bottom.width, bottom.height - dockSize}},
        }
    };

    assert(bounds[notFull][index][0] == monitor->getViewport());
    assert(bounds[notFull][index][1] == otherMonitor->getViewport());
});
MPX_TEST_ITER("avoid_docks_ignore", 2, {
    addWorkspaces(2);
    int properties[4] = {1, 1, 1, 1};
    WindowInfo* winInfo = new WindowInfo(1);
    assert(addWindowInfo(winInfo));
    winInfo->setDock();
    winInfo->setDockProperties(properties, 4);
    if(_i)
        winInfo->moveToWorkspace(1);
    else
        winInfo->addMask(PRIMARY_MONITOR_MASK);
    const short dim = 100;
    static Rect base = {0, 0, dim, dim};
    uint16_t rootBounds[2] = {dim * 2, dim * 2};
    setRootDims(rootBounds);
    static Monitor* monitor = new Monitor(1, base);
    monitor->assignWorkspace(0);
    assert(!monitor->resizeToAvoidDock(winInfo));
    assertEquals(monitor->getBase(), monitor->getViewport());
});


MPX_TEST("get_set_base", {
    Monitor* m = new Monitor(1, {0, 0, 1, 1});
    Rect b = {0, 0, 100, 200};
    m->setBase(b);
    assertEquals(m->getBase(), b);
    getAllMonitors().add(m);
});


MPX_TEST("monitor_primary", {
    assert(!getPrimaryMonitor());
    Monitor* m = new Monitor(2, {1, 1, 1, 1});
    Monitor* m2 = new Monitor(3, {1, 1, 1, 1});
    getAllMonitors().add(m);
    assert(!getPrimaryMonitor());
    m->setPrimary();
    assertEquals(getPrimaryMonitor(), m);
    assert(m->isPrimary());
    getAllMonitors().add(m2);
    assertEquals(getPrimaryMonitor(), m);
    m2->setPrimary();
    assertEquals(getPrimaryMonitor(), m2);
    assert(!m->isPrimary());
    assert(m2->isPrimary());
});


MPX_TEST_ITER("test_monitor_dup_resolution", 5, {
    int masks[] = {TAKE_PRIMARY, TAKE_LARGER, TAKE_SMALLER, TAKE_PRIMARY, 0};
    MONITOR_DUPLICATION_POLICY = INTERSECTS | CONTAINS | SAME_DIMS;
    MONITOR_DUPLICATION_RESOLUTION = masks[_i];

    Monitor* smaller = new Monitor(2, {1, 1, 1, 1});
    Monitor* larger = new Monitor(3, {0, 0, 100, 100});
    getAllMonitors().add(smaller);
    getAllMonitors().add(larger);
    Monitor* primary = _i % 2 == 0 ? larger : smaller;
    primary->setPrimary();
    assert(getAllMonitors().size() == 2);
    removeDuplicateMonitors();
    if(_i == LEN(masks) - 1) {
        assert(getAllMonitors().size() == 2);
        exit(0);
    }

    assert(getAllMonitors().size() == 1);
    assert(getAllMonitors()[0] == (_i < LEN(masks) / 2 ? larger : smaller));
});
MPX_TEST_ITER("test_monitor_intersection", 12, {
    int id = 2;
    MONITOR_DUPLICATION_RESOLUTION = TAKE_LARGER;
    int i = _i / 3;
    int n = _i % 3;
    int masks[] = {SAME_DIMS, CONTAINS, CONTAINS_PROPER, INTERSECTS};
    MONITOR_DUPLICATION_POLICY = masks[i];
    int sizes[][3] = {
        {3, 2, 1},
        {2, 1, 1},
        {3, 2, 2},
        {2, 1, 1}
    };
    int size = sizes[i][n];
    Monitor* m = new Monitor(id++, {0, 0, 100, 100});
    getAllMonitors().add(m);
    Rect base = m->getBase();
    switch(n) {
        case 0:
            getAllMonitors().add(new Monitor(id++, {base.x + base.width / 2, base.y, base.width, base.height}));
            getAllMonitors().add(new Monitor(id++, {base.x + base.width / 2, base.y + base.height / 2, base.width / 4, base.height / 4}));
            break;
        case 1:
            getAllMonitors().add(new Monitor(id++, {1, 1, 99, 99}));
            break;
        case 2:
            getAllMonitors().add(new Monitor(id++, base));
            break;
    }
    removeDuplicateMonitors();
    assertEquals(getAllMonitors().size(), size);
    getAllMonitors().deleteElements();
});
MPX_TEST_ITER("swapMonitors", 4, {
    addWorkspaces(2);
    for(int i = 1; i <= _i; i++)
        getAllMonitors().add(new Monitor(i, {0, 0, 1, 1}));
    assignUnusedMonitorsToWorkspaces();
    Monitor* monitor[2] = {getWorkspace(0)->getMonitor(), getWorkspace(1)->getMonitor()};
    bool visible[2] = {getWorkspace(0)->isVisible(), getWorkspace(1)->isVisible()};
    swapMonitors(0, 1);
    assert(monitor[1] == getWorkspace(0)->getMonitor());
    assert(monitor[0] == getWorkspace(1)->getMonitor());
    assert(visible[1] == getWorkspace(0)->isVisible());
    assert(visible[0] == getWorkspace(1)->isVisible());
});

MPX_TEST_ITER("assign_workspace", 2, {
    addWorkspaces(2);
    getAllMonitors().add(new Monitor(1, {0, 0, 1, 1}));
    Monitor* m = getAllMonitors()[0];
    m->assignWorkspace(_i ? getWorkspace(0) : NULL);
    assert(getWorkspace(0)->getMonitor() == m);
    assert(getWorkspace(0) == m->getWorkspace());
    assert(getWorkspace(0)->isVisible());
});
MPX_TEST("test_auto_assign_workspace", {
    addWorkspaces(2);
    for(int n = 1; n < getNumberOfWorkspaces() + 1; n++) {
        getAllMonitors().add(new Monitor(n, {0, 0, 1, 1}));
        assignUnusedMonitorsToWorkspaces();
        int count = 0;
        for(int i = 0; i < getNumberOfWorkspaces(); i++)
            count += getWorkspace(i)->isVisible();
        assert(count == n);
        for(int i = 0; i < n && i < getNumberOfWorkspaces(); i++)
            assert(getWorkspace(i)->isVisible());
        for(int i = n; i < getNumberOfWorkspaces(); i++)
            assert(!getWorkspace(i)->isVisible());
    }
});
MPX_TEST("monitor_cleanup", {
    addWorkspaces(2);
    getAllMonitors().add(new Monitor(1, {0, 0, 1, 1}));
    Monitor* m = getAllMonitors()[0];
    m->assignWorkspace(getWorkspace(0));
    assert(getWorkspace(0)->getMonitor() == m);
    getAllMonitors().deleteElements();
    assert(getWorkspace(0)->getMonitor() == NULL);
});
