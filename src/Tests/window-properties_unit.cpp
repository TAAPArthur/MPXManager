#include "tester.h"
#include "test-mpx-helper.h"
#include "test-x-helper.h"
#include "../globals.h"
#include "../layouts.h"
#include "../window-properties.h"
#include "../windows.h"
#include "../wmfunctions.h"
#include "../monitors.h"
#include "../state.h"
#include "../devices.h"

using namespace std;
SET_ENV(createXSimpleEnv, cleanupXServer);
MPX_TEST("test_float_sink_window", {
    WindowInfo winInfo = WindowInfo(1);
    winInfo.addMask(MAPPED_MASK | MAPPABLE_MASK);
    assert(winInfo.isTileable());
    floatWindow(&winInfo);
    assert(winInfo.hasMask(FLOATING_MASK));
    assert(!winInfo.isTileable());
    sinkWindow(&winInfo);
    assert(!winInfo.hasMask(FLOATING_MASK));
    assert(winInfo.isTileable());
});
MPX_TEST("get_set_title", {
    WindowID win = createNormalWindow();
    string title = "Title";
    setWindowTitle(win, title);
    assert(getWindowTitle(win) == title);
});
MPX_TEST("get_alt_title", {
    WindowID win = createUnmappedWindow();
    WindowInfo* winInfo = new WindowInfo(win);
    const char* name = "test";
    xcb_icccm_set_wm_name(dis, win, XCB_ATOM_STRING, 8, strlen(name), name);
    addWindowInfo(winInfo);
    loadWindowProperties(winInfo);
    assert(getWindowTitle(win) == string(name));
});
MPX_TEST("get_set_class", {
    WindowID win = createNormalWindow();
    string clazz = "Class";
    string instanceName = "Instance";
    setWindowClass(win, clazz, instanceName);
    string clazzNew;
    string instanceNameNew;
    loadClassInfo(win, &clazzNew, &instanceNameNew);
    assertEquals(clazz, clazzNew);
    assertEquals(instanceName, instanceNameNew);
});
MPX_TEST_ITER("get_set_type", 4, {
    WindowID win = createTypelessWindow();
    bool normal = _i / 2;
    bool implicit = _i % 2;
    xcb_atom_t atom = normal ? ewmh->_NET_WM_WINDOW_TYPE_NORMAL : ewmh->_NET_WM_WINDOW_TYPE_DIALOG;
    if(!implicit)
        setWindowType(win, atom);
    else if(!normal)
        setWindowTransientFor(win, createNormalWindow());
    flush();

    string typeName = getAtomValue(atom);
    xcb_atom_t atomNew;
    string typeNameNew;
    loadWindowType(win, implicit && !normal, &atomNew, &typeNameNew);
    assertEquals(typeNameNew, typeName);
    assertEquals(atomNew, atom);
    WindowInfo* winInfo = new WindowInfo(win);
    getAllWindows().add(winInfo);
    loadWindowProperties(winInfo);
    assertEquals(winInfo->getType(), atom);


});
MPX_TEST("load_properties_normal", {
    WindowID win = createNormalWindow();
    WindowInfo* winInfo = new WindowInfo(win);
    for(int i = 0; i < 2; i++) {
        string clazz = i ? "Class" : "clazz";
        string instanceName = i ? "Instance" : "int";
        string title = i ? "Title" : "name";
        xcb_atom_t atom = i ? ewmh->_NET_WM_WINDOW_TYPE_NORMAL : ewmh->_NET_WM_WINDOW_TYPE_DESKTOP;
        string typeName = i ?  "_NET_WM_WINDOW_TYPE_NORMAL" : "_NET_WM_WINDOW_TYPE_DESKTOP";
        setWindowClass(win, clazz, instanceName);
        setWindowTitle(win, title);
        setWindowType(win, &atom, 1);
        loadWindowProperties(winInfo);
        assertEquals(title, winInfo->getTitle());
        assertEquals(typeName, winInfo->getTypeName());
        assertEquals(atom, winInfo->getType());
        assertEquals(instanceName, winInfo->getInstanceName());
        assertEquals(clazz, winInfo->getClassName());
    }
    getAllWindows().add(winInfo);
});

MPX_TEST_ITER("test_load_masks_from_hints", 4, {
    WindowID win = createUnmappedWindow();
    scan(root);
    WindowInfo* winInfo = getWindowInfo(win);
    bool input = _i / 2;
    bool mapped = _i % 2;
    xcb_icccm_wm_hints_t hints = {.input = input, .initial_state = mapped ? XCB_ICCCM_WM_STATE_NORMAL : XCB_ICCCM_WM_STATE_WITHDRAWN};
    catchError(xcb_icccm_set_wm_hints_checked(dis, win, &hints));
    loadWindowProperties(winInfo);
    assert(winInfo->hasMask(INPUT_MASK) == input);
    assert(winInfo->hasMask(MAPPABLE_MASK) == mapped);
});

MPX_TEST("load_protocols", {
    WindowID win = createUnmappedWindow();
    WindowInfo* winInfo = new WindowInfo(win);
    getAllWindows().add(winInfo);
    xcb_atom_t atoms[] = {WM_TAKE_FOCUS, WM_DELETE_WINDOW, ewmh->_NET_WM_PING};
    xcb_icccm_set_wm_protocols(dis, win, ewmh->WM_PROTOCOLS, 3, atoms);
    loadWindowProperties(winInfo);
    assert(winInfo->hasMask(WM_DELETE_WINDOW_MASK));
    assert(winInfo->hasMask(WM_TAKE_FOCUS_MASK));
    assert(winInfo->hasMask(WM_PING_MASK));
});

MPX_TEST("test_window_extra_property_loading", {
    WindowID win = createUnmappedWindow();
    int groupLead = createUnmappedWindow();
    int trasientForWindow = createUnmappedWindow();
    WindowInfo* winInfo = new WindowInfo(win);
    xcb_icccm_wm_hints_t hints;
    xcb_icccm_wm_hints_set_window_group(&hints, groupLead);
    xcb_icccm_set_wm_hints(dis, win, &hints);
    xcb_icccm_set_wm_transient_for(dis, win, trasientForWindow);
    flush();
    addWindowInfo(winInfo);
    loadWindowProperties(winInfo);
    assert(winInfo->getGroup() == groupLead);
    assert(winInfo->getTransientFor() == trasientForWindow);
});
MPX_TEST("ignore_small_window", {
    WindowID win = mapArbitraryWindow();
    xcb_size_hints_t hints;
    xcb_icccm_size_hints_set_size(&hints, 0, 1, 1);
    assert(!catchError(xcb_icccm_set_wm_size_hints_checked(dis, win, XCB_ATOM_WM_NORMAL_HINTS, &hints)));
    WindowInfo* winInfo = new WindowInfo(win);
    addWindowInfo(winInfo);
    loadWindowProperties(winInfo);
    assert(getWindowSizeHints(winInfo));
});
MPX_TEST("test_set_border_color", {
    WindowID win = mapWindow(createNormalWindow());
    unsigned int colors[] = {0, 255, 255 * 255, 255 * 255 * 255};
    for(int i = 0; i < LEN(colors); i++) {
        setBorderColor(win, colors[i]);
        flush();
    }
    setBorderColor(win, -1);
    setBorder(win);
    resetBorder(win);
    //TODO check to see if border was set correctly
});
MPX_TEST("reset_border_color", {
    createMasterDevice("test");
    initCurrentMasters();
    WindowID win = mapWindow(createNormalWindow());
    getAllWindows().add(new WindowInfo(win));
    for(Master* m : getAllMasters())
        m->onWindowFocus(win);
    resetBorder(win);
    //TODO check to see if border was set correctly
});
MPX_TEST_ITER("test_focus_window", 4, {
    createMasterDevice("test");
    initCurrentMasters();
    Master* m = getAllMasters()[_i % 2];
    WindowID win = mapArbitraryWindow();
    if(_i / 2) {
        WindowInfo* winInfo = new WindowInfo(win);
        winInfo->addMask(INPUT_MASK | MAPPABLE_MASK | MAPPED_MASK);
        getAllWindows().add(winInfo);
        assert(focusWindow(winInfo, m));
    }
    else
        assert(focusWindow(win, m));

    flush();
    assertEquals(win, getActiveFocus(m->getKeyboardID()));
});
MPX_TEST("test_focus_inputless_window", {
    WindowInfo winInfo = WindowInfo(1);
    winInfo.removeMask(INPUT_MASK);
    assert(focusWindow(&winInfo) == 0);
});
MPX_TEST("test_wm_take_focus", {
    WindowInfo winInfo = WindowInfo(1);
    winInfo.addMask(INPUT_MASK | WM_TAKE_FOCUS_MASK);
    assert(focusWindow(&winInfo));
});

MPX_TEST("test_focus_window_bad", {
    CRASH_ON_ERRORS = 0;
    setLogLevel(LOG_LEVEL_NONE);
    /// TODO reverse return value
    assert(focusWindow(createNormalWindow()) == 0);
});

static inline WindowID setDock(WindowID win, int i, int size, bool full = 0, int start = 0, int end = 0) {
    assert(i >= 0);
    assert(i < 4);
    int strut[12] = {0};
    strut[i] = size;
    strut[i * 2 + 4] = start;
    strut[i * 2 + 4 + 1] = end;
    //strut[i*2+4+1]=0;
    xcb_void_cookie_t cookie = full ?
                               xcb_ewmh_set_wm_strut_partial_checked(ewmh, win, *((xcb_ewmh_wm_strut_partial_t*)strut)) :
                               xcb_ewmh_set_wm_strut_checked(ewmh, win, strut[0], strut[1], strut[2], strut[3]);
    assert(xcb_request_check(dis, cookie) == NULL);
    setWindowType(win, ewmh->_NET_WM_WINDOW_TYPE_DOCK);
    return win;
}
MPX_TEST_ITER("docks", 4 * 2, {
    WindowID win = createNormalWindow();
    int i = _i / 2;
    int full = _i % 2;
    int size = 10;
    int start = full ? 1 : 0;
    int end = full ? 2 : 0;
    mapWindow(win);
    scan(root);
    WindowInfo* winInfo = getWindowInfo(win);
    winInfo->setDock();
    loadDockProperties(winInfo);
    auto* prop = getWindowInfo(win)->getDockProperties();
    if(prop) {
        for(int i = 0; i < 4; i++)
            assert(prop[i] == 0);
    }
    setDock(win, i, size, full, start, end);
    loadWindowProperties(winInfo);
    prop = getWindowInfo(win)->getDockProperties();
    assert(prop);
    assertEquals(prop[i], size);
    assertEquals(prop[i * 2 + 4], start);
    assertEquals(prop[i * 2 + 5], end);
});
