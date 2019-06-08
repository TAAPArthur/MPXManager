#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

#include "../UnitTests.h"
#include "../TestX11Helper.h"

#include "../../globals.h"
#include "../../xsession.h"

START_TEST(test_quit){
    quit(0);
    assert(0);
}
END_TEST
START_TEST(test_get_set_atom){
    openXDisplay();
    xcb_atom_t test = getAtom("TEST");
    assert(test == getAtom("TEST"));
    char* str;
    getAtomValue(test, &str);
    assert(strcmp(str, "TEST") == 0);
    free(str);
}
END_TEST
START_TEST(test_get_set_atom_bad){
    openXDisplay();
    assert(getAtom(NULL) == XCB_ATOM_NONE);
    char* str;
    getAtomValue(-1, &str);
    assert(!str);
}
END_TEST
START_TEST(test_open_xdisplay){
    openXDisplay();
    assert(dis);
    assert(!xcb_connection_has_error(dis));
    assert(screen);
    assert(root);
    assert(WM_DELETE_WINDOW);
    assert(WM_TAKE_FOCUS);
    assert(!xcb_connection_has_error(dis));
}
END_TEST
START_TEST(test_close_xdisplay){
    openXDisplay();
    flush();
    closeConnection();
    openXDisplay();
    closeConnection();
    //will leak if vars aren't freed
    dis = NULL;
    ewmh = NULL;
    //tear down will close
    openXDisplay();
}
END_TEST
START_TEST(test_private_window){
    WindowID win = getPrivateWindow();
    assert(win == getPrivateWindow());
    if(!fork()){
        openXDisplay();
        assert(win != getPrivateWindow());
        quit(0);
    }
    waitForCleanExit(0);
    assert(win == getPrivateWindow());
}
END_TEST

Suite* xsessionSuite(){
    Suite* s = suite_create("XSession");
    TCase* tc_core;
    tc_core   = tcase_create("X");
    tcase_add_checked_fixture(tc_core, NULL, closeConnection);
    tcase_add_test(tc_core, test_open_xdisplay);
    tcase_add_test(tc_core, test_get_set_atom);
    tcase_add_test(tc_core, test_get_set_atom_bad);
    tcase_add_test(tc_core, test_close_xdisplay);
    tcase_add_test(tc_core, test_quit);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("PrivateWindow");
    tcase_add_checked_fixture(tc_core, createContextAndSimpleConnection, closeConnection);
    tcase_add_test(tc_core, test_private_window);
    suite_add_tcase(s, tc_core);
    return s;
}
