#include <err.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>

typedef struct {
    Display *dpy;
    xcb_connection_t *dis;
    xcb_ewmh_connection_t *ewmh;
    xcb_window_t root;
    int screen;
}DisplayConnection;
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
    dc->screen=DefaultScreen(dc->dpy);
    dc->ewmh = malloc(sizeof(xcb_ewmh_connection_t));
    xcb_intern_atom_cookie_t *cookie;
    cookie = xcb_ewmh_init_atoms(dc->dis, dc->ewmh);
    xcb_ewmh_init_atoms_replies(dc->ewmh, cookie, (void *)0);

    return dc;
}
void closeDisplayConnection(DisplayConnection*dc){
    XCloseDisplay(dc->dpy);

}
#define INIT DisplayConnection* dc = createDisplayConnection();{
#define END closeDisplayConnection(dc);exit(0);}

#define START_MY_WM  onStartup();\
        pthread_t pThread; assert(pthread_create(&pThread, NULL, runEventLoop, NULL)==0);
#define KILL_MY_WM  pthread_kill(pThread,0);

#define WAIT_FOR(EXPR)  while(EXPR)msleep(100);
#define WAIT_UNTIL_TRUE(EXPR) WAIT_FOR(!EXPR)
Window createWindow(DisplayConnection* dc,int x,int y,int w,int h){
    int win= XCreateSimpleWindow(dc->dpy,dc->root,x,y,w,h,1,
            BlackPixel(dc->dpy,dc->screen),WhitePixel(dc->dpy,dc->screen));

    xcb_icccm_wm_hints_t hints={.input=1};
    xcb_icccm_set_wm_hints_checked(dc->dis, win, &hints);
    XFlush(dc->dpy);
    return win;
}
Window  createArbitraryWindow(DisplayConnection* dc){
    return createWindow(dc, 0, 0, 1, 1);
}

void distroyWindow(DisplayConnection*dc,int win){

    assert(NULL==xcb_request_check(dc->dis, xcb_destroy_window_checked(dc->dis, win)));
}
int mapWindow(DisplayConnection*dc,int win){
    assert(xcb_request_check(dc->dis, xcb_map_window_checked(dc->dis, win))==NULL);
    return win;
}
Window  mapArbitraryWindow(DisplayConnection* dc){
    return mapWindow(dc, createArbitraryWindow(dc));
}

void setProperties(DisplayConnection *dc,Window win,char*classInstance,char*title,xcb_atom_t* atom){
    int size=strlen(classInstance)+1;
    size+=strlen(&classInstance[size]);

    xcb_void_cookie_t cookies[]={
            xcb_ewmh_set_wm_name_checked(dc->ewmh, win, strlen(title), title),
            xcb_icccm_set_wm_class_checked(dc->dis, win, size, classInstance),
            xcb_ewmh_set_wm_window_type_checked(dc->ewmh, win, 1, atom)
    };
    for(int i=0;i<LEN(cookies);i++)
        assert(xcb_request_check(dc->dis,cookies[i])==NULL);
}
void setArbitraryProperties(DisplayConnection* dc,Window win){
    setProperties(dc,win,"class\0instance","title",&dc->ewmh->_NET_WM_WINDOW_TYPE_NORMAL);
}
