
#include "unistd.h"
#include <assert.h>
#include <string.h>

#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

#include "../system.h"
#include "../util/debug.h"
#include "../util/logger.h"
#include "xsession.h"

#define _ADD_EVENT_TYPE_CASE(TYPE) case TYPE: return #TYPE
#define _ADD_GE_EVENT_TYPE_CASE(TYPE) case GENERIC_EVENT_OFFSET+TYPE: return #TYPE
const char* opcodeToString(int opcode) {
    switch(opcode) {
            _ADD_EVENT_TYPE_CASE(XCB_CREATE_WINDOW);
            _ADD_EVENT_TYPE_CASE(XCB_CHANGE_WINDOW_ATTRIBUTES);
            _ADD_EVENT_TYPE_CASE(XCB_GET_WINDOW_ATTRIBUTES);
            _ADD_EVENT_TYPE_CASE(XCB_DESTROY_WINDOW);
            _ADD_EVENT_TYPE_CASE(XCB_MAP_WINDOW);
            _ADD_EVENT_TYPE_CASE(XCB_CONFIGURE_WINDOW);
            _ADD_EVENT_TYPE_CASE(XCB_CHANGE_PROPERTY);
        default:
            return "unknown code";
    }
}

int catchError(xcb_void_cookie_t cookie) {
    xcb_generic_error_t* e = xcb_request_check(dis, cookie);
    int errorCode = 0;
    if(e) {
        errorCode = e->error_code;
        logError(e);
        free(e);
    }
    return errorCode;
}
int catchErrorSilent(xcb_void_cookie_t cookie) {
    xcb_generic_error_t* e = xcb_request_check(dis, cookie);
    int errorCode = 0;
    if(e) {
        errorCode = e->error_code;
        free(e);
    }
    return errorCode;
}
void logError(xcb_generic_error_t* e) {
    ERROR("error occurred with seq %d resource %d. Error code: %d %s (%d %d)", e->sequence, e->resource_id, e->error_code,
        opcodeToString(e->major_code), e->major_code, e->minor_code);
    if((1 << e->error_code) & CRASH_ON_ERRORS) {
        ERROR("Crashing on error");
        LOG_RUN(LOG_LEVEL_DEBUG, printSummary());
        quit(X_ERROR);
    }
}
