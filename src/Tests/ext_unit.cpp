#include "../ext.h"
#include "test-mpx-helper.h"
#include "test-x-helper.h"
#include "tester.h"


static void* id1 = (void*)1, *id2 = (void*)2;

static void cleanup() {
    removeID(id1);
    removeID(id2);
    cleanupXServer();
}
struct foo {
    int arr[12];
};

static Index<foo> key, key2;

SET_ENV(NULL, cleanup);
MPX_TEST("keys", {
    static Index<foo> key;
    get(key, id1);
});

MPX_TEST("mpx_getters", {
    assert(getAllWorkspaces().size() == 0);
    assert(getAllMonitors().size() == 0);
    assert(getAllWindows().size() == 0);
    assert(getAllSlaves().size() == 0);
    addDefaultMaster();
    assert(getAllMasters().size() == 1);
    addWorkspaces(1);
    assert(getAllWorkspaces().size() == 1);
});
MPX_TEST("print", {
    suppressOutput();
    printSummary();
    addWorkspaces(20);
    getAllWindows().add(new WindowInfo(1));
    addRootMonitor();
    addDefaultMaster();
    getAllSlaves().add(new Slave(1, 0, 1));
    printSummary();
});
MPX_TEST("get_new_no_create", {
    bool newElement = 1;
    assert(!get<foo>(key, id1, 0, &newElement));
    assert(!newElement);
    assert(!get<foo>(key, id1, 0, &newElement));
    assert(!newElement);
});

MPX_TEST("get_new", {
    bool newElement;
    assert(!get<foo>(key, id1, 0, &newElement));
    auto p = get<foo>(key, id1, 1, &newElement);
    assert(p);
    assert(newElement);
    assertEquals(p, get<foo>(key, id1, 1, &newElement));
    assert(!newElement);
});
MPX_TEST("get_ext", {
    auto bar = get<foo>(key, id1);
    assert(bar);
    assert(bar == get<foo>(key, id1));
    for(int i = 0; i < LEN(foo::arr); i++)
        bar->arr[i]++;
});
MPX_TEST("get_ext_modify", {
    foo* bar = get<foo>(key, id1);
    assert(bar->arr[0] != 1);
    for(int i = 0; i < LEN(foo::arr); i++)
        bar->arr[i] = 0;
    bar->arr[0] = 1;
    bar = get<foo>(key, id1);
    assert(bar->arr[0] == 1);
});
MPX_TEST("get_ext_isolation", {
    foo* bar[4] = {
        get<foo>(key, id1),
        get<foo>(key, id2),
        get<foo>(key2, id1),
        get<foo>(key2, id2),
    };
    int i = 0;
    for(foo* b : bar)
        b
        ->arr[0] = i++;
    i = 0;
    for(foo* b : bar)
        assert(b->arr[0] == i++);
});
