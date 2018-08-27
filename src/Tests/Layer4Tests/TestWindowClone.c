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
#include "../../globals.h"
#include "../../wmfunctions.h"
#include "../../window-clone.h"

static WindowInfo*winInfo;
static WindowInfo*clone;
static void setup(){
    addCloneRules();
    createSimpleContext();
    connectToXserver();
    int window=createNormalWindow();
    mapWindow(window);

    scan();
    create_graphics_context();
    winInfo=getValue(getAllWindows());
    assert(winInfo);
    clone=createCloneWindow(winInfo);
}
START_TEST(test_clone){
    assert(getSize(getAllWindows())==2);
    assert(getSize(winInfo->cloneList)==1);
    assert(getValue(winInfo->cloneList)==clone);
    assert(getCloneOrigin(clone)==winInfo->id);
    assert(clone->mask==winInfo->mask);
    assert(clone->id!=winInfo->id);
}END_TEST
START_TEST(test_update_clone){
    updateClone(winInfo, clone);
    //TODO actually test somthing
}END_TEST
START_TEST(test_update_all_clones){
    updateAllClones(winInfo);
}END_TEST

Suite *windowCloneSuite() {
    Suite*s = suite_create("My Window Manager");
    TCase* tc_core = tcase_create("Clone");
    tcase_add_checked_fixture(tc_core, setup, NULL);
    tcase_add_test(tc_core, test_clone);
    tcase_add_test(tc_core, test_update_clone);
    tcase_add_test(tc_core, test_update_all_clones);
    suite_add_tcase(s, tc_core);
    return s;
}

