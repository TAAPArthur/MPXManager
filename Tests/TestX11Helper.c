#include <err.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "UnitTests.h"
#include "TestX11Helper.h"
#include "../default-rules.h"
#include "../logger.h"



DisplayConnection* createDisplayConnection(){
    DisplayConnection*dc=malloc(sizeof(DisplayConnection));
    for(int i=0;i<30;i++){
        dc->dpy = XOpenDisplay(NULL);
        if(dc->dpy)
            break;
        msleep(50);
    }
    if(!dc->dpy)
        exit(60);
    dc->dis = XGetXCBConnection(dc->dpy);
    dc->root=DefaultRootWindow(dc->dpy);
    dc->screen=xcb_setup_roots_iterator(xcb_get_setup(dc->dis)).data;
    dc->ewmh = malloc(sizeof(xcb_ewmh_connection_t));
    xcb_intern_atom_cookie_t *cookie;
    cookie = xcb_ewmh_init_atoms(dc->dis, dc->ewmh);
    xcb_ewmh_init_atoms_replies(dc->ewmh, cookie, (void *)0);

    return dc;
}


int createWindow(DisplayConnection *dc,int ignored){

    xcb_window_t window = xcb_generate_id (dc->dis);
    xcb_void_cookie_t c=xcb_create_window_checked (dc->dis,                    /* Connection          */
          XCB_COPY_FROM_PARENT,          /* depth (same as root)*/
          window,                        /* window Id           */
          dc->root,                  /* parent window       */
          0, 0,                          /* x, y                */
          150, 150,                      /* width, height       */
          10,                            /* border_width        */
          XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class               */
          dc->screen->root_visual,           /* visual              */
          ignored, (int[]){XCB_CW_OVERRIDE_REDIRECT} );                     /* masks, not used yet */

    xcb_generic_error_t* e=xcb_request_check(dc->dis,c);
    if(e){
        LOG(LOG_LEVEL_ERROR,"could not create window\n");
        assert(0);
    }
    xcb_icccm_wm_hints_t hints={.input=1};
    e=xcb_request_check(dc->dis,xcb_icccm_set_wm_hints_checked(dc->dis, window, &hints));
    if(e){
        LOG(LOG_LEVEL_ERROR,"could not set hintsfor window on creation\n");
        assert(0);
    }
    xcb_flush(dc->dis);

    return window;
}

int  createArbitraryWindow(DisplayConnection* dc){
    return createWindow(dc,0);
}

int mapWindow(DisplayConnection*dc,int win){
    assert(xcb_request_check(dc->dis, xcb_map_window_checked(dc->dis, win))==NULL);
    return win;
}
int  mapArbitraryWindow(DisplayConnection* dc){
    return mapWindow(dc, createArbitraryWindow(dc));
}

void setProperties(DisplayConnection *dc,int win,char*classInstance,char*title,xcb_atom_t* atom){
    int size=strlen(classInstance)+1;
    size+=strlen(&classInstance[size]);

    xcb_void_cookie_t cookies[]={
            xcb_ewmh_set_wm_name_checked(dc->ewmh, win, strlen(title), title),
            xcb_icccm_set_wm_class_checked(dc->dis, win, size, classInstance),
            xcb_ewmh_set_wm_window_type_checked(dc->ewmh, win, 1, atom)
    };
    for(int i=0;i<3;i++)
        assert(xcb_request_check(dc->dis,cookies[i])==NULL);
}
void setArbitraryProperties(DisplayConnection* dc,int win){
    setProperties(dc,win,"class\0instance","title",&dc->ewmh->_NET_WM_WINDOW_TYPE_NORMAL);
}

