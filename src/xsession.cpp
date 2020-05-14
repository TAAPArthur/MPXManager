#include <assert.h>
#include <string.h>
#include "unistd.h"

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <X11/Xlib-xcb.h>

#include "bindings.h"
#include "globals.h"
#include "logger.h"
#include "monitors.h"
#include "ringbuffer.h"
#include "system.h"
#include "threads.h"
#include "debug.h"
#include "time.h"
#include "user-events.h"
#include "xsession.h"


xcb_atom_t MPX_WM_INTERPROCESS_COM;
xcb_atom_t MPX_WM_INTERPROCESS_COM_STATUS;
xcb_atom_t MPX_WM_STATE_CENTER_X;
xcb_atom_t MPX_WM_STATE_CENTER_Y;
xcb_atom_t MPX_WM_STATE_NO_TILE;
xcb_atom_t MPX_WM_STATE_ROOT_FULLSCREEN;
xcb_atom_t WM_CHANGE_STATE;
xcb_atom_t WM_DELETE_WINDOW;
xcb_atom_t WM_SELECTION_ATOM;
xcb_atom_t WM_STATE;
xcb_atom_t WM_TAKE_FOCUS;
xcb_atom_t WM_WINDOW_ROLE;

xcb_gcontext_t graphics_context;
Display* dpy;
xcb_connection_t* dis;
WindowID root;
xcb_ewmh_connection_t* ewmh;
xcb_screen_t* screen;
const int defaultScreenNumber = 0;

/**
 * Window created by us to show to the world that an EWMH compliant WM is active
 */
static WindowID compliantWindowManagerIndicatorWindow;


WindowID getPrivateWindow(void) {
    if(!compliantWindowManagerIndicatorWindow) {
        compliantWindowManagerIndicatorWindow = createOverrideRedirectWindow();
        xcb_ewmh_set_wm_pid(ewmh, compliantWindowManagerIndicatorWindow, getpid());
    }
    return compliantWindowManagerIndicatorWindow;
}

static WindowID windowDivider[2] = {0};
WindowID getWindowDivider(bool upper) {
    if(!windowDivider[0]) {
        windowDivider[1] = createOverrideRedirectWindow();
        windowDivider[0] = createOverrideRedirectWindow();
    }
    return windowDivider[upper];
}


static std::unordered_map<uint32_t, WindowMask> atomStateToMask;
static std::unordered_map<uint32_t, WindowMask> atomActionToMask;
static void createMaskAtomMapping() {
    atomStateToMask[ewmh->_NET_WM_STATE_MODAL] = MODAL_MASK;
    atomStateToMask[ewmh->_NET_WM_STATE_ABOVE] = ABOVE_MASK | NO_TILE_MASK;
    atomStateToMask[MPX_WM_STATE_CENTER_X] = X_CENTERED_MASK;
    atomStateToMask[MPX_WM_STATE_CENTER_Y] = Y_CENTERED_MASK;
    atomStateToMask[MPX_WM_STATE_NO_TILE] = NO_TILE_MASK;
    atomStateToMask[MPX_WM_STATE_ROOT_FULLSCREEN] = ROOT_FULLSCREEN_MASK;
    atomStateToMask[ewmh->_NET_WM_STATE_BELOW] = BELOW_MASK | NO_TILE_MASK;
    atomStateToMask[ewmh->_NET_WM_STATE_FULLSCREEN] = FULLSCREEN_MASK;
    atomStateToMask[ewmh->_NET_WM_STATE_HIDDEN] = HIDDEN_MASK;
    atomStateToMask[ewmh->_NET_WM_STATE_STICKY] = STICKY_MASK;
    atomStateToMask[ewmh->_NET_WM_STATE_DEMANDS_ATTENTION] = URGENT_MASK;
    atomStateToMask[ewmh->_NET_WM_STATE_MAXIMIZED_HORZ] = X_MAXIMIZED_MASK;
    atomStateToMask[ewmh->_NET_WM_STATE_MAXIMIZED_VERT] = Y_MAXIMIZED_MASK;
    atomActionToMask[ewmh->_NET_WM_ACTION_ABOVE] = ABOVE_MASK | NO_TILE_MASK;
    atomActionToMask[ewmh->_NET_WM_ACTION_BELOW] = BELOW_MASK | NO_TILE_MASK;
    atomActionToMask[ewmh->_NET_WM_ACTION_FULLSCREEN] = FULLSCREEN_MASK;
    atomActionToMask[ewmh->_NET_WM_ACTION_MINIMIZE] = HIDDEN_MASK;
    atomActionToMask[ewmh->_NET_WM_ACTION_MOVE] = EXTERNAL_MOVE_MASK;
    atomActionToMask[ewmh->_NET_WM_ACTION_RESIZE] = EXTERNAL_RESIZE_MASK;
    atomActionToMask[ewmh->_NET_WM_ACTION_STICK] = STICKY_MASK;
    atomActionToMask[ewmh->_NET_WM_ACTION_MAXIMIZE_HORZ] = X_MAXIMIZED_MASK;
    atomActionToMask[ewmh->_NET_WM_ACTION_MAXIMIZE_VERT] = Y_MAXIMIZED_MASK;
}
WindowMask getMaskFromAtom(xcb_atom_t atom) {
    return atomStateToMask.count(atom) ? atomStateToMask[atom] : WindowMask(NO_MASK);
}
void getAtomsFromMask(WindowMask masks, ArrayList<xcb_atom_t>& arr, bool action) {
    for(auto pair : (action ? atomActionToMask : atomStateToMask))
        if((pair.second & masks) == pair.second)
            arr.add(pair.first);
}

xcb_atom_t getAtom(const char* name) {
    if(!name)return XCB_ATOM_NONE;
    xcb_intern_atom_reply_t* reply;
    reply = xcb_intern_atom_reply(dis, xcb_intern_atom(dis, 0, strlen(name), name), NULL);
    xcb_atom_t atom = reply->atom;
    free(reply);
    return atom;
}
std::string getAtomName(xcb_atom_t atom) {
    xcb_get_atom_name_reply_t* valueReply = xcb_get_atom_name_reply(dis, xcb_get_atom_name(dis, atom), NULL);
    std::string str = "";
    if(valueReply) {
        str = std::string(xcb_get_atom_name_name(valueReply), valueReply->name_len);
        free(valueReply);
    }
    return str;
}

std::shared_ptr<xcb_get_property_reply_t> getWindowProperty(WindowID win, xcb_atom_t atom, xcb_atom_t type) {
    xcb_get_property_reply_t* reply;
    std::shared_ptr<xcb_get_property_reply_t> result = NULL;
    xcb_get_property_cookie_t cookie = xcb_get_property(dis, 0, win, atom, type, 0, -1);
    if((reply = xcb_get_property_reply(dis, cookie, NULL)))
        if(xcb_get_property_value_length(reply))
            result = std::shared_ptr<xcb_get_property_reply_t>(reply, free);
        else free(reply);
    return result;
}
int getWindowPropertyValue(WindowID win, xcb_atom_t atom, xcb_atom_t type) {
    auto reply = getWindowProperty(win, atom, type);
    return reply ? *(int*)xcb_get_property_value(reply.get()) : 0;
}

std::string getWindowPropertyValueString(WindowID win, xcb_atom_t atom, xcb_atom_t type) {
    auto reply = getWindowProperty(win, atom, type);
    return reply ? std::string((char*)xcb_get_property_value(reply.get())) : "";
}

uint32_t generateID() {
    return xcb_generate_id(dis);
}

WindowID createWindow(WindowID parent, xcb_window_class_t clazz, uint32_t mask, uint32_t* valueList,
    const RectWithBorder& dims) {
    WindowID win = generateID();
    xcb_create_window(dis, XCB_COPY_FROM_PARENT, win, parent, dims.x, dims.y, dims.width, dims.height, dims.border, clazz,
        screen->root_visual, mask, valueList);
    return win;
}
int destroyWindow(WindowID win) {
    assert(win);
    LOG(LOG_LEVEL_DEBUG, "Destroying window %d", win);
    return catchError(xcb_destroy_window_checked(dis, win));
}
WindowID mapWindow(WindowID id) {
    TRACE("Mapping " << id);
    xcb_map_window(dis, id);
    return id;
}
void unmapWindow(WindowID id) {
    TRACE("UnMapping " << id);
    xcb_unmap_window(dis, id);
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
        ERROR("XLib error");
    LOG(LOG_LEVEL_ERROR, "error occurred with resource %d. Error code: %d %s (%d %d)", e->resource_id, e->error_code,
        opcodeToString(e->major_code), e->major_code, e->minor_code);
    int size = 256;
    char buff[size];
    XGetErrorText(dpy, e->error_code, buff, size);
    LOG(LOG_LEVEL_ERROR, "Error code %d %s ", e->error_code, buff) ;
    if((1 << e->error_code) & CRASH_ON_ERRORS) {
        ERROR("Crashing on error");
        LOG_RUN(LOG_LEVEL_ERROR, printStackTrace());
        LOG_RUN(LOG_LEVEL_DEBUG, printSummary());
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

bool hasXConnectionBeenOpened() {
    return dpy ? 1 : 0;
}
void openXDisplay(void) {
    XInitThreads();
    DEBUG(" connecting to XServer ");
    for(int i = 0; i < 20; i++) {
        dpy = XOpenDisplay(NULL);
        if(dpy)
            break;
        msleep(5);
    }
    if(!dpy) {
        ERROR(" Failed to connect to xserver");
        exit(EXIT_FAILURE);
    }
    dis = XGetXCBConnection(dpy);
    assert(!xcb_connection_has_error(dis));
    xcb_intern_atom_cookie_t* cookie;
    bool applyRule = ewmh == NULL;
    ewmh = (xcb_ewmh_connection_t*)malloc(sizeof(xcb_ewmh_connection_t));
    cookie = xcb_ewmh_init_atoms(dis, ewmh);
    xcb_ewmh_init_atoms_replies(ewmh, cookie, NULL);
    CREATE_ATOM(MPX_WM_INTERPROCESS_COM);
    CREATE_ATOM(MPX_WM_INTERPROCESS_COM_STATUS);
    CREATE_ATOM(MPX_WM_STATE_CENTER_X);
    CREATE_ATOM(MPX_WM_STATE_CENTER_Y);
    CREATE_ATOM(MPX_WM_STATE_NO_TILE);
    CREATE_ATOM(MPX_WM_STATE_ROOT_FULLSCREEN);
    CREATE_ATOM(WM_CHANGE_STATE);
    CREATE_ATOM(WM_DELETE_WINDOW);
    CREATE_ATOM(WM_STATE);
    CREATE_ATOM(WM_TAKE_FOCUS);
    CREATE_ATOM(WM_WINDOW_ROLE);
    screen = ewmh->screens[0];
    setRootDims(&screen->width_in_pixels);
    root = screen->root;
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
    createMaskAtomMapping();
    if(applyRule)
        applyEventRules(X_CONNECTION, NULL);
}

void closeConnection(void) {
    if(ewmh) {
        DEBUG("closing X connection");
        xcb_ewmh_connection_wipe(ewmh);
        free(ewmh);
        ewmh = NULL;
        compliantWindowManagerIndicatorWindow = 0;
        windowDivider[0] = 0;
        windowDivider[1] = 0;
        if(dpy)
            XCloseDisplay(dpy);
    }
}

#define _ADD_EVENT_TYPE_CASE(TYPE) case TYPE: return #TYPE
#define _ADD_GE_EVENT_TYPE_CASE(TYPE) case GENERIC_EVENT_OFFSET+TYPE: return #TYPE

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
            _ADD_GE_EVENT_TYPE_CASE(XCB_INPUT_DEVICE_CHANGED);
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
            _ADD_EVENT_TYPE_CASE(EXTRA_EVENT);
            _ADD_EVENT_TYPE_CASE(X_CONNECTION);
            _ADD_EVENT_TYPE_CASE(PRE_REGISTER_WINDOW);
            _ADD_EVENT_TYPE_CASE(POST_REGISTER_WINDOW);
            _ADD_EVENT_TYPE_CASE(UNREGISTER_WINDOW);
            _ADD_EVENT_TYPE_CASE(DEVICE_EVENT);
            _ADD_EVENT_TYPE_CASE(CLIENT_MAP_ALLOW);
            _ADD_EVENT_TYPE_CASE(PROPERTY_LOAD);
            _ADD_EVENT_TYPE_CASE(WINDOW_WORKSPACE_CHANGE);
            _ADD_EVENT_TYPE_CASE(MONITOR_WORKSPACE_CHANGE);
            _ADD_EVENT_TYPE_CASE(SCREEN_CHANGE);
            _ADD_EVENT_TYPE_CASE(WINDOW_MOVE);
            _ADD_EVENT_TYPE_CASE(TILE_WORKSPACE);
            _ADD_EVENT_TYPE_CASE(PERIODIC);
            _ADD_EVENT_TYPE_CASE(IDLE);
            _ADD_EVENT_TYPE_CASE(TRUE_IDLE);
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
std::string getAtomsAsString(const xcb_atom_t* atoms, int numberOfAtoms) {
    std::string result = "";
    for(int i = 0; i < numberOfAtoms; i++) {
        if(i)
            result += ", ";
        result += getAtomName(atoms[i]) + " (" + std::to_string(atoms[i]) + ")";
    }
    return result;
}
static volatile int idle;
static unsigned int periodCounter;
int getIdleCount() {
    return idle;
}

static RingBuffer < xcb_generic_event_t*, 1 << 12 > eventBuffer;
static uint32_t lastDetectedEventSequence = 0;

uint32_t getLastDetectedEventSequenceNumber() {return lastDetectedEventSequence;}

static inline void enqueueEvents(xcb_generic_event_t* event) {
    while(eventBuffer.push(xcb_poll_for_queued_event(dis)));
    assert(event);
    assert(eventBuffer.isEmpty() || eventBuffer.peekEnd());
    lastDetectedEventSequence = (eventBuffer.isEmpty() ? event : eventBuffer.peekEnd())->sequence;
}
static inline xcb_generic_event_t* pollForEvent() {
    xcb_generic_event_t* event;
    for(int i = POLL_COUNT - 1; i >= 0; i--) {
        msleep(POLL_INTERVAL);
        event = xcb_poll_for_event(dis);
        if(event) {
            enqueueEvents(event);
            return event;
        }
    }
    return NULL;
}
static inline xcb_generic_event_t* waitForEvent() {
    xcb_generic_event_t* event = xcb_wait_for_event(dis);
    if(event)
        enqueueEvents(event);
    return event;
}

static inline xcb_generic_event_t* getNextEvent() {
    if(EVENT_PERIOD && ++periodCounter > EVENT_PERIOD) {
        periodCounter = 0;
        applyEventRules(PERIODIC, NULL);
    }
    static xcb_generic_event_t* event;
    event = eventBuffer.pop();
    if(!event)
        event = pollForEvent();
    if(!event && !xcb_connection_has_error(dis)) {
        periodCounter = 0;
        lock();
        applyBatchEventRules();
        if(applyEventRules(IDLE, NULL)) {
            flush();
            event = pollForEvent();
            if(event) {
                unlock();
                return event;
            }
        }
        applyEventRules(TRUE_IDLE, NULL);
        idle++;
        flush();
        unlock();
        INFO("Idle " << idle);
        if(!isShuttingDown())
            event = waitForEvent();
    }
    return event;
}

int isSyntheticEvent() {
    xcb_generic_event_t* event = (xcb_generic_event_t*)getLastEvent();
    return event->response_type > 127;
}


void runEventLoop() {
    xcb_generic_event_t* event = NULL;
    while(!isShuttingDown() && dis) {
        event = getNextEvent();
        if(isShuttingDown() || xcb_connection_has_error(dis) || !event) {
            if(event)
                free(event);
            break;
        }
        int type = event->response_type & 127;
        lock();
        // TODO pre event processing rule
        setLastEvent(event);
        type = type < LASTEvent ? type : EXTRA_EVENT;
        applyEventRules(type, NULL);
        setLastEvent(NULL);
        free(event);
        unlock();
#ifdef DEBUG
        XSync(dpy, 0);
#endif
        TRACE("event processed");
    }
    if(isShuttingDown() || xcb_connection_has_error(dis) || !event) {
        while(event = eventBuffer.pop())
            free(event);
        if(isShuttingDown())
            INFO("shutting down");
    }
    INFO("Exited event loop");
}
int loadGenericEvent(xcb_ge_generic_event_t* event) {
    LOG(LOG_LEVEL_TRACE, "processing generic event; ext: %d type: %d event type %d seq %d",
        event->extension, event->response_type, event->event_type, event->sequence);
    if(event->extension == xcb_get_extension_data(dis, &xcb_input_id)->major_opcode) {
        int type = event->event_type + GENERIC_EVENT_OFFSET;
        return type;
    }
    LOG(LOG_LEVEL_WARN, "Could not process generic event");
    return 0;
}
///Last registered event
static xcb_generic_event_t* lastEvent;
void setLastEvent(void* event) {
    lastEvent = (xcb_generic_event_t*)event;
}
xcb_generic_event_t* getLastEvent(void) {
    return lastEvent;
}
uint16_t getCurrentSequenceNumber(void) {
    return lastEvent ? lastEvent->sequence : 0;
}
