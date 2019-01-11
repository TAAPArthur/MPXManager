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
#include "../../layouts.h"
#include "../../window-clone.h"
#include "../../test-functions.h"
#include "../../state.h"

static WindowInfo*winInfo;
static WindowInfo*clone;
static void setup(){
    startUpMethod=addCloneRules;
    onStartup();
    setActiveLayout(&DEFAULT_LAYOUTS[GRID]);
    int window=createNormalWindow();
    mapWindow(window);

    scan(root);
    START_MY_WM
    winInfo=getValue(getAllWindows());
    assert(winInfo);
    clone=cloneWindow(winInfo);
    lock();
    int idle=getIdleCount();
    unlock();
    WAIT_UNTIL_TRUE(idle!=getIdleCount());

}
START_TEST(test_clone_window){
   assert(getSize(getAllWindows())==2);
   assert(checkStackingOrder((int*)&clone->id,1));
}END_TEST
START_TEST(test_swap_with_original){
    Node*head=getActiveWindowStack(); 
    int cloneHead=getValue(head)==clone;
    movePointer(getActiveMasterPointerID(),winInfo->id,0,0);
    movePointer(getActiveMasterPointerID(),clone->id,0,0);
    lock();
    int idle=getIdleCount();
    unlock();
    WAIT_UNTIL_TRUE(idle!=getIdleCount());
    assert(cloneHead!=(getValue(head)==clone));
}END_TEST
START_TEST(test_update_clone){
    updateClone(winInfo, clone);
    //TODO actually test somthing
}END_TEST
START_TEST(test_auto_update_clones){
    pthread_t thread=runInNewThread(autoUpdateClones,NULL,0);
    msleep(100);
    requestShutdown();
    pthread_join(thread,((void *)0));
}END_TEST

Suite *windowCloneSuite() {
    Suite*s = suite_create("My Window Manager");
    TCase* tc_core = tcase_create("Clone");
    tcase_add_checked_fixture(tc_core, setup, fullCleanup);
    tcase_add_test(tc_core, test_clone_window);
    tcase_add_test(tc_core, test_update_clone);
    tcase_add_test(tc_core, test_swap_with_original);
    tcase_add_test(tc_core, test_auto_update_clones);
    suite_add_tcase(s, tc_core);
    return s;
}

