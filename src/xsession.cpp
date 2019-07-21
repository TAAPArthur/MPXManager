/**
 * @file xsession.cpp
 * @brief Create/destroy XConnection
 */

#include <assert.h>
#include <string.h>
#include <strings.h>

#ifndef NO_XRANDR
    #include <xcb/randr.h>
#endif
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <X11/Xlib-xcb.h>


#include "logger.h"
#include "bindings.h"
#include "xsession.h"
#include "system.h"
#include "user-events.h"
#include "globals.h"
#include "time.h"
#include "monitors.h"
#include <string>
#include <iostream>


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
typedef xcb_window_t WindowID;
Display* dpy;
xcb_connection_t* dis;
WindowID root;
xcb_ewmh_connection_t* ewmh;
xcb_screen_t* screen;
int defaultScreenNumber;

/**
 * Window created by us to show to the world that an EWMH compliant WM is active
 */
static WindowID compliantWindowManagerIndicatorWindow;

WindowID getPrivateWindow(void) {
    if(!compliantWindowManagerIndicatorWindow) {
        int overrideRedirect = 1;
        compliantWindowManagerIndicatorWindow = xcb_generate_id(dis);
        xcb_create_window(dis, 0, compliantWindowManagerIndicatorWindow, root, 0, 0, 1, 1, 0,
                          XCB_WINDOW_CLASS_INPUT_ONLY, 0, XCB_CW_OVERRIDE_REDIRECT, &overrideRedirect);
    }
    return compliantWindowManagerIndicatorWindow;
}

xcb_atom_t getAtom(const char* name) {
    if(!name)return XCB_ATOM_NONE;
    xcb_intern_atom_reply_t* reply;
    reply = xcb_intern_atom_reply(dis, xcb_intern_atom(dis, 0, 32, name), NULL);
    xcb_atom_t atom = reply->atom;
    free(reply);
    return atom;
}
std::string getAtomValue(xcb_atom_t atom) {
    xcb_get_atom_name_reply_t* valueReply = xcb_get_atom_name_reply(dis, xcb_get_atom_name(dis, atom), NULL);
    std::string str = "";
    if(valueReply) {
        str = std::string(xcb_get_atom_name_name(valueReply), valueReply->name_len);
        //*value = (char*)malloc((valueReply->name_len + 1) * sizeof(char));
        //memcpy(*value, xcb_get_atom_name_name(valueReply), (valueReply->name_len)*sizeof(char));
        //(*value)[valueReply->name_len] = 0;
        free(valueReply);
    }
    return str;
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
void logError(xcb_generic_error_t* e, bool xlibError) {
    if(xlibError)
        LOG(LOG_LEVEL_ERROR, "XLib error\n");
    LOG(LOG_LEVEL_ERROR, "error occurred with resource %d. Error code: %d %s (%d %d)\n", e->resource_id, e->error_code,
        opcodeToString(e->major_code), e->major_code, e->minor_code);
    int size = 256;
    char buff[size];
    XGetErrorText(dpy, e->error_code, buff, size);
    LOG(LOG_LEVEL_ERROR, "Error code %d %s \n", e->error_code, buff) ;
    if((1 << e->error_code) & CRASH_ON_ERRORS) {
        LOG(LOG_LEVEL_ERROR, "Crashing on error\n");
        LOG_RUN(LOG_LEVEL_ERROR, printStackTrace());
        quit(1);
    }
}
static int handleXLibError(Display* dpy __attribute__((unused)), XErrorEvent* e) {
    xcb_generic_error_t error = {.response_type = (uint8_t)e->type, .error_code = e->error_code, .resource_id = (uint32_t)e->resourceid, .minor_code = e->minor_code, .major_code = e->request_code};
    logError(&error, 1);
    return 0;
}

static xcb_gcontext_t create_graphics_context(void) {
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
void openXDisplay(void) {
    XInitThreads();
    LOG(LOG_LEVEL_DEBUG, " connecting to XServer \n");
    for(int i = 0; i < 20; i++) {
        dpy = XOpenDisplay(NULL);
        if(dpy)
            break;
        msleep(5);
    }
    if(!dpy) {
        LOG(LOG_LEVEL_ERROR, " Failed to connect to xserver\n");
        exit(EXIT_FAILURE);
    }
    dis = XGetXCBConnection(dpy);
    assert(!xcb_connection_has_error(dis));
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(dis));
    screen = iter.data;
    //defaultScreenNumber=iter.index;
    setRootDims(&screen->width_in_pixels);
    root = screen->root;
    xcb_intern_atom_cookie_t* cookie;
    ewmh = (xcb_ewmh_connection_t*)malloc(sizeof(xcb_ewmh_connection_t));
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
    char selectionName[8];
    sprintf(selectionName, "WM_S%d", '0' + defaultScreenNumber);
    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(dis,
                                     xcb_intern_atom(dis, 0, strlen(selectionName), selectionName), NULL);
    assert(reply);
    WM_SELECTION_ATOM = reply->atom;
    free(reply);
    create_graphics_context();
    XSetEventQueueOwner(dpy, XCBOwnsEventQueue);
    XSetErrorHandler(handleXLibError);
    compliantWindowManagerIndicatorWindow = 0;
}

void closeConnection(void) {
    if(ewmh) {
        LOG(LOG_LEVEL_INFO, "closing X connection\n");
        xcb_ewmh_connection_wipe(ewmh);
        free(ewmh);
        ewmh = NULL;
        if(dpy)
            XCloseDisplay(dpy);
    }
}

#define _ADD_EVENT_TYPE_CASE(TYPE) case TYPE: return  #TYPE
#define _ADD_GE_EVENT_TYPE_CASE(TYPE) case GENERIC_EVENT_OFFSET+TYPE: return  #TYPE

const char* eventTypeToString(int type) {
    switch(type) {
            _ADD_EVENT_TYPE_CASE(XCB_EXPOSE);
            _ADD_EVENT_TYPE_CASE(XCB_VISIBILITY_NOTIFY);
            _ADD_EVENT_TYPE_CASE(XCB_CREATE_NOTIFY);
            _ADD_EVENT_TYPE_CASE(XCB_DESTROY_NOTIFY);
            _ADD_EVENT_TYPE_CASE(XCB_UNMAP_NOTIFY);
            _ADD_EVENT_TYPE_CASE(XCB_MAP_NOTIFY);
            _ADD_EVENT_TYPE_CASE(XCB_MAP_REQUEST);
            _ADD_EVENT_TYPE_CASE(XCB_REPARENT_NOTIFY);
            _ADD_EVENT_TYPE_CASE(XCB_CONFIGURE_NOTIFY);
            _ADD_EVENT_TYPE_CASE(XCB_CONFIGURE_REQUEST);
            _ADD_EVENT_TYPE_CASE(XCB_PROPERTY_NOTIFY);
            _ADD_EVENT_TYPE_CASE(XCB_SELECTION_CLEAR);
            _ADD_EVENT_TYPE_CASE(XCB_CLIENT_MESSAGE);
            _ADD_EVENT_TYPE_CASE(XCB_GE_GENERIC);
            _ADD_GE_EVENT_TYPE_CASE(XCB_INPUT_KEY_PRESS);
            _ADD_GE_EVENT_TYPE_CASE(XCB_INPUT_KEY_RELEASE);
            _ADD_GE_EVENT_TYPE_CASE(XCB_INPUT_BUTTON_PRESS);
            _ADD_GE_EVENT_TYPE_CASE(XCB_INPUT_BUTTON_RELEASE);
            _ADD_GE_EVENT_TYPE_CASE(XCB_INPUT_MOTION);
            _ADD_GE_EVENT_TYPE_CASE(XCB_INPUT_ENTER);
            _ADD_GE_EVENT_TYPE_CASE(XCB_INPUT_LEAVE);
            _ADD_GE_EVENT_TYPE_CASE(XCB_INPUT_FOCUS_IN);
            _ADD_GE_EVENT_TYPE_CASE(XCB_INPUT_FOCUS_OUT);
            _ADD_GE_EVENT_TYPE_CASE(XCB_INPUT_HIERARCHY);
            _ADD_EVENT_TYPE_CASE(ExtraEvent);
            _ADD_EVENT_TYPE_CASE(onXConnection);
            _ADD_EVENT_TYPE_CASE(ClientMapAllow);
            _ADD_EVENT_TYPE_CASE(PreRegisterWindow);
            _ADD_EVENT_TYPE_CASE(PostRegisterWindow);
            _ADD_EVENT_TYPE_CASE(onScreenChange);
            _ADD_EVENT_TYPE_CASE(onWindowMove);
            _ADD_EVENT_TYPE_CASE(Periodic);
            _ADD_EVENT_TYPE_CASE(Idle);
        case 0:
            return "Error";
        default:
            return "unknown event";
    }
}
int getButtonDetailOrKeyCode(int buttonOrKey) {
    return isButton(buttonOrKey) ? buttonOrKey : getKeyCode(buttonOrKey);
}
int getKeyCode(int keysym) {
    return XKeysymToKeycode(dpy, keysym);
}
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
void dumpAtoms(xcb_atom_t* atoms, int numberOfAtoms) {
    for(int i = 0; i < numberOfAtoms; i++) {
        if(i)
            std::cout << ", ";
        std::cout << getAtomValue(atoms[i]);
    }
    std::cout << "\n";
}
static volatile int idle;
static unsigned int periodCounter;
int getIdleCount() {
    return idle;
}

static inline xcb_generic_event_t* pollForEvent() {
    xcb_generic_event_t* event;
    for(int i = 0; i < POLL_COUNT; i++) {
        if(i)
            msleep(POLL_INTERVAL);
        event = xcb_poll_for_event(dis);
        if(event)return event;
    }
    return NULL;
}
static inline xcb_generic_event_t* getNextEvent() {
    if(EVENT_PERIOD && ++periodCounter > EVENT_PERIOD) {
        periodCounter = 0;
        applyEventRules(Periodic, NULL);
    }
    static xcb_generic_event_t* event;
    event = pollForEvent();
    if(!event && !xcb_connection_has_error(dis)) {
        periodCounter = 0;
        lock();
        applyBatchEventRules();
        applyEventRules(Periodic, NULL);
        if(applyEventRules(Idle, NULL)) {
            flush();
            event = pollForEvent();
            if(event) {
                unlock();
                return event;
            }
        }
        applyEventRules(TrueIdle, NULL);
        idle++;
        flush();
        unlock();
        LOG(LOG_LEVEL_VERBOSE, "Idle %d\n", idle);
        if(!isShuttingDown())
            event = xcb_wait_for_event(dis);
    }
    return event;
}

int isSyntheticEvent() {
    xcb_generic_event_t* event = (xcb_generic_event_t*)getLastEvent();
    return event->response_type > 127;
}


void* runEventLoop(void* arg __attribute__((unused))) {
    xcb_generic_event_t* event;
    int xrandrFirstEvent = -1;
#ifndef NO_XRANDR
    xrandrFirstEvent = xcb_get_extension_data(dis, &xcb_randr_id)->first_event;
#endif
    LOG(LOG_LEVEL_TRACE, "starting event loop; xrandr event %d\n", xrandrFirstEvent);
    while(!isShuttingDown() && dis) {
        event = getNextEvent();
        if(isShuttingDown() || xcb_connection_has_error(dis) || !event) {
            if(event)free(event);
            if(isShuttingDown())
                LOG(LOG_LEVEL_INFO, "shutting down\n");
            break;
        }
        int type = event->response_type & 127;
        lock();
        // TODO pre event processing rule
        setLastEvent(event);
        type = type < LASTEvent ? type : type == xrandrFirstEvent ? onScreenChange : ExtraEvent;
        applyEventRules(type, NULL);
        unlock();
        free(event);
#ifdef DEBUG
        XSync(dpy, 0);
#endif
        LOG(LOG_LEVEL_VERBOSE, "event processed\n");
    }
    LOG(LOG_LEVEL_DEBUG, "Exited event loop\n");
    return NULL;
}
int loadGenericEvent(xcb_ge_generic_event_t* event) {
    LOG(LOG_LEVEL_ALL, "processing generic event; ext: %d type: %d event type %d  seq %d\n",
        event->extension, event->response_type, event->event_type, event->sequence);
    if(event->extension == xcb_get_extension_data(dis, &xcb_input_id)->major_opcode) {
        int type = event->event_type + GENERIC_EVENT_OFFSET;
        return type;
    }
    return 0;
}
///Last registered event
static void* lastEvent;
void setLastEvent(void* event) {
    lastEvent = event;
}
void* getLastEvent(void) {
    return lastEvent;
}
