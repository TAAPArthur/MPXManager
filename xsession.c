/**
 * @file xsession.c
 * @brief Create/destroy XConnection
 */

#include <assert.h>
#include <string.h>
#include <strings.h>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/randr.h>

#include <X11/Xlib-xcb.h>

#include "ewmh.h"
#include "mywm-util.h"
#include "defaults.h"
#include "xsession.h"
#include "logger.h"
#include "devices.h"
#include "bindings.h"
#include "events.h"


/*TODO remove
static int shuttingDown=0;
int isShuttingDown(){
    return shuttingDown;
}
*/
void openXDisplay(){
    LOG(LOG_LEVEL_DEBUG," connecting to XServe \n");
    for(int i=0;i<30;i++){
        dpy = XOpenDisplay(NULL);
        if(dpy)
            break;
        msleep(50);
    }
    if(!dpy){
        LOG(LOG_LEVEL_ERROR," Failed to connect to xserver\n");
        exit(60);
    }
    dis = XGetXCBConnection(dpy);
    if(xcb_connection_has_error(dis)){
        LOG(LOG_LEVEL_ERROR," failed to conver xlib display to xcb version\n");
        exit(1);
    }
    XSetEventQueueOwner(dpy, XCBOwnsEventQueue);
    xcb_set_close_down_mode(dis, XCB_CLOSE_DOWN_DESTROY_ALL);
}
void connectToXserver(){

    openXDisplay();

    screen=xcb_setup_roots_iterator (xcb_get_setup (dis)).data;

    root = DefaultRootWindow(dpy);
    ewmh = malloc(sizeof(xcb_ewmh_connection_t));
    xcb_intern_atom_cookie_t *cookie;
    cookie = xcb_ewmh_init_atoms(dis, ewmh);
    xcb_ewmh_init_atoms_replies(ewmh, cookie, NULL);

    initCurrentMasters();
    assert(getActiveMaster()!=NULL);
    detectMonitors();
    registerForEvents();
    applyEventRules(eventRules[onXConnection],NULL);
}

void detectMonitors(){
    xcb_randr_get_monitors_cookie_t cookie =xcb_randr_get_monitors(dis, root, 1);
    xcb_randr_get_monitors_reply_t *monitors=xcb_randr_get_monitors_reply(dis, cookie, NULL);
    assert(monitors);
    xcb_randr_monitor_info_iterator_t iter=xcb_randr_get_monitors_monitors_iterator(monitors);
    while(iter.rem){
        xcb_randr_monitor_info_t *monitorInfo = iter.data;
        addMonitor(monitorInfo->name,monitorInfo->primary,&monitorInfo->x);
        xcb_randr_monitor_info_next(&iter);
    }
    free(monitors);
}

void closeConnection(){
    LOG(LOG_LEVEL_INFO,"closing X connection\n");

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
