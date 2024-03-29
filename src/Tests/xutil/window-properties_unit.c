#include "../../devices.h"
#include "../../globals.h"
#include "../../monitors.h"
#include "../../windows.h"
#include "../../xutil/window-properties.h"
#include "../test-event-helper.h"
#include "../test-mpx-helper.h"
#include "../test-x-helper.h"
#include "../tester.h"

/*
SET_ENV(createXSimpleEnv, cleanupXServer);
SCUTEST(test_float_sink_window) {
    WindowInfo winInfo = WindowInfo(1);
    winInfo.addMask(MAPPED_MASK | MAPPABLE_MASK);
    assert(winInfo.isTileable());
    floatWindow(&winInfo);
    assert(winInfo.hasMask(FLOATING_MASK));
    assert(!winInfo.isTileable());
    sinkWindow(&winInfo);
    assert(!(winInfo.getMask() & FLOATING_MASK));
    assert(winInfo.isTileable());
}
SCUTEST(get_set_title) {
    WindowID win = createNormalWindow();
    string title = "Title";
    setWindowTitle(win, title);
    assert(getWindowTitle(win) == title);
}
SCUTEST(get_alt_title) {
    WindowID win = createUnmappedWindow();
    WindowInfo* winInfo = new WindowInfo(win);
    const char* name = "test";
    xcb_icccm_set_wm_name(dis, win, XCB_ATOM_STRING, 8, strlen(name), name);
    addWindowInfo(winInfo);
    loadWindowProperties(winInfo);
    assert(getWindowTitle(win) == string(name));
}
SCUTEST(get_set_class) {
    WindowID win = createNormalWindow();
    string clazz = "Class";
    string instanceName = "Instance";
    setWindowClass(win, clazz, instanceName);
    string clazzNew;
    string instanceNameNew;
    loadClassInfo(win, &clazzNew, &instanceNameNew);
    assertEquals(clazz, clazzNew);
    assertEquals(instanceName, instanceNameNew);
}
SCUTEST(get_set_role) {
    WindowID win = createNormalWindow();
    string role = "Role";
    setWindowRole(win, role);
    WindowInfo* winInfo = new WindowInfo(win);
    getAllWindows().add(winInfo);
    loadWindowRole(winInfo);
    assert(strcmp(role.c_str(), winInfo->getRole().c_str()) == 0);
}
SCUTEST_ITER(get_set_type, 4) {
    WindowID win = createTypelessInputOnlyWindow();
    bool normal = _i / 2;
    bool implicit = _i % 2;
    xcb_atom_t atom = normal ? ewmh->_NET_WM_WINDOW_TYPE_NORMAL : ewmh->_NET_WM_WINDOW_TYPE_DIALOG;
    if(!implicit)
        setWindowType(win, atom);
    else if(!normal)
        setWindowTransientFor(win, createNormalWindow());
    flush();
    string typeName = getAtomName(atom);
    WindowInfo* winInfo = new WindowInfo(win);
    winInfo->setTransientFor(implicit && !normal);
    getAllWindows().add(winInfo);
    loadWindowProperties(winInfo);
    assertEquals(winInfo->getType(), atom);
    assertEquals(winInfo->getTypeName(), typeName);
}
SCUTEST(load_properties_normal) {
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
}

SCUTEST_ITER(test_load_masks_from_hints, 4) {
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
}

SCUTEST(load_protocols) {
    WindowID win = createUnmappedWindow();
    WindowInfo* winInfo = new WindowInfo(win);
    getAllWindows().add(winInfo);
    xcb_atom_t atoms[] = {WM_TAKE_FOCUS, WM_DELETE_WINDOW, ewmh->_NET_WM_PING, ewmh->_NET_WM_SYNC_REQUEST};
    xcb_icccm_set_wm_protocols(dis, win, ewmh->WM_PROTOCOLS, LEN(atoms), atoms);
    loadWindowProperties(winInfo);
    assert(winInfo->hasMask(WM_DELETE_WINDOW_MASK));
    assert(winInfo->hasMask(WM_TAKE_FOCUS_MASK));
    assert(winInfo->hasMask(WM_PING_MASK));
}

SCUTEST(test_window_extra_property_loading) {
    WindowID win = createUnmappedWindow();
    int groupLead = createUnmappedWindow();
    int transientForWindow = createUnmappedWindow();
    WindowInfo* winInfo = new WindowInfo(win);
    xcb_icccm_wm_hints_t hints;
    xcb_icccm_wm_hints_set_window_group(&hints, groupLead);
    xcb_icccm_wm_hints_set_urgency(&hints);
    xcb_icccm_set_wm_hints(dis, win, &hints);
    xcb_icccm_set_wm_transient_for(dis, win, transientForWindow);
    flush();
    addWindowInfo(winInfo);
    loadWindowProperties(winInfo);
    assert(winInfo->getGroup() == groupLead);
    assert(winInfo->getTransientFor() == transientForWindow);
    assert(winInfo->hasMask(URGENT_MASK));
}
SCUTEST(ignore_small_window) {
    WindowID win = mapArbitraryWindow();
    xcb_size_hints_t hints;
    xcb_icccm_size_hints_set_size(&hints, 0, 1, 1);
    assert(!catchError(xcb_icccm_set_wm_size_hints_checked(dis, win, XCB_ATOM_WM_NORMAL_HINTS, &hints)));
    WindowInfo* winInfo = new WindowInfo(win);
    addWindowInfo(winInfo);
    loadWindowProperties(winInfo);
    assert(getWindowSizeHints(winInfo));
}
SCUTEST(test_set_border_color) {
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
}
SCUTEST(reset_border_color) {
    createMasterDevice("test");
    initCurrentMasters();
    WindowID win = mapWindow(createNormalWindow());
    scan(root);
    for(Master* m : getAllMasters())
        m->onWindowFocus(win);
    resetBorder(win);
    //TODO check to see if border was set correctly
}
SCUTEST_ITER(test_focus_window, 4) {
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
}
SCUTEST(test_focus_inputless_window) {
    WindowInfo winInfo = WindowInfo(1);
    winInfo.removeMask(INPUT_MASK);
    assert(focusWindow(&winInfo) == 0);
}
SCUTEST(test_wm_take_focus) {
    WindowInfo winInfo = WindowInfo(mapArbitraryWindow());
    winInfo.addMask(INPUT_MASK | WM_TAKE_FOCUS_MASK);
    assert(focusWindow(&winInfo));
}
SCUTEST(test_focus_unmapped_window) {
    CRASH_ON_ERRORS = 0;
    setLogLevel(LOG_LEVEL_NONE);
    WindowInfo winInfo = WindowInfo(createNormalWindow());
    winInfo.addMask(INPUT_MASK | WM_TAKE_FOCUS_MASK);
    assert(focusWindow(&winInfo) == 0);
}

SCUTEST(test_focus_window_bad) {
    CRASH_ON_ERRORS = 0;
    setLogLevel(LOG_LEVEL_NONE);
    /// TODO reverse return value
    assert(focusWindow(createNormalWindow()) == 0);
}

SCUTEST_ITER(user_time, 2) {
    WindowID win = createNormalWindow();
    assertEquals(getUserTime(win), 1);
    TimeStamp time = getTime();
    if(_i)
        setUserTime(win, time);
    else {
        WindowID win2 = createNormalWindow();
        setUserTime(win2, time);
        assert(!catchError(xcb_ewmh_set_wm_user_time_checked(ewmh, win2, time)));
        assert(!catchError(xcb_ewmh_set_wm_user_time_window_checked(ewmh, win, win2)));
    }
    assertEquals(getUserTime(win), time);
}
*/
