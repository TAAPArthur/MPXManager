/**
 * @file xsession.c
 * @brief Create/destroy XConnection
 */
///\cond
#include <assert.h>
#include <string.h>
#include <strings.h>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <X11/Xlib-xcb.h>
///\endcond

#include "logger.h"
#include "xsession.h"
#include "globals.h"

void openXDisplay(){
#ifdef MPX_TESTING
    XInitThreads();
#endif
    LOG(LOG_LEVEL_DEBUG," connecting to XServe \n");
    for(int i=0;i<30;i++){
        dpy = XOpenDisplay(NULL);
        if(dpy)
            break;
        msleep(50);
    }
    if(!dpy){
        LOG(LOG_LEVEL_ERROR," Failed to connect to xserver\n");
        exit(EXIT_FAILURE);
    }
    dis = XGetXCBConnection(dpy);
    assert(!xcb_connection_has_error(dis));

    xcb_screen_iterator_t iter=xcb_setup_roots_iterator(xcb_get_setup(dis));
    screen=iter.data;
    //defaultScreenNumber=iter.index;
    root = screen->root;


    if(ewmh){
        xcb_ewmh_connection_wipe(ewmh);
        free(ewmh);
    }
    xcb_intern_atom_cookie_t *cookie;
    ewmh=malloc(sizeof(xcb_ewmh_connection_t));
    cookie = xcb_ewmh_init_atoms(dis, ewmh);

    xcb_ewmh_init_atoms_replies(ewmh, cookie, NULL);

    XSetEventQueueOwner(dpy, XCBOwnsEventQueue);
    xcb_set_close_down_mode(dis, XCB_CLOSE_DOWN_DESTROY_ALL);
}

void closeConnection(){
    LOG(LOG_LEVEL_INFO,"closing X connection\n");

    if(ewmh){
        xcb_ewmh_connection_wipe(ewmh);
        free(ewmh);
        ewmh=NULL;
    }
    if(dpy)XCloseDisplay(dpy);
    //dis=NULL;
    //dpy=NULL;
}

void quit(){
    LOG(LOG_LEVEL_INFO,"Shuttign down\n");
    //shuttingDown=1;
    closeConnection();
    LOG(LOG_LEVEL_INFO,"destroying context\n");
    destroyContext();
    exit(0);
}
