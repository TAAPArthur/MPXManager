

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

#include "../devices.h"
#include "../default-rules.h"
#include "../logger.h"
#include "../events.h"
#include "../wmfunctions.h"
#include "../functions.h"
#include "../ewmh.h"
#include "../globals.h"
#include "../layouts.h"
#include "../xsession.h"


#include "tester.h"
#include "test-event-helper.h"

MPX_TEST("test_triple_start", {
    setLogLevel(LOG_LEVEL_NONE);
    xcb_get_selection_owner_reply_t* ownerReply;
    ownerReply = xcb_get_selection_owner_reply(dis, xcb_get_selection_owner(dis,
            WM_SELECTION_ATOM), NULL);
    assert(ownerReply->owner);
    free(ownerReply);
    if(!fork())
        onStartup();
    wait(NULL);
    ownerReply = xcb_get_selection_owner_reply(dis, xcb_get_selection_owner(dis,
            WM_SELECTION_ATOM), NULL);
    assert(ownerReply->owner);
    free(ownerReply);
    onStartup();
    exit(10);
}
        );
MPX_TEST("test_top_focused_window_synced", {
    setActiveLayout(&DEFAULT_LAYOUTS[FULL]);
    int idle;
    lock();
    activateWorkspace(0);
    WindowInfo* winInfo = getHead(getActiveWindowStack());
    WindowInfo* last = getLast(getActiveWindowStack());
    winInfo->addMask(STICKY_MASK);
    activateWindow(winInfo);
    raiseWindowInfo(last);
    activateWorkspace(getNumberOfWorkspaces() - 1);
    assert(getActiveWindowStack().size() == 1);
    idle = getIdleCount();
    unlock();
    WAIT_UNTIL_TRUE(idle != getIdleCount());
    assert(getFocusedWindow() == winInfo);
    lock();
    activateWindow(last);
    assert(getActiveWindowStack().size() == 2);
    retile();
    flush();
    WindowID stackingOrder[] = {last->getID(), winInfo->getID(), last->getID()};
    assert(checkStackingOrder(stackingOrder + (last->getID() == getActiveFocus(getActiveMasterKeyboardID())), 2));
    unlock();
});
MPX_TEST("test_monitor_deletion", {
    POLL_COUNT = 0;
    MONITOR_DUPLICATION_POLICY = 0;
    Monitor* m = getHead(getAllMonitors());
    for(int i = 0; i < getNumberOfWorkspaces(); i++)
        createMonitor(m->base);
    detectMonitors();
    assert(getNumberOfWorkspaces() + 1 == getAllMonitors().size());
    assert(m);
    assert(getWorkspaceFromMonitor(m));
    for(int i = 0; i < getNumberOfWorkspaces(); i++)
        assert(isVisible(i));
    removeMonitor(m->getID());
    markState();
    startWM();
    waitUntilIdle();
    for(int i = 0; i < getNumberOfWorkspaces(); i++)
        assert(isVisible(i));
    consumeEvents();
}
        );
MPX_TEST("test_workspace_deletion", {
    assert(getNumberOfWorkspaces() > 3);
    activateWorkspace(1);
    for(int i = 0; i < 10; i++)
        registerWindow(createWindowInfo(createNormalWindow()));
    flush();
    activateWorkspace(0);
    MONITOR_DUPLICATION_POLICY = 0;
    createMonitor((Rect) {0, 0, 100, 100});
    detectMonitors();
    startWM();
    WAIT_UNTIL_TRUE(getWorkspaceByIndex(1)->mapped);
    lock();
    activateWorkspace(1);
    activateWorkspace(2);
    assert(isVisible(2));
    assert(!isVisible(1));
    assert(isVisible(0));
    xcb_ewmh_request_change_number_of_desktops(ewmh, defaultScreenNumber, 1);
    flush();
    unlock();
    WAIT_UNTIL_TRUE(getNumberOfWorkspaces() == 1);
    assert(isVisible(0));
    assert(getActiveWindowStack().size() == getAllWindows().size());
}
        );
MPX_TEST("test_workspace_monitor_addition", {
    MONITOR_DUPLICATION_POLICY = 0;
    removeWorkspaces(getNumberOfWorkspaces() - 1);
    createMonitor((Rect) {0, 0, 100, 100});
    detectMonitors();
    assert(getNumberOfWorkspaces() < getAllMonitors().size());
    for(int i = 0; i < getNumberOfWorkspaces(); i++)
        assert(isVisible(i));
    consumeEvents();
    xcb_ewmh_request_change_number_of_desktops(ewmh, defaultScreenNumber, getAllMonitors().size());
    flush();
    startWM();
    WAIT_UNTIL_TRUE(getNumberOfWorkspaces() == getAllMonitors().size());
    for(int i = 0; i < getNumberOfWorkspaces(); i++)
        assert(isVisible(i));
}
        );
/*TODO functional test
MPX_TEST("hung_thread", {
    runInNewThread([](void*)->void* {
        while(1)msleep(100);
        return NULL;
    }, NULL, "Spin Forever");
    requestShutdown();
    waitForAllThreadsToExit();
});
*/
