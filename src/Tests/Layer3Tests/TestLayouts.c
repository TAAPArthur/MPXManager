#include "../UnitTests.h"
#include "../TestX11Helper.h"

#include "../../layouts.h"
#include "../../wmfunctions.h"
#include "../../functions.h"
#include "../../monitors.h"
#include "../../state.h"
#include "../../devices.h"

START_TEST(test_layout_add){
    addFakeMaster(1, 1);
    Layout* l = calloc(1, sizeof(Layout));
    for(int n = 0; n < getNumberOfWorkspaces(); n++){
        setActiveWorkspaceIndex(n);
        for(int i = 1; i <= 10; i++){
            addLayoutsToWorkspace(n, l, 1);
            assert(getSize(&getActiveWorkspace()->layouts) == i);
        }
    }
    for(int n = 0; n < getNumberOfWorkspaces(); n++){
        clearLayoutsOfWorkspace(n);
    }
    free(l);
}
END_TEST
START_TEST(test_layout_add_multiple){
    addFakeMaster(1, 1);
    int size = 10;
    Layout* l = calloc(size, sizeof(Layout));
    addLayoutsToWorkspace(getActiveWorkspaceIndex(), l, size);
    assert(getSize(&getActiveWorkspace()->layouts) == size);
    int i = 0;
    FOR_EACH(Layout*, layout, &getActiveWorkspace()->layouts) assert(layout == &l[i++]);
    free(l);
}
END_TEST
START_TEST(test_active_layout){
    addFakeMaster(1, 1);
    int size = 10;
    Layout* l = calloc(size, sizeof(Layout));
    for(int i = 0; i < size; i++){
        setActiveLayout(&l[i]);
        assert(getActiveLayout() == &l[i]);
    }
    free(l);
}
END_TEST

START_TEST(test_layout_name){
    addFakeMaster(1, 1);
    Layout* l = calloc(1, sizeof(Layout));
    getNameOfLayout(NULL);//should not crash
    char* name = "test";
    l->name = name;
    assert(strcmp(getNameOfLayout(l), name) == 0);
    free(l);
}
END_TEST

START_TEST(test_layouts){
    DEFAULT_BORDER_WIDTH = 0;
    Monitor* m = getHead(getAllMonitors());
    int targetArea = m->view.width * m->view.height;
    int layoutIndex = _i;
    setActiveLayout(&LAYOUT_FAMILIES[layoutIndex]);
    int size = 5;
    assert(!isNotEmpty(getAllWindows()));
    retile();
    START_MY_WM;
    int area;
    xcb_get_geometry_reply_t* reply1, *reply2;
    ArrayList* list = getActiveWindowStack();
    for(int i = 1; i <= size; i++){
        lock();
        createNormalWindow();
        flush();
        int idle = getIdleCount();
        unlock();
        WAIT_UNTIL_TRUE(getNumberOfWindowsToTile(getActiveWindowStack(), NULL) >= i);
        WAIT_UNTIL_TRUE(idle != getIdleCount());
        msleep(50);
        lock();
        retile();
        assert(getNumberOfWindowsToTile(getActiveWindowStack(), NULL) == i);
        assert(getNumberOfWindowsToTile(getActiveWindowStack(), NULL) == getSize(getActiveWindowStack()));
        area = 0;
        FOR_EACH(WindowInfo*, winInfo, getActiveWindowStack()){
            reply1 = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, winInfo->id), NULL);
            if(isWindowVisible(winInfo)){
                area += reply1->width * reply1->height;
            }
            free(reply1);
        }
        assert(area == targetArea);
        int count = 0;
        for(int i = 0; i < getSize(list); i++){
            count++;
            if(isWindowVisible(getElement(list, i))){
                reply1 = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, ((WindowInfo*)getElement(list, i))->id), NULL);
                for(int n = i + 1; n < getSize(list); n++)
                    if(isWindowVisible(getElement(list, n))){
                        reply2 = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, ((WindowInfo*)getElement(list, n))->id), NULL);
                        assert(intersects(*(Rect*)&reply1->x, *(Rect*)&reply2->x) == 0);
                        free(reply2);
                    }
                free(reply1);
            }
        }
        unlock();
    }
    lock();
    int limit = 3;
    getActiveLayout()->args.limit = limit;
    retile();
    reply1 = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, ((WindowInfo*)getElement(list, size - 1))->id), NULL);
    reply2 = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, ((WindowInfo*)getElement(list, limit))->id), NULL);
    assert(memcmp(&reply1->x, &reply2->x, sizeof(reply1->x) * 4) == 0);
    free(reply1);
    free(reply2);
    unlock();
}
END_TEST

START_TEST(test_layouts_transition){
    AUTO_FOCUS_NEW_WINDOW_TIMEOUT = -1;
    LOAD_SAVED_STATE = 0;
    int layoutIndex = _i;
    setActiveLayout(&LAYOUT_FAMILIES[layoutIndex]);
    int size = 10;
    int startingWorkspace = 0;
    switchToWorkspace(startingWorkspace);
    int numOfMasters = 5;
    for(int i = 0; i < numOfMasters - 1; i++)
        createMasterDevice((char[]){
        'A' + i, 0
    });
    initCurrentMasters();
    WindowInfo* markedWindows[numOfMasters];
    Master* masterList[numOfMasters];
    ArrayList* masters = getAllMasters();
    int i = 0;
    int idle;
    FOR_EACH(Master*, master, masters) masterList[i++] = master;
    START_MY_WM;
    lock();
    for(int i = 0; i < size; i++)
        createNormalWindow();
    flush();
    unlock();
    WAIT_UNTIL_TRUE(getNumberOfWindowsToTile(getActiveWindowStack(), NULL) == size);
    lock();
    ArrayList* stack = getActiveWindowStack();
    i = 0;
    FOR_EACH_REVERSED(WindowInfo*, winInfo, stack) markedWindows[i++] = winInfo;
    unlock();
    for(int i = 0; i < numOfMasters; i++){
        lock();
        setActiveMaster(masterList[i]);
        activateWindow(markedWindows[i]);
        flush();
        unlock();
        idle = getIdleCount();
        WAIT_UNTIL_TRUE(getIdleCount() != idle);
    }
    lock();
    switchToWorkspace(!startingWorkspace);
    assert(getActiveWorkspaceIndex() != startingWorkspace);
    assert(!isNotEmpty(getActiveWindowStack()));
    createNormalWindow();
    idle = getIdleCount();
    unlock();
    WAIT_UNTIL_TRUE(getIdleCount() != idle);
    assert(getActiveWorkspaceIndex() != startingWorkspace);
    lock();
    WindowInfo* tempWindow = (getHead(getActiveWindowStack()));
    for(int i = 0; i < numOfMasters; i++){
        setActiveMaster(masterList[i]);
        focusWindowInfo(tempWindow);
    }
    flush();
    idle = getIdleCount();
    unlock();
    WAIT_UNTIL_TRUE(getIdleCount() != idle);
    lock();
    for(int i = 0; i < numOfMasters; i++){
        assert(getFocusedWindowByMaster(masterList[i]) == tempWindow);
    }
    switchToWorkspace(startingWorkspace);
    assert(getActiveWorkspaceIndex() == startingWorkspace);
    flush();
    idle = getIdleCount();
    unlock();
    WAIT_UNTIL_TRUE(getIdleCount() != idle);
    for(int i = 0; i < numOfMasters; i++){
        assert(getFocusedWindowByMaster(masterList[i]) == markedWindows[i]);
    }
    for(int i = 0; i < numOfMasters; i++)
        destroyMasterDevice(masterList[i]->id, 2, 3);
    flush();
    WAIT_UNTIL_TRUE(getSize(getAllMasters()) == 1);
}
END_TEST
START_TEST(test_fixed_position_windows){
    DEFAULT_BORDER_WIDTH = 0;
    int size = 4;
    for(int i = 1; i <= size; i++)
        mapWindow(createNormalWindow());
    flush();
    scan(root);
    short config[] = {1, 2, 3, 4, 5};
    WindowInfo* winInfo = getHead(getAllWindows());
    assert(LEN(config) == LEN(winInfo->config));
    setConfig(winInfo, config);
    setActiveLayout(&LAYOUT_FAMILIES[_i]);
    tileWorkspace(getActiveWorkspaceIndex());
    xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, winInfo->id), NULL);
    assert(memcmp(config, &reply->x, sizeof(short)*LEN(config)) == 0);
    free(reply);
}
END_TEST

START_TEST(test_identity_transform_config){
    DEFAULT_BORDER_WIDTH = 0;
    int n = 10;
    int d = _i % 2 ? 0 : n;
    _i /= 2;
    Monitor monitor = {.view.x = d, .view.y = d, .view.width = n, .view.height = n};
    int config[CONFIG_LEN] = {d, d, n, n};
    int correctConfig[4] = {d, d, n, n};
    LayoutArgs prop = {.transform = _i};
    LayoutState state = {.args = &prop, .monitor = &monitor};
    transformConfig(&state, config);
    assert(memcmp(config, correctConfig, sizeof(correctConfig)) == 0);
}
END_TEST
START_TEST(test_simple_transform_config){
    DEFAULT_BORDER_WIDTH = 0;
    Monitor monitor = {.view.x = 0, .view.y = 0, .view.width = 0, .view.height = 0};
    Monitor* m = &monitor;
    for(int i = 1; i < 5; i++){
        int config[CONFIG_LEN] = {i % 2, i / 2, 0, 0};
        const int correctConfig[CONFIG_LEN][2] = {
            [NONE] = {config[0], config[1]},
            [REFLECT_HOR] = {-config[0], config[1]},
            [REFLECT_VERT] = {config[0], -config[1]},
            [ROT_180] = {-config[0], -config[1]},
            [ROT_90] = {-config[1], config[0]},
            [ROT_270] = {config[1], -config[0]},
        };
        LayoutArgs prop = {.transform = _i};
        LayoutState state = {.args = &prop, .monitor = m};
        transformConfig(&state, config);
        assert(memcmp(config, correctConfig[_i], sizeof(correctConfig[_i])) == 0);
    }
}
END_TEST
START_TEST(test_tile_input_only_window){
    CRASH_ON_ERRORS = -1;
    WindowID win = mapWindow(createInputOnlyWindow());
    scan(root);
    WindowInfo* winInfo = getWindowInfo(win);
    assert(hasMask(winInfo, INPUT_ONLY_MASK));
    assert(isTileable(winInfo));
    assert(getSize(getActiveWindowStack()) == 1);
    setActiveLayout(&DEFAULT_LAYOUTS[GRID]);
    assert(getActiveLayout()->args.noBorder == 0);
    retile();
}
END_TEST


START_TEST(test_empty_layout){
    LOAD_SAVED_STATE = 0;
    mapWindow(createNormalWindow());
    scan(root);
    setActiveLayout(NULL);
    consumeEvents();
    tileWorkspace(getActiveWorkspaceIndex());
    assert(!xcb_poll_for_event(dis));
    Layout l = {};
    setActiveLayout(&l);
    tileWorkspace(getActiveWorkspaceIndex());
    assert(!xcb_poll_for_event(dis));
    void dummy(){
        exit(1);
    }
    l.layoutFunction = NULL;
    setActiveLayout(&l);
    tileWorkspace(getActiveWorkspaceIndex());
    assert(!xcb_poll_for_event(dis));
}
END_TEST
START_TEST(test_transient_windows){
    WindowID stackingOrder[5] = {0};
    int transIndex = 3;
    int transTarget = 0;
    assert(transTarget < transIndex);
    WindowInfo* transWin;
    for(int i = 0, count = 0; count < LEN(stackingOrder); count++){
        WindowID win = mapWindow(createNormalWindow());
        WindowInfo* winInfo = createWindowInfo(win);
        addWindowInfo(winInfo);
        addMask(winInfo, ABOVE_MASK);
        if(i != transIndex || stackingOrder[transTarget + 1] != 0){
            stackingOrder[i] = win;
            moveWindowToWorkspace(winInfo, getActiveWorkspaceIndex());
            shiftToHead(getActiveWindowStack(), getSize(getActiveWindowStack()) - 1);
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
END_TEST
START_TEST(test_transient_windows_always_above){
    DEFAULT_WINDOW_MASKS = MAPPABLE_MASK | MAPPED_MASK;
    WindowInfo* winInfo = createWindowInfo(mapWindow(createNormalWindow()));
    processNewWindow(winInfo);
    moveWindowToWorkspace(winInfo, getActiveWorkspaceIndex());
    WindowInfo* winInfo2 = createWindowInfo(mapWindow(createNormalWindow()));
    processNewWindow(winInfo2);
    xcb_icccm_set_wm_transient_for(dis, winInfo2->id, winInfo->id);
    loadWindowProperties(winInfo2);
    assert(winInfo2->transientFor == winInfo->id);
    moveWindowToWorkspace(winInfo2, getActiveWorkspaceIndex());
    assert(isActivatable(winInfo) && isActivatable(winInfo2));
    assert(isInteractable(winInfo2));
    for(int i = 0; i < 2; i++){
        assert(checkStackingOrder((WindowID[]){
            winInfo->id, winInfo2->id
        }, 2));
        activateWindow(winInfo);
    }
}
END_TEST
START_TEST(test_tile_windows){
    setActiveLayout(NULL);
    //retile empty workspace
    for(int i = 0; i < getNumberOfWorkspaces(); i++)
        tileWorkspace(i);
    //bottom to top stacking order;
    WindowID stackingOrder[3] = {0};
    int masks[] = {BELOW_MASK, 0, ABOVE_MASK};
    for(int i = 0; i < LEN(stackingOrder); i++){
        WindowID win = mapWindow(createNormalWindow());
        WindowInfo* winInfo = createWindowInfo(win);
        addWindowInfo(winInfo);
        moveWindowToWorkspace(winInfo, getActiveWorkspaceIndex());
        addMask(winInfo, masks[i]);
        stackingOrder[i] = win;
    }
    raiseWindow(stackingOrder[0]);
    retile();
    flush();
    assert(checkStackingOrder(stackingOrder, LEN(stackingOrder)));
}
END_TEST

START_TEST(test_layout_toggle){
    assert(getActiveLayout() == &DEFAULT_LAYOUTS[0]);
    assert(toggleLayout(&DEFAULT_LAYOUTS[1]));
    assert(getActiveLayout() == &DEFAULT_LAYOUTS[1]);
    assert(toggleLayout(&DEFAULT_LAYOUTS[1]));
    assert(getActiveLayout() == &DEFAULT_LAYOUTS[0]);
    assert(toggleLayout(&DEFAULT_LAYOUTS[0]) == 0);
    assert(getActiveLayout() == &DEFAULT_LAYOUTS[0]);
}
END_TEST

START_TEST(test_layout_cycle){
    clearLayoutsOfWorkspace(getActiveWorkspaceIndex());
    addLayoutsToWorkspace(getActiveWorkspaceIndex(), DEFAULT_LAYOUTS, 2);
    assert(getActiveLayout() == &DEFAULT_LAYOUTS[0]);
    cycleLayouts(3);
    assert(getActiveLayout() == &DEFAULT_LAYOUTS[1]);
    cycleLayouts(1);
    assert(getActiveLayout() == &DEFAULT_LAYOUTS[0]);
}
END_TEST
START_TEST(test_layout_cycle_reverse){
    clearLayoutsOfWorkspace(getActiveWorkspaceIndex());
    addLayoutsToWorkspace(getActiveWorkspaceIndex(), DEFAULT_LAYOUTS, 2);
    assert(getActiveLayout() == &DEFAULT_LAYOUTS[0]);
    cycleLayouts(-1);
    assert(getActiveLayout() == &DEFAULT_LAYOUTS[1]);
    cycleLayouts(-1);
    assert(getActiveLayout() == &DEFAULT_LAYOUTS[0]);
}
END_TEST
START_TEST(test_retile){
    int count = 0;
    void dummy(){
        count++;
    }
    Layout l = {.layoutFunction = dummy};
    mapWindow(createNormalWindow());
    scan(root);
    WAIT_UNTIL_TRUE(isNotEmpty(getActiveWindowStack()));
    setActiveLayout(&l);
    retile();
    assert(count == 1);
}
END_TEST

START_TEST(test_privileged_windows_size){
    Monitor* m = getHead(getAllMonitors());
    assert(m);
    Rect view = {10, 20, 30, 40};
    m->view = view;
    WindowInfo* winInfo = createWindowInfo(mapWindow(createNormalWindow()));
    addWindowInfo(winInfo);
    addWindowToWorkspace(winInfo, getActiveWorkspaceIndex());
    static short baseConfig[CONFIG_LEN] = {20, 30, 40, 50};
    void dummyLayout(LayoutState * state){
        configureWindow(state, getHead(state->stack), baseConfig);
    }
    Layout l = {.layoutFunction = dummyLayout, .args.noBorder = 1};
    setActiveLayout(&l);
    short arr[][5] = {
        {0, baseConfig[0], baseConfig[1], baseConfig[2], baseConfig[3]},
        {X_MAXIMIZED_MASK, baseConfig[0], baseConfig[1], m->view.width, baseConfig[3]},
        {Y_MAXIMIZED_MASK, baseConfig[0], baseConfig[1], baseConfig[2], m->view.height},
        {FULLSCREEN_MASK, m->base.x, m->base.y, m->base.width, m->base.height},
        {ROOT_FULLSCREEN_MASK, 0, 0, getRootWidth(), getRootHeight()},
    };
    xcb_get_geometry_reply_t* reply;
    int baseMask = MAPPABLE_MASK | MAPPED_MASK;
    int i = _i;
    removeMask(winInfo, -1);
    addMask(winInfo, baseMask | arr[i][0]);
    retile();
    reply = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, winInfo->id), NULL);
    assert(memcmp(&reply->x, &arr[i][1], sizeof(Rect)) == 0);
    free(reply);
}
END_TEST

START_TEST(test_raise_lower_request){
    WindowInfo* winInfo[5];
    WindowID win[5];
    WindowID finalStackingOrder[LEN(win)];
    DEFAULT_WINDOW_MASKS = BELOW_MASK | EXTERNAL_RAISE_MASK;
    setActiveLayout(NULL);
    for(int i = 0; i < LEN(winInfo); i++){
        win[i] = mapWindow(createNormalWindow());
        winInfo[i] = createWindowInfo(win[i]);
        finalStackingOrder[i] = win[i];
    }
    for(int i = LEN(winInfo) - 1; i >= 0; i--)
        processNewWindow(winInfo[i]);
    finalStackingOrder[0] = win[LEN(win) - 1];
    finalStackingOrder[LEN(win) - 1] = win[0];
    assert(checkStackingOrder(win, LEN(win)));
    retile();
    assert(checkStackingOrder(win, LEN(win)));
    assert(getSize(getActiveWindowStack()) == LEN(winInfo));
    int idle = getIdleCount();
    if(!fork()){
        openXDisplay();
        raiseLowerWindowInfo(winInfo[0], 1);
        raiseLowerWindowInfo(winInfo[LEN(win) - 1], 0);
        flush();
        exit(0);
    }
    waitForCleanExit();
    START_MY_WM;
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    assert(checkStackingOrder(finalStackingOrder, LEN(win)));
    ATOMIC(retile(); flush());
    assert(!checkStackingOrder(win, LEN(win)));
    assert(checkStackingOrder(finalStackingOrder, LEN(win)));
}
END_TEST

Suite* layoutSuite(){
    Suite* s = suite_create("Layouts");
    TCase* tc_core;
    tc_core = tcase_create("Layout Interface");
    tcase_add_checked_fixture(tc_core, createSimpleContext, resetContext);
    tcase_add_test(tc_core, test_layout_add);
    tcase_add_test(tc_core, test_layout_add_multiple);
    tcase_add_test(tc_core, test_active_layout);
    tcase_add_test(tc_core, test_layout_name);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Layout Args");
    tcase_add_loop_test(tc_core, test_identity_transform_config, 0, TRANSFORM_LEN * 2);
    tcase_add_loop_test(tc_core, test_simple_transform_config, 0, TRANSFORM_LEN);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Layout_Functions");
    tcase_add_checked_fixture(tc_core, onStartup, fullCleanup);
    tcase_add_test(tc_core, test_layout_toggle);
    tcase_add_test(tc_core, test_layout_cycle);
    tcase_add_test(tc_core, test_layout_cycle_reverse);
    tcase_add_test(tc_core, test_retile);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Tiling");
    tcase_add_checked_fixture(tc_core, onStartup, fullCleanup);
    tcase_add_test(tc_core, test_tile_windows);
    tcase_add_test(tc_core, test_transient_windows);
    tcase_add_test(tc_core, test_transient_windows_always_above);
    tcase_add_test(tc_core, test_tile_input_only_window);
    tcase_add_test(tc_core, test_empty_layout);
    tcase_add_loop_test(tc_core, test_privileged_windows_size, 0, 5);
    tcase_add_test(tc_core, test_raise_lower_request);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Layouts");
    tcase_add_checked_fixture(tc_core, onStartup, fullCleanup);
    tcase_add_loop_test(tc_core, test_layouts, 0, NUMBER_OF_LAYOUT_FAMILIES);
    tcase_add_loop_test(tc_core, test_layouts_transition, 0, NUMBER_OF_LAYOUT_FAMILIES);
    tcase_add_loop_test(tc_core, test_fixed_position_windows, 0, NUMBER_OF_LAYOUT_FAMILIES);
    suite_add_tcase(s, tc_core);
    return s;
}
