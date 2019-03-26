
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>

#include "../../devices.h"
#include "../../default-rules.h"
#include "../../mywm-util.h"
#include "../../logger.h"
#include "../../events.h"
#include "../../ewmh.h"
#include "../../globals.h"

#include "../UnitTests.h"
#include "../TestX11Helper.h"


START_TEST(test_mask_add_remove_toggle){
    SYNC_WINDOW_MASKS = 1;
    WindowInfo* winInfo = createWindowInfo(createNormalWindow());
    addWindowInfo(winInfo);
    assert(winInfo->mask == 0);
    addMask(winInfo, 1);
    assert(winInfo->mask == 1);
    addMask(winInfo, 6);
    assert(winInfo->mask == 7);
    removeMask(winInfo, 12);
    assert(winInfo->mask == 3);
    assert(!hasMask(winInfo, 6));
    toggleMask(winInfo, 6);
    assert(hasMask(winInfo, 6));
    assert(winInfo->mask == 7);
    toggleMask(winInfo, 6);
    assert(!hasMask(winInfo, 6));
    assert(winInfo->mask == 1);
    toggleMask(winInfo, 6);
}
END_TEST

START_TEST(test_mask_reset){
    WindowInfo* winInfo = createWindowInfo(createNormalWindow());
    addWindowInfo(winInfo);
    addMask(winInfo, X_MAXIMIZED_MASK | Y_MAXIMIZED_MASK | ROOT_FULLSCREEN_MASK | FULLSCREEN_MASK);
    int nonUserMask = MAPPED_MASK | PARTIALLY_VISIBLE;
    addMask(winInfo, nonUserMask);
    resetUserMask(winInfo);
    assert(getUserMask(winInfo) == 0);
    assert(winInfo->mask == nonUserMask);
}
END_TEST

START_TEST(test_mask_save_restore){
    SYNC_WINDOW_MASKS = 1;
    WindowInfo* winInfo = createWindowInfo(createNormalWindow());
    WindowInfo* winInfo2 = createWindowInfo(createNormalWindow());
    assert(processNewWindow(winInfo));
    //add a USER_MASK
    addMask(winInfo, NO_TILE_MASK);
    xcb_ewmh_get_atoms_reply_t reply;
    int hasState = xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, winInfo->id), &reply, NULL);
    assert(hasState);
    catchError(xcb_ewmh_set_wm_state_checked(ewmh, winInfo2->id, reply.atoms_len, reply.atoms));
    assert(processNewWindow(winInfo2));
    assert(getSize(getAllWindows()) == 2);
    assert(getMask(winInfo) == getMask(winInfo2));
    xcb_ewmh_get_atoms_reply_wipe(&reply);
}
END_TEST

START_TEST(test_get_set_geometry_config){
    WindowInfo* winInfo = createWindowInfo(1);
    addWindowInfo(winInfo);
    short arr[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    assert(LEN(arr) == LEN(winInfo->geometry) + LEN(winInfo->config) && "array size needs to be increased");
    int offset = LEN(winInfo->geometry);
    setGeometry(winInfo, arr);
    short* geo = getGeometry(winInfo);
    setConfig(winInfo, &arr[offset]);
    short* config = getConfig(winInfo);
    for(int i = 0; i < LEN(arr); i++)
        assert(arr[i] == (i < offset ? geo[i] : config[i - offset]));
}
END_TEST


START_TEST(test_lock_geometry_config){
    WindowInfo* winInfo = createWindowInfo(1);
    addWindowInfo(winInfo);
    int offset = LEN(winInfo->geometry);
    short* geo = NULL;
    short* config = NULL;
    for(int n = 0; n < 6; n++){
        if(n == 0)assert(winInfo->geometrySemaphore == 0);
        if(n < 3)
            lockWindow(winInfo);
        else unlockWindow(winInfo);
        short arr[10] = {n + 1, 2, 3, 4, 5, n + 1, 7, 8, 9, 10};
        if(n){
            assert(arr[0] != geo[0]);
            assert(arr[offset] != config[0]);
        }
        setGeometry(winInfo, arr);
        setConfig(winInfo, &arr[offset]);
        geo = getGeometry(winInfo);
        config = getConfig(winInfo);
        for(int i = 0; i < LEN(arr); i++)
            if(i < offset)
                assert((arr[i] == geo[i]) == (winInfo->geometrySemaphore == 0 ? 1 : 0));
            else
                assert(arr[i] == config[i - offset]);
    }
    assert(winInfo->geometrySemaphore == 0);
}
END_TEST


START_TEST(test_window_property_alt_loading){
    WindowID win = createUnmappedWindow();
    WindowInfo* winInfo = createWindowInfo(win);
    char* name = "test";
    xcb_icccm_set_wm_name(dis, win, XCB_ATOM_STRING, 8, strlen(name), name);
    addWindowInfo(winInfo);
    loadWindowProperties(winInfo);
    assert(strcmp(winInfo->title, name) == 0);
}
END_TEST
START_TEST(test_window_property_loading){
    WindowID win = createIgnoredWindow();
    WindowInfo* winInfo = createWindowInfo(win);
    addWindowInfo(winInfo);
    //no properties are set don't crash
    loadWindowProperties(winInfo);
    assert(winInfo->className == NULL);
    assert(winInfo->instanceName == NULL);
    assert(winInfo->title == NULL);
    assert(winInfo->typeName == NULL || hasMask(winInfo, IMPLICIT_TYPE));
    assert(winInfo->type == 0 || hasMask(winInfo, IMPLICIT_TYPE));
    setProperties(win);
    //reloaeding should break anything
    for(int i = 0; i < 2; i++){
        loadWindowProperties(winInfo);
        checkProperties(winInfo);
    }
}
END_TEST
START_TEST(test_window_property_reloading){
    WindowID win = createUnmappedWindow();
    WindowID win2 = createUnmappedWindow();
    WindowInfo* winInfo = createWindowInfo(win);
    addWindowInfo(winInfo);
    setProperties(win);
    loadWindowProperties(winInfo);
    //very hacky way to unset the title property of the window
    winInfo->id = win2;
    //don't double free window properties
    loadWindowProperties(winInfo);
    loadWindowProperties(winInfo);
}
END_TEST
START_TEST(test_window_extra_property_loading){
    WindowID win = createUnmappedWindow();
    int groupLead = createUnmappedWindow();
    int trasientForWindow = createUnmappedWindow();
    WindowInfo* winInfo = createWindowInfo(win);
    xcb_icccm_wm_hints_t hints;
    xcb_icccm_wm_hints_set_window_group(&hints, groupLead);
    xcb_icccm_set_wm_hints(dis, win, &hints);
    xcb_icccm_set_wm_transient_for(dis, win, trasientForWindow);
    xcb_atom_t atoms[] = {WM_TAKE_FOCUS, WM_DELETE_WINDOW, ewmh->_NET_WM_PING};
    xcb_icccm_set_wm_protocols(dis, win, ewmh->WM_PROTOCOLS, 3, atoms);
    flush();
    addWindowInfo(winInfo);
    loadWindowProperties(winInfo);
    assert(winInfo->groupId == groupLead);
    assert(winInfo->transientFor == trasientForWindow);
}
END_TEST

START_TEST(test_window_input_focus_detection){
    for(int i = 0; i < 4; i++){
        DEFAULT_WINDOW_MASKS = i % 2 ? 0 : INPUT_MASK;
        WindowID win = createInputWindow(i < 2);
        WindowInfo* winInfo = createWindowInfo(win);
        flush();
        processNewWindow(winInfo);
        assert(hasMask(winInfo, INPUT_MASK) == (i < 2 ? 1 : 0));
        removeWindow(win);
    }
}
END_TEST

START_TEST(test_process_window){
    DEFAULT_WINDOW_MASKS = 0;
    xcb_atom_t net_atoms[] = {SUPPORTED_STATES};
    WindowID win = createUnmappedWindow();
    setProperties(win);
    xcb_ewmh_set_wm_state(ewmh, win, LEN(net_atoms), net_atoms);
    flush();
    assert(!isNotEmpty(getAllWindows()));
    WindowInfo* winInfo = createWindowInfo(win);
    processNewWindow(winInfo);
    assert(isNotEmpty(getAllWindows()));
    assert(getSize(getAllWindows()) == 1);
    checkProperties(getHead(getAllWindows()));
    xcb_get_window_attributes_reply_t* attr;
    attr = xcb_get_window_attributes_reply(dis, xcb_get_window_attributes(dis, win), NULL);
    assert(attr->your_event_mask == NON_ROOT_EVENT_MASKS);
    free(attr);
    assert(winInfo->mask);
}
END_TEST
START_TEST(test_process_input_only_window){
    WindowID win = createInputOnlyWindow();
    scan(root);
    assert(hasMask(getWindowInfo(win), INPUT_ONLY_MASK));
}
END_TEST
START_TEST(test_process_bad_window){
    WindowID win = createNormalWindow();
    WindowInfo* winInfo = createWindowInfo(win);
    winInfo->creationTime = getTime();
    assert(!catchError(xcb_destroy_window_checked(dis, win)));
    processNewWindow(winInfo);
    assert(getSize(getAllWindows()) == 0);
}
END_TEST
START_TEST(test_register_bad_window){
    processNewWindow(createWindowInfo(1));
    assert(getSize(getAllWindows()) == 0);
    processNewWindow(createWindowInfo(2));
    assert(getSize(getAllWindows()) == 0);
}
END_TEST

START_TEST(test_window_scan){
    assert(!isNotEmpty(getAllWindows()));
    addDummyIgnoreRule();
    scan(root);
    assert(!isNotEmpty(getAllWindows()));
    WindowID win = createUnmappedWindow();
    WindowID win2 = createUnmappedWindow();
    WindowID win3 = createUnmappedWindow();
    createIgnoredWindow();
    createUserIgnoredWindow();
    assert(!isWindowMapped(win));
    assert(!isWindowMapped(win2));
    assert(!isWindowMapped(win3));
    mapWindow(win);
    xcb_flush(dis);
    scan(root);
    assert(find(getAllWindows(), &win, sizeof(int)));
    assert(find(getAllWindows(), &win2, sizeof(int)));
    assert(find(getAllWindows(), &win3, sizeof(int)));
    assert(getSize(getAllWindows()) == 3);
}
END_TEST
START_TEST(test_child_window_scan){
    IGNORE_SUBWINDOWS = 1;
    assert(!isNotEmpty(getAllWindows()));
    int parent = createNormalWindow();
    WindowID win = createNormalSubWindow(parent);
    int child = createNormalSubWindow(win);
    int grandChild = createNormalSubWindow(child);
    scan(parent);
    assert(find(getAllWindows(), &win, sizeof(int)));
    assert(!find(getAllWindows(), &child, sizeof(int)));
    assert(getSize(getAllWindows()) == 1);
    IGNORE_SUBWINDOWS = 0;
    scan(win);
    assert(find(getAllWindows(), &child, sizeof(int)));
    assert(find(getAllWindows(), &grandChild, sizeof(int)));
    assert(getSize(getAllWindows()) == 3);
}
END_TEST


START_TEST(test_window_state){
    xcb_atom_t net_atoms[] = {WM_TAKE_FOCUS, WM_DELETE_WINDOW, SUPPORTED_STATES};
    WindowID win = createNormalWindow();
    WindowInfo* winInfo = createWindowInfo(win);
    addWindowInfo(winInfo);
    addWindowToWorkspace(winInfo, 0);
    //Check to make sure some mask is set
    //should not error
    setWindowStateFromAtomInfo(NULL, net_atoms, LEN(net_atoms), XCB_EWMH_WM_STATE_REMOVE);
    setWindowStateFromAtomInfo(winInfo, net_atoms, LEN(net_atoms), XCB_EWMH_WM_STATE_REMOVE);
    assert(winInfo->mask == 0);
    setWindowStateFromAtomInfo(winInfo, net_atoms, LEN(net_atoms), XCB_EWMH_WM_STATE_ADD);
    assert(winInfo->mask);
    setWindowStateFromAtomInfo(winInfo, net_atoms, LEN(net_atoms), XCB_EWMH_WM_STATE_TOGGLE);
    assert(winInfo->mask == 0);
    setWindowStateFromAtomInfo(winInfo, net_atoms, LEN(net_atoms), XCB_EWMH_WM_STATE_TOGGLE);
    assert(winInfo->mask);
    catchError(xcb_ewmh_set_wm_state_checked(ewmh, winInfo->id, 1, (unsigned int[]){
        -1
        }));
    setXWindowStateFromMask(winInfo);
    xcb_ewmh_get_atoms_reply_t reply;
    int hasState = xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, winInfo->id), &reply, NULL);
    assert(hasState);
    for(int i = 0; i < reply.atoms_len; i++)
        if(reply.atoms[i] == -1){
            xcb_ewmh_get_atoms_reply_wipe(&reply);
            return;
        }
    assert(0);
}
END_TEST

START_TEST(test_window_state_sync){
    SYNC_WINDOW_MASKS = 1;
    WindowID win = createNormalWindow();
    scan(root);
    WindowInfo* winInfo = getWindowInfo(win);
    addMask(winInfo, -1);
    xcb_atom_t net_atoms[] = {SUPPORTED_STATES};
    xcb_ewmh_get_atoms_reply_t reply;
    int count = 0;
    int status = xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, win), &reply, NULL);
    assert(status);
    for(int j = 0; j < LEN(net_atoms); j++)
        for(int i = 0; i < reply.atoms_len; i++)
            if(reply.atoms[i] == net_atoms[j]){
                count++;
                break;
            }
    assert(count == LEN(net_atoms));
    xcb_ewmh_get_atoms_reply_wipe(&reply);
}
END_TEST


Suite* windowsSuite(void){
    Suite* s = suite_create("Window Manager Functions");
    TCase* tc_core;
    tc_core = tcase_create("Window fields");
    tcase_add_checked_fixture(tc_core, createContextAndSimpleConnection, fullCleanup);
    tcase_add_test(tc_core, test_mask_add_remove_toggle);
    tcase_add_test(tc_core, test_mask_reset);
    tcase_add_test(tc_core, test_mask_save_restore);
    tcase_add_test(tc_core, test_get_set_geometry_config);
    tcase_add_test(tc_core, test_lock_geometry_config);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Window_Detection");
    tcase_add_checked_fixture(tc_core, createContextAndSimpleConnection, destroyContextAndConnection);
    tcase_add_test(tc_core, test_window_property_loading);
    tcase_add_test(tc_core, test_window_property_reloading);
    tcase_add_test(tc_core, test_window_property_alt_loading);
    tcase_add_test(tc_core, test_window_input_focus_detection);
    tcase_add_test(tc_core, test_window_extra_property_loading);
    tcase_add_test(tc_core, test_process_window);
    tcase_add_test(tc_core, test_process_input_only_window);
    tcase_add_test(tc_core, test_process_bad_window);
    tcase_add_test(tc_core, test_register_bad_window);
    tcase_add_test(tc_core, test_window_scan);
    tcase_add_test(tc_core, test_child_window_scan);
    //tcase_add_test(tc_core, test_sync_state);
    tcase_add_test(tc_core, test_window_state);
    tcase_add_test(tc_core, test_window_state_sync);
    suite_add_tcase(s, tc_core);
    return s;
}
