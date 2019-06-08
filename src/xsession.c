/**
 * @file xsession.c
 * @brief Create/destroy XConnection
 */

#include <assert.h>
#include <string.h>
#include <strings.h>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <X11/Xlib-xcb.h>


#include "logger.h"
#include "xsession.h"


xcb_atom_t WM_TAKE_FOCUS;
xcb_atom_t WM_DELETE_WINDOW;
xcb_atom_t WM_STATE_NO_TILE;
xcb_atom_t WM_STATE_ROOT_FULLSCREEN;
xcb_atom_t WM_SELECTION_ATOM;
xcb_atom_t WM_INTERPROCESS_COM;
xcb_atom_t WM_WORKSPACE_LAYOUT_NAMES;
xcb_atom_t WM_WORKSPACE_MONITORS;
xcb_atom_t WM_WORKSPACE_WINDOWS;
xcb_atom_t WM_MASTER_WINDOWS;
xcb_atom_t WM_ACTIVE_MASTER;
xcb_atom_t WM_WORKSPACE_LAYOUT_INDEXES;

xcb_gcontext_t graphics_context;

/**
 * Window created by us to show to the world that an EWMH compliant WM is active
 */
static WindowID compliantWindowManagerIndicatorWindow;

WindowID getPrivateWindow(void){
    if(!compliantWindowManagerIndicatorWindow){
        int overrideRedirect = 1;
        compliantWindowManagerIndicatorWindow = xcb_generate_id(dis);
        xcb_create_window(dis, 0, compliantWindowManagerIndicatorWindow, root, 0, 0, 1, 1, 0,
                          XCB_WINDOW_CLASS_INPUT_ONLY, 0, XCB_CW_OVERRIDE_REDIRECT, &overrideRedirect);
    }
    return compliantWindowManagerIndicatorWindow;
}

xcb_atom_t getAtom(char* name){
    if(!name)return XCB_ATOM_NONE;
    xcb_intern_atom_reply_t* reply;
    reply = xcb_intern_atom_reply(dis, xcb_intern_atom(dis, 0, 32, name), NULL);
    xcb_atom_t atom = reply->atom;
    free(reply);
    return atom;
}
void getAtomValue(xcb_atom_t atom, char** value){
    xcb_get_atom_name_reply_t* valueReply = xcb_get_atom_name_reply(dis, xcb_get_atom_name(dis, atom), NULL);
    if(valueReply){
        *value = malloc((valueReply->name_len + 1) * sizeof(char));
        memcpy(*value, xcb_get_atom_name_name(valueReply), (valueReply->name_len)*sizeof(char));
        (*value)[valueReply->name_len] = 0;
        free(valueReply);
    }
    else *value = NULL;
}
Display* dpy;
xcb_connection_t* dis;
int root;
xcb_ewmh_connection_t* ewmh;
xcb_screen_t* screen;
int defaultScreenNumber;

static xcb_gcontext_t create_graphics_context(void){
    uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_GRAPHICS_EXPOSURES;
    uint32_t values[3] = { screen->black_pixel, screen->white_pixel, 0};
    graphics_context = xcb_generate_id(dis);
    xcb_void_cookie_t cookie = xcb_create_gc_checked(dis,
                               graphics_context, root,
                               mask, values);
    catchError(cookie);
    return graphics_context;
}
/**
 * Shorthand macro to init a X11 atom
 * @param name the name of the atom to init
 */
#define _CREATE_ATOM(name)name=getAtom(# name);
void openXDisplay(void){
    XInitThreads();
    LOG(LOG_LEVEL_DEBUG, " connecting to XServe \n");
    for(int i = 0; i < 30; i++){
        dpy = XOpenDisplay(NULL);
        if(dpy)
            break;
        msleep(50);
    }
    if(!dpy){
        LOG(LOG_LEVEL_ERROR, " Failed to connect to xserver\n");
        exit(EXIT_FAILURE);
    }
    dis = XGetXCBConnection(dpy);
    assert(!xcb_connection_has_error(dis));
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(dis));
    screen = iter.data;
    //defaultScreenNumber=iter.index;
    root = screen->root;
    if(ewmh){
        xcb_ewmh_connection_wipe(ewmh);
        free(ewmh);
    }
    xcb_intern_atom_cookie_t* cookie;
    ewmh = malloc(sizeof(xcb_ewmh_connection_t));
    cookie = xcb_ewmh_init_atoms(dis, ewmh);
    xcb_ewmh_init_atoms_replies(ewmh, cookie, NULL);
    _CREATE_ATOM(WM_TAKE_FOCUS);
    _CREATE_ATOM(WM_DELETE_WINDOW);
    _CREATE_ATOM(WM_STATE_NO_TILE);
    _CREATE_ATOM(WM_STATE_ROOT_FULLSCREEN);
    _CREATE_ATOM(WM_INTERPROCESS_COM);
    _CREATE_ATOM(WM_WORKSPACE_LAYOUT_NAMES);
    _CREATE_ATOM(WM_WORKSPACE_MONITORS);
    _CREATE_ATOM(WM_WORKSPACE_LAYOUT_INDEXES);
    _CREATE_ATOM(WM_WORKSPACE_WINDOWS);
    _CREATE_ATOM(WM_MASTER_WINDOWS);
    _CREATE_ATOM(WM_ACTIVE_MASTER);
    char selectionName[] = {'W', 'M', '_', 'S', '0' + defaultScreenNumber, '\0'};
    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(dis,
                                     xcb_intern_atom(dis, 0, strlen(selectionName), selectionName), NULL);
    assert(reply);
    WM_SELECTION_ATOM = reply->atom;
    free(reply);
    create_graphics_context();
    XSetEventQueueOwner(dpy, XCBOwnsEventQueue);
    compliantWindowManagerIndicatorWindow = 0;
}

void closeConnection(void){
    LOG(LOG_LEVEL_INFO, "closing X connection\n");
    if(ewmh){
        xcb_ewmh_connection_wipe(ewmh);
        free(ewmh);
        ewmh = NULL;
    }
    if(dpy)XCloseDisplay(dpy);
    //dis=NULL;
    //dpy=NULL;
}

void flush(void){
    XFlush(dpy);
    xcb_flush(dis);
}
