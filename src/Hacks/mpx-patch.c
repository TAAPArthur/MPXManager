#define _GNU_SOURCE
#include <X11/extensions/XInput2.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <xcb/xinput.h>

#define BASE_NAME(SymbolName) base_ ## SymbolName
#define TYPE_NAME(SymbolName) SymbolName ## _t
#define INTERCEPT(ReturnType, SymbolName, ...) \
	typedef ReturnType (*TYPE_NAME(SymbolName))(__VA_ARGS__); \
	ReturnType SymbolName(__VA_ARGS__){ \
	void * const BASE_NAME(SymbolName) __attribute__((unused))= dlsym(RTLD_NEXT, # SymbolName); \

#define END_INTERCEPT }
#define BASE(SymbolName) ((TYPE_NAME(SymbolName))BASE_NAME(SymbolName))

static int getClientPointer() {
    char* var = getenv("CLIENT_POINTER");
    if(var && var[0])
        return atoi(var);
    return 2;
}
INTERCEPT(xcb_connection_t*, xcb_connect, const char* displayname, int* screenp) {
    xcb_connection_t* dis = BASE(xcb_connect)(displayname, screenp);
    if(dis)
        xcb_input_xi_set_client_pointer(dis, 0, getClientPointer());
    return dis;
}
END_INTERCEPT
INTERCEPT(Display*, XOpenDisplay, _Xconst char* displayname) {
    Display* dpy = BASE(XOpenDisplay)(displayname);
    if(dpy)
        XISetClientPointer(dpy, 0, getClientPointer());
    return dpy;
}
END_INTERCEPT
