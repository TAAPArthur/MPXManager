#include "../UnitTests.h"

#include "../../defaults.h"
#include "../../xsession.h"
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

void openXDisplay();
START_TEST(test_quit){
    quit();
    assert(0);
}END_TEST
START_TEST(test_open_xdisplay){
    openXDisplay();
    assert(!xcb_connection_has_error(dis));
}END_TEST
START_TEST(test_close_xdisplay){
    openXDisplay();
    closeConnection();
    openXDisplay();
}END_TEST
START_TEST(test_xconnection){
    createContext(1);
    connectToXserver();
    assert(getActiveMaster());
    assert(getMonitorFromWorkspace(getWorkspaceByIndex(0)));
}END_TEST

Suite *xsessionSuite() {
    Suite*s = suite_create("XSession");

    TCase* tc_core = tcase_create("X");
    tcase_add_checked_fixture(tc_core, NULL, closeConnection);
    tcase_add_test(tc_core, test_open_xdisplay);
    tcase_add_test(tc_core, test_close_xdisplay);
    tcase_add_test(tc_core, test_xconnection);

    tcase_add_test(tc_core, test_quit);

    suite_add_tcase(s, tc_core);
    return s;
}
