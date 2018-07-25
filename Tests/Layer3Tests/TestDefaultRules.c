#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>

#include "../UnitTests.h"
#include "../TestX11Helper.h"
#include "../../logger.h"
#include "../../defaults.h"
#include "../../wmfunctions.h"


WindowInfo*winInfo;
void setup(){
    INIT
    createContext(1);
    createWindow(dc,0);
    connectToXserver();
    START_MY_WM
    winInfo=getValue(getAllWindows());
}

START_TEST(test_focus_in){
    xcb_set_input_focus(dis, 0, winInfo->id, 0);
    assert(getFocusedWindow()==winInfo);
}END_TEST
Suite *defaultRulesSuite() {
    Suite*s = suite_create("My Window Manager");
    TCase* tc_core = tcase_create("Events");
    tcase_add_checked_fixture(tc_core, setup, NULL);
    tcase_add_test(tc_core, test_focus_in);
    suite_add_tcase(s, tc_core);
    return s;
}
