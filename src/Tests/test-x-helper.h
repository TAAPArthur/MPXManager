#ifndef MPX_TEST_X11_HELPER2
#define MPX_TEST_X11_HELPER2

#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xinput.h>

#include <assert.h>
#include <err.h>
#include "test-mpx-helper.h"
#include "../globals.h"
#include "../xsession.h"
#include "../logger.h"
#include "../system.h"
#include "../masters.h"
#include "../device-grab.h"
#include "../bindings.h"
#include "../devices.h"
#include "../user-events.h"

static void inline saveXSession() {
    static auto _dpy = dpy;
    static auto _dis = dis;
    static auto _ewmh = ewmh;
    assert(_dpy && _dis && _ewmh);
}
static inline int createWindow(int parent, int mapped, int ignored, int userIgnored, uint32_t input, int clazz) {
    assert(ignored < 2);
    assert(dis);
    xcb_window_t window = xcb_generate_id(dis);
    xcb_void_cookie_t c = xcb_create_window_checked(dis,                   /* Connection          */
            XCB_COPY_FROM_PARENT,          /* depth (same as root)*/
            window,                        /* window Id           */
            parent,                  /* parent window       */
            0, 0,                          /* x, y                */
            150, 150,                      /* width, height       */
            0,                            /* border_width        */
            clazz, /* class               */
            screen->root_visual,           /* visual              */
            XCB_CW_OVERRIDE_REDIRECT, &ignored);                     /* masks, not used yet */
    xcb_generic_error_t* e = xcb_request_check(dis, c);
    if(e) {
        logError(e);
        err(1, "could not create window\n");
    }
    xcb_icccm_wm_hints_t hints = {.input = input, .initial_state = mapped};
    e = xcb_request_check(dis, xcb_icccm_set_wm_hints_checked(dis, window, &hints));
    if(e) {
        err(1, "could not set hintsfor window on creation\n");
    }
    flush();
    if(!userIgnored && !ignored)
        catchError(xcb_ewmh_set_wm_window_type_checked(ewmh, window, 1, &ewmh->_NET_WM_WINDOW_TYPE_NORMAL));
    return window;
}
static inline WindowID  createInputOnlyWindow(void) {
    return createWindow(root, 1, 0, 0, 1, XCB_WINDOW_CLASS_INPUT_ONLY);
}
static inline WindowID  createInputWindow(int input) {
    return createWindow(root, 1, 0, 0, input, XCB_WINDOW_CLASS_INPUT_OUTPUT);
}
static inline WindowID createTypelessInputOnlyWindow(void) {
    return createWindow(root, 0, 0, 1, 1, XCB_WINDOW_CLASS_INPUT_ONLY);
}
static inline WindowID createIgnoredWindow(void) {
    return createWindow(root, 1, 1, 0, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT);
}
static inline WindowID createNormalWindow(void) {
    return createWindow(root, 1, 0, 0, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT);
}
static inline WindowID createNormalWindowWithType(xcb_atom_t type) {
    Window win = createWindow(root, 1, 0, 0, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT);
    assert(!catchError(xcb_ewmh_set_wm_window_type_checked(ewmh, win, 1, &type)));
    return win;
}
static inline WindowID createNormalSubWindow(int parent) {
    return createWindow(parent, 1, 0, 0, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT);
}
static inline WindowID  createUnmappedWindow(void) {
    return createWindow(root, 0, 0, 0, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT);
}
static inline WindowID mapWindow(int win) {
    assert(xcb_request_check(dis, xcb_map_window_checked(dis, win)) == NULL);
    flush();
    return win;
}
static inline WindowID mapArbitraryWindow() {return mapWindow(createNormalWindow());}
static inline bool checkStackingOrder(const WindowID* stackingOrder, int num, bool adj = 0) {
    xcb_query_tree_reply_t* reply;
    reply = xcb_query_tree_reply(dis, xcb_query_tree(dis, root), 0);
    if(!reply) {
        LOG(LOG_LEVEL_WARN, "could not query tree to check stacking order\n");
        return 0;
    }
    int numberOfChildren = xcb_query_tree_children_length(reply);
    xcb_window_t* children = xcb_query_tree_children(reply);
    int counter = 0;
    LOG_RUN(LOG_LEVEL_TRACE, PRINT_ARR("Window Stack", children, numberOfChildren, "\n"));
    LOG_RUN(LOG_LEVEL_TRACE, PRINT_ARR("Expected Stack", stackingOrder, num, "\n"));
    for(int i = 0; i < numberOfChildren; i++) {
        if(children[i] == stackingOrder[counter]) {
            counter++;
            if(counter == num)
                break;
        }
        if(adj && counter) {
            LOG(LOG_LEVEL_WARN, "results not adjacent\n");
            break;
        }
    }
    free(reply);
    LOG(LOG_LEVEL_TRACE, "%d vs %d\n", counter, num);
    return counter == num;
}

static inline int consumeEvents() {
    lock();
    flush();
    xcb_generic_event_t* e;
    int numEvents = 0;
    while(1) {
        e = xcb_poll_for_event(dis);
        if(e) {
            numEvents++;
            LOG(LOG_LEVEL_ALL, "Event ignored %d %s\n",
                e->response_type, eventTypeToString(e->response_type & 127));
            free(e);
        }
        else break;
    }
    unlock();
    return numEvents;
}
static inline void cleanupXServer() {
    destroyAllNonDefaultMasters();
    for(int i = 0; i < MPX_LAST_EVENT; i++) {
        getEventRules(i).deleteElements();
        getBatchEventRules(i).deleteElements();
    }
    destroyAllLists();
}
static inline void createXSimpleEnv(void) {
    openXDisplay();
    createSimpleEnv();
    addRootMonitor();
    assignUnusedMonitorsToWorkspaces();
}
#endif
