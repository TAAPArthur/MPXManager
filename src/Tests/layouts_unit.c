#include "../devices.h"
#include "../layouts.h"
#include "../monitors.h"
#include "../wmfunctions.h"
#include "test-event-helper.h"
#include "test-mpx-helper.h"
#include "test-x-helper.h"
#include "tester.h"

#define _LAYOUT_FAMILY(S){.name=""#S,.func=S}
// we do a lot of short arthimatic which get promoted to an int
// which causes narrowing we we try to store it back into a short
// ignore
#pragma GCC diagnostic ignored "-Wnarrowing"

static Layout LAYOUT_FAMILIES[] = {
    _LAYOUT_FAMILY(full),
    _LAYOUT_FAMILY(column),
    _LAYOUT_FAMILY(masterPane),
};
int NUMBER_OF_LAYOUT_FAMILIES = LEN(LAYOUT_FAMILIES);
SCUTEST_SET_ENV(createXSimpleEnv, cleanupXServer);

/**
 * Test to ensure layouts take up the whole screen and that windows that if any windows overlap, they overlap completely
 */
SCUTEST_ITER(test_layouts, NUMBER_OF_LAYOUT_FAMILIES) {
    DEFAULT_BORDER_WIDTH = 0;
    Rect view = {0, 0, 100, 200};
    Monitor* m = getHead(getAllMonitors());
    m->view = view;
    int targetArea = getArea(view);
    int layoutIndex = _i;
    toggleActiveLayout(&LAYOUT_FAMILIES[layoutIndex]);
    int size = 7;
    int limit = 5;
    getActiveLayout()->args.limit = limit;
    retile();
    for(int i = 1; i <= size; i++) {
        WindowID win =
            i % 3 == 0 ? mapArbitraryWindow() :
            i % 3 == 1 ? mapWindow(createNormalSubWindow(createNormalWindow())) :
            mapArbitraryWindow();
        WindowInfo* winInfo = addWindow(win);
        moveToWorkspace(winInfo, 0);
        addMask(winInfo, MAPPABLE_MASK | MAPPED_MASK);
        assert(isTileable(winInfo));
        retile();
        int area = 0;
        RectWithBorder rects[getActiveWindowStack()->size];
        int numUniqueRects = 0;
        FOR_EACH(WindowInfo*, winInfo, getActiveWindowStack()) {
            Rect r = getRealGeometry(winInfo->id);
            dumpRect(r);
            bool noIntersections = 1;
            for(int i = 0; i < numUniqueRects; i++)
                if(intersects(rects[i], r)) {
                    noIntersections = 0;
                }
                else assert(noIntersections == 1);
            if(noIntersections) {
                rects[numUniqueRects++] = r;
                area += getArea(r);
            }
        }
        assert(numUniqueRects <= limit);
        assertEquals(area, targetArea);
    }
}
SCUTEST_ITER(test_layouts_with_param, NUMBER_OF_LAYOUT_FAMILIES) {
    toggleActiveLayout(&LAYOUT_FAMILIES[_i]);
    getActiveLayout()->args.argStep = 1;
    assertEquals(0, getActiveLayout()->args.arg);
    increaseLayoutArg(LAYOUT_ARG, 1, getActiveLayout());
    assertEquals(1, getActiveLayout()->args.arg);
}
SCUTEST_ITER(test_layouts_unmapped_windows, NUMBER_OF_LAYOUT_FAMILIES) {
    DEFAULT_BORDER_WIDTH = 0;
    toggleActiveLayout(&LAYOUT_FAMILIES[_i]);
    RectWithBorder rect = {0, 0, 1, 1};
    Window unmapped = createUnmappedWindow();
    setWindowPosition(unmapped, rect);
    mapArbitraryWindow();
    createUnmappedWindow();
    scan(root);
    FOR_EACH(WindowInfo*, winInfo, getAllWindows()) {
        moveToWorkspace(winInfo, getActiveWorkspaceIndex());
    }
    retile();
    RectWithBorder geo = getRealGeometry(unmapped);
    assertEquals(*(long*)&rect, *(long*)&geo);
}

SCUTEST(raise_focused) {
    toggleActiveLayout(&FULL);
    assert(getActiveLayout()->args.raiseFocused);
    Window ids[] = {mapArbitraryWindow(), mapArbitraryWindow(), mapArbitraryWindow()};
    scan(root);
    FOR_EACH(WindowInfo*, winInfo, getAllWindows())
    moveToWorkspace(winInfo, 0);
    onWindowFocus(ids[2]);
    onWindowFocus(ids[1]);
    retile();
    WindowID stack[] = {ids[0], ids[2], ids[1]};
    assert(checkStackingOrder(stack, LEN(stack)));
}

SCUTEST_ITER(test_fixed_position_windows, NUMBER_OF_LAYOUT_FAMILIES + 1) {
    DEFAULT_BORDER_WIDTH = 0;
    mapArbitraryWindow();
    flush();
    scan(root);
    RectWithBorder config = {1, 2, 3, 4, 5};
    WindowInfo* winInfo = getHead(getAllWindows());
    if(_i == NUMBER_OF_LAYOUT_FAMILIES)
        floatWindow(winInfo);
    moveToWorkspace(winInfo, 0);
    setTilingOverrideEnabled(winInfo, 31);
    setTilingOverride(winInfo, config);
    toggleActiveLayout(&LAYOUT_FAMILIES[_i % NUMBER_OF_LAYOUT_FAMILIES ]);
    retile();
    RectWithBorder actualConfig = getRealGeometry(winInfo->id);
    assertEqualsRect(config, actualConfig);
}
SCUTEST(test_fixed_position_windows_relative) {
    DEFAULT_BORDER_WIDTH = 0;
    getMonitor(getWorkspace(0))->base = (Rect) {100, 200, 300, 400};
    Rect bounds = getMonitor(getWorkspace(0))->view;
    mapArbitraryWindow();
    flush();
    scan(root);
    RectWithBorder config;
    Rect expectedConfig = {0, 0, 0, 0};
    WindowInfo* winInfo = getHead(getAllWindows());
    if(_i) {
        setTilingOverrideEnabled(winInfo, 31);
        config = (RectWithBorder) {-1, 0, 0, -1};
        expectedConfig = (Rect) {bounds.x + bounds.width + config.x, bounds.y, bounds.width, bounds.height + config.height};
    }
    else {
        setTilingOverrideEnabled(winInfo, -1);
        config = (RectWithBorder) {-1, -1, 0, 0};
        expectedConfig = (Rect) {bounds.x + config.x, bounds.y + config.y, bounds.width, bounds.height };
    }
    floatWindow(winInfo);
    moveToWorkspace(winInfo, 0);
    setTilingOverride(winInfo, config);
    retile();
    RectWithBorder actualConfig = getRealGeometry(winInfo->id);
    assertEqualsRect(expectedConfig, actualConfig);
}

SCUTEST(test_maximized_floating_window) {
    DEFAULT_BORDER_WIDTH = 0;
    mapArbitraryWindow();
    flush();
    scan(root);
    WindowInfo* winInfo = getHead(getAllWindows());
    floatWindow(winInfo);
    addMask(winInfo, X_MAXIMIZED_MASK);
    moveToWorkspace(winInfo, 0);
    RectWithBorder bounds = {1, 2, 3, 4};
    setWindowPosition(winInfo->id, bounds);
    toggleActiveLayout(&FULL);
    retile();
    RectWithBorder expectedConfig = bounds;
    expectedConfig.width = getMonitor(getWorkspace(0))->view.width;
    assertEqualsRect(expectedConfig, getRealGeometry(winInfo->id));
}

SCUTEST_ITER(test_identity_transform_config, TRANSFORM_LEN * 2) {
    DEFAULT_BORDER_WIDTH = 0;
    int n = 10;
    int d = _i % 2 ? 0 : n;
    _i /= 2;
    Rect view = {.x = d, .y = d, .width = n, .height = n};
    Monitor* monitor = addFakeMonitor(view);
    uint32_t config[CONFIG_LEN] = {d, d, n, n};
    int correctConfig[4] = {d, d, n, n};
    LayoutArgs prop = {.transform = (Transform)_i};
    transformConfig(&prop, monitor, config);
    assert(memcmp(config, correctConfig, sizeof(correctConfig)) == 0);
}
SCUTEST_ITER(test_simple_transform_config, TRANSFORM_LEN) {
    DEFAULT_BORDER_WIDTH = 0;
    Monitor* monitor = addFakeMonitor((Rect) {0, 0, 0, 0});
    for(int i = 1; i < 5; i++) {
        uint32_t config[TRANSFORM_LEN] = {i % 2, i / 2, 0, 0};
        const int correctConfig[TRANSFORM_LEN][2] = {
            [NONE] = {config[0], config[1]},
            [ROT_180] = {-config[0], -config[1]},
            [REFLECT_HOR] = {-config[0], config[1]},
            [REFLECT_VERT] = {config[0], -config[1]},
        };
        LayoutArgs prop = {.transform = (Transform)_i};
        transformConfig(&prop, monitor, config);
        assert(memcmp(config, correctConfig[_i], sizeof(correctConfig[_i])) == 0);
    }
}
SCUTEST(floating_windows) {
    WindowID win = mapWindow(createNormalWindow());
    WindowID win2 = mapWindow(createNormalWindow());
    scan(root);
    FOR_EACH(WindowInfo*, winInfo, getAllWindows()) {
        moveToWorkspace(winInfo, getActiveWorkspaceIndex());
        floatWindow(winInfo);
    }
    setTilingOverrideEnabled(getWindowInfo(win2), 1 << CONFIG_INDEX_BORDER);
    int customBorder = 5;
    setTilingOverride(getWindowInfo(win2), (RectWithBorder) {0, 0, 0, 0, customBorder});
    retile();
    assertEquals(getRealGeometry((win)).border, DEFAULT_BORDER_WIDTH);
    assertEquals(getRealGeometry((win2)).border, customBorder);
}

SCUTEST(test_tile_windows) {
    //retile empty workspace
    tileWorkspace(getActiveWorkspace());
    //bottom to top stacking order;
    WindowID stackingOrder[3] = {0};
    int masks[] = {BELOW_MASK, 0, ABOVE_MASK};
    for(int i = 0; i < LEN(stackingOrder); i++) {
        WindowID win = mapWindow(createNormalWindow());
        WindowInfo* winInfo = addWindow(win);
        moveToWorkspace(winInfo, getActiveWorkspaceIndex());
        addMask(winInfo, masks[i]);
        stackingOrder[i] = win;
        lowerWindow(winInfo->id, 0);
    }
    retile();
    flush();
    assert(checkStackingOrder(stackingOrder, LEN(stackingOrder)));
}


static RectWithBorder baseConfig;
static void dummyLayout(LayoutState* state) {
    tileWindow(state, getHead(state->stack), (short*)&baseConfig);
};
SCUTEST_ITER(test_privileged_windows_size, 9 * 2) {
    WindowMask extra = _i % 2 ? NO_MASK : NO_TILE_MASK;
    _i /= 2;
    Monitor* m = getHead(getAllMonitors());
    assert(m);
    RectWithBorder view = {10, 20, 30, 40};
    m->view = (view);
    WindowInfo* winInfo = addWindow(mapWindow(createNormalWindow()));
    RectWithBorder startingPos = {0, 10, 150, 200};
    setWindowPosition(winInfo->id, startingPos);
    moveToWorkspace(winInfo, getActiveWorkspaceIndex());
baseConfig = extra == NO_MASK ? (RectWithBorder) {20, 30, 40, 50}:
    startingPos;
    Layout l = {"", .func = dummyLayout, {.noBorder = 1}};
    toggleActiveLayout(&l);
    DEFAULT_BORDER_WIDTH = 1;
    struct {
        WindowMask mask;
        const RectWithBorder dims;
    } arr[] = {
        {0, baseConfig},

        {X_CENTERED_MASK | extra, {m->view.x + m->view.width / 2, baseConfig.y, baseConfig.width, baseConfig.height, DEFAULT_BORDER_WIDTH }},
        {Y_CENTERED_MASK | extra, {baseConfig.x, m->view.y + m->view.height / 2, baseConfig.width, baseConfig.height, DEFAULT_BORDER_WIDTH}},
        {CENTERED_MASK | extra, {m->view.x + m->view.width / 2, m->view.y + m->view.height / 2, baseConfig.width, baseConfig.height, DEFAULT_BORDER_WIDTH}},
        {X_MAXIMIZED_MASK | extra, {baseConfig.x, baseConfig.y, m->view.width, baseConfig.height, DEFAULT_BORDER_WIDTH }},
        {Y_MAXIMIZED_MASK | extra, {baseConfig.x, baseConfig.y, baseConfig.width, m->view.height, DEFAULT_BORDER_WIDTH}},
        {MAXIMIZED_MASK | extra, {baseConfig.x, baseConfig.y, m->view.width, m->view.height, DEFAULT_BORDER_WIDTH}},
        {FULLSCREEN_MASK | extra, m->base},
        {ROOT_FULLSCREEN_MASK | extra, {0, 0, getRootWidth(), getRootHeight()}},
    };
    assert(_i < LEN(arr));
    int baseMask = MAPPABLE_MASK | MAPPED_MASK;
    removeMask(winInfo, -1);
    addMask(winInfo, baseMask | arr[_i].mask);
    retile();
    Rect rect = getRealGeometry(winInfo->id);
    assertEquals(*(long*)&arr[_i].dims, *(long*)&rect);
    toggleActiveLayout(NULL);
}

SCUTEST(test_register_restore_layout) {
    Layout c = {"", NULL};
    toggleActiveLayout(&c);
    LayoutArgs args = c.args;
    for(int i = 0; i <= LAYOUT_ARG; i++)
        increaseLayoutArg(i, i * i, &c);
    assert(memcmp(&c.args, &args, sizeof(LayoutArgs)));
    restoreArgs(&c);
    assert(!memcmp(&c.args, &args, sizeof(LayoutArgs)));
    toggleActiveLayout(NULL);
}
