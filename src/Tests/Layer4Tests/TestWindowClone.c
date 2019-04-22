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
#include "../../Extensions/window-clone.h"
#include "../../test-functions.h"
#include "../../state.h"

static WindowInfo* winInfo;
static WindowInfo* clone;
static void setup(){
    POLL_COUNT = 10;
    CRASH_ON_ERRORS = -1;
    startUpMethod = addCloneRules;
    onStartup();
    switchToWorkspace(0);
    setActiveLayout(&DEFAULT_LAYOUTS[MASTER]);
    WindowID win = mapWindow(createNormalWindow());
    setProperties(win);
    scan(root);
    winInfo = getHead(getAllWindows());
    checkProperties(winInfo);
    assert(winInfo);
    clone = cloneWindow(winInfo);
    assert(hasMask(clone,MAPPED_MASK));
    retile();
    int idle = getIdleCount();
    flush();
    START_MY_WM;
    WAIT_UNTIL_TRUE(idle != getIdleCount());
}
START_TEST(test_clone_window){
    assert(getSize(getAllWindows()) == 2);
    assert(checkStackingOrder(&clone->id, 1));
    checkProperties(winInfo);
    updateAllClonesOfWindow(winInfo->id);
    WAIT_UNTIL_TRUE(clone->title);
    checkProperties(clone);
}
END_TEST
START_TEST(test_sync_properties){
    setLogLevel(LOG_LEVEL_DEBUG);
    int idle;
    lock();
    winInfo->title[0] = 'A';
    assert(strcmp(clone->title, winInfo->title));
    idle = getIdleCount();
    updateAllClonesOfWindow(winInfo->id);
    flush();
    unlock();
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    assert(strcmp(clone->title, winInfo->title) == 0);
}
END_TEST
START_TEST(test_seemless_swap_with_original){
    updateState(NULL, NULL);
    markState();
    swapWindows(winInfo, clone);
    assert(updateState(NULL, NULL) == 0);
}
END_TEST
START_TEST(test_swap_with_original){
    setActiveLayout(NULL);
    int index = indexOf(getActiveWindowStack(), clone, sizeof(WindowID));
    xcb_get_geometry_reply_t* replyBefore = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, winInfo->id), NULL);
    xcb_get_geometry_reply_t* replyCloneBefore = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, clone->id), NULL);
    assert(memcmp(&replyBefore->x, &replyCloneBefore->x, sizeof(short) * 4) != 0);
    movePointer(getActiveMasterPointerID(), winInfo->id, 0, 0);
    movePointer(getActiveMasterPointerID(), clone->id, 0, 0);
    flush();
    WAIT_UNTIL_TRUE(index != indexOf(getActiveWindowStack(), clone, sizeof(WindowID)));
    xcb_get_geometry_reply_t* replyAfter = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, winInfo->id), NULL);
    xcb_get_geometry_reply_t* replyCloneAfter = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, clone->id), NULL);
    assert(memcmp(&replyBefore->x, &replyCloneAfter->x, sizeof(short) * 4) == 0);
    assert(memcmp(&replyCloneBefore->x, &replyAfter->x, sizeof(short) * 4) == 0);
    free(replyBefore);
    free(replyCloneBefore);
    free(replyAfter);
    free(replyCloneAfter);
}
END_TEST
START_TEST(test_swap_with_original_workspaces){
    int index = getActiveWorkspaceIndex();
    for(int i = 0; i < 2; i++){
        lock();
        if(i == 0){
            moveWindowToWorkspace(clone, !index);
            activateWorkspace(!index);
            assert(indexOf(getActiveWindowStack(), clone, sizeof(WindowID)) != -1);
        }
        else {
            moveWindowToWorkspace(winInfo, index);
            moveWindowToWorkspace(clone, !index);
        }
        unlock();
        WAIT_UNTIL_TRUE(indexOf(getActiveWindowStack(), clone, sizeof(WindowID)) == -1);
        assert(indexOf(getActiveWindowStack(), winInfo, sizeof(WindowID)) != -1);
        assert(focusWindowInfo(winInfo));
    }
}
END_TEST

START_TEST(test_update_clone){
    updateAllClonesOfWindow(winInfo->id);
    //TODO actually test somthing
}
END_TEST
START_TEST(test_simple_kill_clone){
    if(_i)
        updateAllClones();
    ATOMIC(killAllClones(winInfo));
    flush();
    WAIT_UNTIL_TRUE(getSize(getAllWindows()) == 1);
    updateAllClones();
}
END_TEST

// Tests was inspired from use
START_TEST(test_kill_clone_different_workspace){
    lock();
    moveWindowToWorkspace(clone, 1);
    switchToWorkspace(1);
    unlock();
    WAIT_UNTIL_TRUE(getHead(getActiveWindowStack()) == winInfo);
    ATOMIC(killAllClones(winInfo));
    flush();
    WAIT_UNTIL_TRUE(getSize(getAllWindows()) == 1);
    lock();
    moveWindowToWorkspace(winInfo, 0);
    switchToWorkspace(0);
    int idle = getIdleCount();
    unlock();
    WAIT_UNTIL_TRUE(idle != getIdleCount());
}
END_TEST

START_TEST(test_auto_update_clones){
    pthread_t thread = runInNewThread(autoUpdateClones, NULL, 0);
    msleep(100);
    requestShutdown();
    pthread_join(thread, ((void*)0));
    startAutoUpdatingClones();
}
END_TEST

Suite* windowCloneSuite(){
    Suite* s = suite_create("WindowCloning");
    TCase* tc_core;
    tc_core = tcase_create("WindowCloning");
    tcase_add_checked_fixture(tc_core, setup, fullCleanup);
    tcase_add_test(tc_core, test_clone_window);
    tcase_add_test(tc_core, test_sync_properties);
    tcase_add_loop_test(tc_core, test_simple_kill_clone, 0, 2);
    tcase_add_test(tc_core, test_kill_clone_different_workspace);
    tcase_add_test(tc_core, test_update_clone);
    tcase_add_test(tc_core, test_seemless_swap_with_original);
    tcase_add_test(tc_core, test_swap_with_original);
    tcase_add_test(tc_core, test_swap_with_original_workspaces);
    tcase_add_test(tc_core, test_auto_update_clones);
    suite_add_tcase(s, tc_core);
    return s;
}

