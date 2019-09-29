
#include "tester.h"
#include "test-mpx-helper.h"
#include "test-x-helper.h"
#include "test-event-helper.h"
#include "../layouts.h"
#include "../window-properties.h"
#include "../wmfunctions.h"
#include "../monitors.h"
#include "../devices.h"

#define _LAYOUT_FAMILY(S)Layout{.name=""#S,.layoutFunction=S}
// we do a lot of short arthimatic which get promoted to an int
// which causes narrowing we we try to store it back into a short
// ignore
#pragma GCC diagnostic ignored "-Wnarrowing"

Layout LAYOUT_FAMILIES[] = {
    _LAYOUT_FAMILY(full),
    _LAYOUT_FAMILY(column),
    _LAYOUT_FAMILY(masterPane),
};
int NUMBER_OF_LAYOUT_FAMILIES = LEN(LAYOUT_FAMILIES);
void setActiveLayoutByName(std::string name, Workspace* w = getActiveWorkspace()) {
    w->setActiveLayout(findLayoutByName(name));
}
SET_ENV(createXSimpleEnv, fullCleanup);
MPX_TEST("print_layout", {
    suppressOutput();
    std::cout << getDefaultLayouts()[FULL];
});

MPX_TEST("tile_worskspace_event", {

    addWorkspaces(2);
    mapArbitraryWindow();
    mapArbitraryWindow();
    mapArbitraryWindow();
    scan(root);
    getAllWindows()[0]->moveToWorkspace(0);
    getAllWindows()[1]->moveToWorkspace(1);
    getEventRules(TileWorkspace).add(DEFAULT_EVENT(incrementCount));
    getEventRules(TileWorkspace).add(DEFAULT_EVENT(+[](WindowInfo * winInfo) {assertEquals(winInfo, getAllWindows()[1]); return 0;}));
    getEventRules(TileWorkspace).add(DEFAULT_EVENT(incrementCount));
    tileWorkspace(1);
    assertEquals(getCount(), 1);
});




MPX_TEST("tile", {
    addWorkspaces(2);
    static int count = 0;
    void (*dummy)(LayoutState * state) = [](LayoutState * state) {
        assert(state);
        count++;
    };
    Layout l = {.layoutFunction = dummy};
    mapArbitraryWindow();
    scan(root);
    getAllWindows()[0]->moveToWorkspace(getActiveWorkspaceIndex());
    setActiveLayout(&l);
    retile();
    assert(count == 1);
    tileWorkspace(2);
    assert(count == 1);
});

/**
 * Test to ensure layouts take up the whole screen and that windows that if any windows overlap, they overlap completely
 */
MPX_TEST_ITER("test_layouts", NUMBER_OF_LAYOUT_FAMILIES, {
    DEFAULT_BORDER_WIDTH = 0;
    Monitor* m = getAllMonitors()[0];
    m->setBase({0, 0, 600, 60});
    m->setViewport({0, 0, 60, 120});
    int targetArea = m->getViewport().width * m->getViewport().height;
    int layoutIndex = _i;
    setActiveLayout(&LAYOUT_FAMILIES[layoutIndex]);
    int size = 7;
    int limit = 5;
    getActiveLayout()->getArgs().limit = limit;
    retile();
    int area;
    for(int i = 1; i <= size; i++) {
        WindowID win =
        i == 0 ? mapArbitraryWindow():
        i == 1 ? mapWindow(createInputOnlyWindow()) :
        i == 1 ? mapWindow(createNormalSubWindow(createNormalWindow())) :
        mapArbitraryWindow();
        registerWindow(win, root);
        WindowInfo* winInfo = getWindowInfo(win);
        winInfo->moveToWorkspace(0);
        winInfo->addMask(FULLY_VISIBLE);
        assert(getNumberOfWindowsToTile(getActiveWindowStack(), NULL) >= i);
        retile();
        flush();
        assert(getNumberOfWindowsToTile(getActiveWindowStack(), NULL) == i);
        assert(getNumberOfWindowsToTile(getActiveWindowStack(), NULL) == getActiveWindowStack().size());
        area = 0;
        ArrayList<Rect>rects;
        for(WindowInfo* winInfo : getActiveWindowStack()) {
            Rect r = getRealGeometry(winInfo);
            bool noIntersections = 1;
            for(Rect rect : rects)
                if(r.intersects(rect)) {
                    assert(r == rect);
                    noIntersections = 0;
                }
                else assert(noIntersections == 1);
            if(noIntersections) {
                rects.add(r);
                area += r.width * r.height;
            }
        }
        assertEquals(area, targetArea);
        assert(rects.size() <= limit);
        rects.clear();
    }
});
MPX_TEST_ITER("default_layouts", LAST_LAYOUT, {
    DEFAULT_BORDER_WIDTH = 0;
    Monitor* m = getAllMonitors()[0];
    short w = 60;
    short h = 120;
    m->setBase({0, 0, w, h});
    setActiveLayout(getDefaultLayouts()[_i]);
    float arg = getActiveLayout()->getArgs().arg;
    const int size = 3;
    const Rect empty = {0, 0, 0, 0};
    const Rect rect[][size] = {
        [FULL] = {m->getBase(), empty, empty},
        [GRID] = {{0, 0, w / 2, h}, {w / 2, 0, w / 2, h / 2}, {w / 2, h / 2, w / 2, h / 2}},
        [TWO_COLUMN] = {{0, 0, w / 2, h}, {w / 2, 0, w / 2, h / 2}, {w / 2, h / 2, w / 2, h / 2}},
        [TWO_ROW] = {{0, 0, w, h / 2}, {0, h / 2, w / 2, h / 2}, {w / 2, h / 2, w / 2, h / 2}},
        [TWO_PANE] = {{0, 0, w / 2, h}, {w / 2, 0, w / 2, h}, empty},
        [TWO_HPLANE] = {{0, 0, w, h / 2}, {0, h / 2, w, h / 2}, empty},
        [MASTER] = {{0, 0, w * arg, h}, {w * arg, 0, w * (1 - arg), h / 2}, {w * arg, h / 2, w * (1 - arg), h / 2}}
    };
    int area = 0;
    for(Rect r : rect[_i])
        area += r.width* r.height;
    assertEquals(area, w * h);
    for(int i = 1; i <= size; i++)
        mapArbitraryWindow();
    scan(root);
    for(WindowInfo* winInfo : getAllWindows())
        winInfo->moveToWorkspace(0);

    retile();
    int i = 0;
    for(WindowInfo* winInfo : getWorkspace(0)->getWindowStack()) {
        if(rect[_i][i] == empty)
            i--;
        assertEquals(rect[_i][i++], getRealGeometry(winInfo));
    }
});

MPX_TEST("raise_focused", {
    setActiveLayout(getDefaultLayouts()[FULL]);
    assert(getActiveLayout()->getArgs().raiseFocused);
    Window ids[] = {mapArbitraryWindow(), mapArbitraryWindow(), mapArbitraryWindow()};
    scan(root);
    for(WindowInfo* winInfo : getAllWindows())
        winInfo->moveToWorkspace(0);
    getActiveMaster()->onWindowFocus(ids[1]);
    tileWorkspace(0);
    WindowID stack[] = {ids[0], ids[2], ids[1]};
    assert(checkStackingOrder(stack, LEN(stack)));
});

MPX_TEST_ITER("test_fixed_position_windows", NUMBER_OF_LAYOUT_FAMILIES, {
    DEFAULT_BORDER_WIDTH = 0;
    mapArbitraryWindow();
    flush();
    scan(root);
    RectWithBorder config = {1, 2, 3, 4, 5};
    WindowInfo* winInfo = getAllWindows()[0];
    winInfo->moveToWorkspace(0);
    winInfo->setTilingOverrideEnabled(-1, 1);
    winInfo->setTilingOverride(config);
    setActiveLayout(&LAYOUT_FAMILIES[_i]);
    tileWorkspace(getActiveWorkspaceIndex());
    RectWithBorder actualConfig = getRealGeometry(winInfo);;
    assertEquals(config, actualConfig);
});

MPX_TEST_ITER("test_identity_transform_config", TRANSFORM_LEN * 2, {
    DEFAULT_BORDER_WIDTH = 0;
    int n = 10;
    int d = _i % 2 ? 0 : n;
    _i /= 2;
    Rect view = {.x = d, .y = d, .width = n, .height = n};
    Monitor monitor = Monitor(1, view);
    int config[CONFIG_LEN] = {d, d, n, n};
    int correctConfig[4] = {d, d, n, n};
    LayoutArgs prop = {.transform = (Transform)_i};
    transformConfig(&prop, &monitor, config);
    assert(memcmp(config, correctConfig, sizeof(correctConfig)) == 0);
});
MPX_TEST_ITER("test_simple_transform_config", TRANSFORM_LEN, {
    DEFAULT_BORDER_WIDTH = 0;
    Monitor monitor = Monitor(1, {0, 0, 0, 0});
    for(int i = 1; i < 5; i++) {
        int config[TRANSFORM_LEN] = {i % 2, i / 2, 0, 0};
        const int correctConfig[TRANSFORM_LEN][2] = {
            [NONE] = {config[0], config[1]},
            [ROT_180] = {-config[0], -config[1]},
            [REFLECT_HOR] = {-config[0], config[1]},
            [REFLECT_VERT] = {config[0], -config[1]},
        };
        LayoutArgs prop = {.transform = (Transform)_i};
        transformConfig(&prop, &monitor, config);
        assert(memcmp(config, correctConfig[_i], sizeof(correctConfig[_i])) == 0);
    }
});
MPX_TEST("test_tile_input_only_window", {
    CRASH_ON_ERRORS = -1;
    WindowID win = mapWindow(createInputOnlyWindow());
    scan(root);
    WindowInfo* winInfo = getWindowInfo(win);
    winInfo->moveToWorkspace(getActiveWorkspaceIndex());
    assert(winInfo->hasMask(INPUT_ONLY_MASK));
    assert(winInfo->isTileable());
    assert(getActiveWindowStack().size() == 1);
    setActiveLayout(getDefaultLayouts()[GRID]);
    assert(getActiveLayout()->getArgs().noBorder == 0);
    retile();
});


MPX_TEST("test_empty_layout", {
    mapWindow(createNormalWindow());
    scan(root);
    for(int i = 0; i < 2; i++) {
        setActiveLayout(NULL);
        consumeEvents();
        tileWorkspace(getActiveWorkspaceIndex());
        assert(!xcb_poll_for_event(dis));
        Layout l = {"", NULL};
        setActiveLayout(&l);
        tileWorkspace(getActiveWorkspaceIndex());
        assert(!xcb_poll_for_event(dis));
        getAllWindows()[0]->moveToWorkspace(getActiveWorkspaceIndex());
    }
});
MPX_TEST("floating_windows", {
    WindowID win = mapWindow(createNormalWindow());
    WindowID win2 = mapWindow(createInputOnlyWindow());
    WindowID win3 = mapWindow(createNormalWindow());
    scan(root);
    for(WindowInfo* winInfo : getAllWindows()) {
        winInfo->moveToWorkspace(getActiveWorkspaceIndex());
        floatWindow(winInfo);
    }
    getWindowInfo(win3)->setTilingOverrideEnabled(1 << CONFIG_INDEX_BORDER);
    int customBorder = 5;
    getWindowInfo(win3)->setTilingOverride({0, 0, 0, 0, customBorder});
    retile();
    assertEquals(getRealGeometry(getWindowInfo(win3)).border, customBorder);
    assertEquals(getRealGeometry(getWindowInfo(win2)).border, 0);
    assertEquals(getRealGeometry(getWindowInfo(win)).border, DEFAULT_BORDER_WIDTH);
});
/* TODO test transients
MPX_TEST("test_transient_windows", {
    WindowID stackingOrder[5] = {0,0,1,1};
    int transIndex = 3;
    int transTarget = 0;
    assert(transTarget < transIndex);
    WindowInfo* transWin;
    for(int i = 0, count = 0; count < LEN(stackingOrder); count++){
        WindowID win = mapWindow(createNormalWindow());
        WindowInfo* winInfo = new WindowInfo(win);
        addWindowInfo(winInfo);
        winInfo->addMask( ABOVE_MASK);
        if(i != transIndex || stackingOrder[transTarget + 1] != 0){
            stackingOrder[i] = win;
            winInfo->moveToWorkspace( getActiveWorkspaceIndex());
            getActiveWindowStack().shiftToHead(getActiveWindowStack().size() - 1);
            if(i == transTarget)i++;
            i++;
        }
        else {
            stackingOrder[transTarget + 1] = win;
            xcb_icccm_set_wm_transient_for(dis, win, stackingOrder[transTarget]);
            loadWindowProperties(transWin = winInfo);
            assert(transWin->transientFor);
        }
    }
    assert(transWin);
    moveWindowToWorkspace(transWin, getActiveWorkspaceIndex());
    shiftToHead(getActiveWindowStack(), getSize(getActiveWindowStack()) - 1);
    retile();
    flush();
    assert(checkStackingOrder(stackingOrder, LEN(stackingOrder)));
}
        );
MPX_TEST("test_transient_windows_always_above", {
    DEFAULT_WINDOW_MASKS = MAPPABLE_MASK | MAPPED_MASK;
    WindowInfo* winInfo = new WindowInfo(mapWindow(createNormalWindow()));
    registerWindow(winInfo);
    winInfo->moveWindowToWorkspace( getActiveWorkspaceIndex());
    WindowInfo* winInfo2 = new WindowInfo(mapWindow(createNormalWindow()));
    registerWindow(winInfo2);
    xcb_icccm_set_wm_transient_for(dis, winInfo2->getID(), winInfo->getID());
    loadWindowProperties(winInfo2);
    assert(winInfo2->transientFor == winInfo->getID());
    moveWindowToWorkspace(winInfo2, getActiveWorkspaceIndex());
    assert(isActivatable(winInfo)&& isActivatable(winInfo2));
    assert(isInteractable(winInfo2));
    for(int i = 0; i < 2; i++){
        assert(checkStackingOrder((WindowID[]){
            winInfo->getID(), winInfo2->getID()
        }, 2));
        activateWindow(winInfo);
    }
}
        );
*/

MPX_TEST("test_tile_windows", {
    setActiveLayout(NULL);
    //retile empty workspace
    for(int i = 0; i < getNumberOfWorkspaces(); i++)
        tileWorkspace(i);
    //bottom to top stacking order;
    WindowID stackingOrder[3] = {0};
    int masks[] = {BELOW_MASK, 0, ABOVE_MASK};
    for(int i = 0; i < LEN(stackingOrder); i++) {
        WindowID win = mapWindow(createNormalWindow());
        WindowInfo* winInfo = new WindowInfo(win);
        addWindowInfo(winInfo);
        winInfo->moveToWorkspace(getActiveWorkspaceIndex());
        winInfo->addMask(masks[i]);
        stackingOrder[i] = win;
        lowerWindowInfo(winInfo);
    }
    retile();
    flush();
    assert(checkStackingOrder(stackingOrder, LEN(stackingOrder)));
});


MPX_TEST_ITER("test_privileged_windows_size", 5, {
    Monitor* m = getAllMonitors()[0];
    assert(m);
    Rect view = {10, 20, 30, 40};
    m->setViewport(view);
    WindowInfo* winInfo = new WindowInfo(mapWindow(createNormalWindow()));
    addWindowInfo(winInfo);
    winInfo->moveToWorkspace(getActiveWorkspaceIndex());
    static RectWithBorder baseConfig = {20, 30, 40, 50};
    void (*dummyLayout)(LayoutState * state) = [](LayoutState * state) {
        configureWindow(state, state->stack[0], baseConfig);
    };
    Layout l = {"", .layoutFunction = dummyLayout, {.noBorder = 1}};
    setActiveLayout(&l);
    struct{
        WindowMask mask;
        const Rect dims;
    } arr[] = {
        {0, baseConfig},
        {X_MAXIMIZED_MASK, {baseConfig.x, baseConfig.y, m->getViewport().width, baseConfig.height}},
        {Y_MAXIMIZED_MASK, {baseConfig.x, baseConfig.y, baseConfig.width, m->getViewport().height}},
        {FULLSCREEN_MASK, m->getBase()},
        {ROOT_FULLSCREEN_MASK, {0, 0, getRootWidth(), getRootHeight()}},
    };
    int baseMask = MAPPABLE_MASK | MAPPED_MASK;
    int i = _i;
    winInfo->removeMask(-1);
    winInfo->addMask(baseMask | arr[i].mask);
    retile();
    Rect rect = getRealGeometry(winInfo);
    assertEquals(arr[i].dims, rect);
});

MPX_TEST("find_layout_by_name", {
    assert(getDefaultLayouts()[FULL] == findLayoutByName(getDefaultLayouts()[FULL]->getName()));
});

MPX_TEST("test_get_layout_by_name", {
    assert(findLayoutByName("__bad__layout__name") == NULL);
});
MPX_TEST("test_set_layout_by_name", {
    setActiveLayoutByName(getDefaultLayouts()[FULL]->getName());
    assert(getActiveLayout() == getDefaultLayouts()[FULL]);
    setActiveLayoutByName("__bad__layout__name");
    assert(getActiveLayout() == NULL);
});
MPX_TEST_ITER("test_layout_arg_change", LAYOUT_ARG + 1, {
    setActiveLayout(getDefaultLayouts()[0]);
    getActiveLayout()->getArgs().argStep = 1;
    LayoutArgs arg = getActiveLayout()->getArgs();
    increaseLayoutArg(_i, 1);
    assert(getActiveLayout() == getDefaultLayouts()[0]);
    assert(memcmp(&arg, &getActiveLayout()->getArgs(), sizeof(arg)));
});
MPX_TEST("test_register_restore_layout", {
    Layout c = {NULL};
    Layout zeroLayout = c;
    setActiveLayout(&c);
    for(int i = 0; i <= LAYOUT_ARG; i++)
        increaseLayoutArg(i, i * i);
    assert(memcmp(&zeroLayout.getArgs(), &getActiveLayout()->getArgs(), sizeof(zeroLayout.getArgs())));
    getActiveLayout()->restoreArgs();
    assert(memcmp(&zeroLayout.getArgs(), &getActiveLayout()->getArgs(), sizeof(zeroLayout.getArgs())) == 0);
});
