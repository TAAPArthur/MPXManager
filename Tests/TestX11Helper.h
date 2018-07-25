
#ifndef TESTS_XUNITTESTS_H_
#define TESTS_XUNITTESTS_H_

#include "UnitTests.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>

#include "../default-rules.h"
#include "../defaults.h"
#include "../logger.h"
#include "../event-loop.h"

Suite *x11Suite();

#define INIT DisplayConnection* dc = createDisplayConnection();
#define END XCloseDisplay(dc->dpy);
#define START_MY_WM \
        pthread_t pThread; assert(pthread_create(&pThread, NULL, runEventLoop, NULL)==0);
#define KILL_MY_WM  pthread_kill(pThread,0);
#define destroyWindow(dis,win) assert(NULL==xcb_request_check(dis, xcb_destroy_window_checked(dis, win)));

typedef struct {
    Display *dpy;
    xcb_connection_t *dis;
    xcb_ewmh_connection_t *ewmh;
    xcb_window_t root;
    xcb_screen_t* screen;
}DisplayConnection;
DisplayConnection* createDisplayConnection();
int createWindow(DisplayConnection* dc,int ignored);
int  createArbitraryWindow(DisplayConnection* dc);
int mapWindow(DisplayConnection*dc,int win);
int  mapArbitraryWindow(DisplayConnection* dc);
void setProperties(DisplayConnection *dc,int win,char*classInstance,char*title,xcb_atom_t* atom);
void setArbitraryProperties(DisplayConnection* dc,int win);

#endif /* TESTS_UNITTESTS_H_ */
