#include "../UnitTests.h"

#include "../../xsession.h"
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include "../../globals.h"

void openXDisplay();
START_TEST(test_quit){
    quit();
    assert(0);
}END_TEST
START_TEST(test_open_xdisplay){
    openXDisplay();
    assert(dis);
    assert(!xcb_connection_has_error(dis));
    assert(screen);
    assert(root);
    assert(!xcb_connection_has_error(dis));
}END_TEST
START_TEST(test_close_xdisplay){
    openXDisplay();
    closeConnection();
    openXDisplay();
    closeConnection();
    //will leak if vars aren't freed
    dis=NULL;
    ewmh=NULL;
    //tear down will close
    openXDisplay();
}END_TEST

Suite *xsessionSuite() {
    Suite*s = suite_create("XSession");

    TCase* tc_core = tcase_create("X");
    tcase_add_checked_fixture(tc_core, NULL, closeConnection);
    tcase_add_test(tc_core, test_open_xdisplay);
    tcase_add_test(tc_core, test_close_xdisplay);

    tcase_add_test(tc_core, test_quit);

    suite_add_tcase(s, tc_core);
    return s;
}
