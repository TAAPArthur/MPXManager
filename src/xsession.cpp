#include <assert.h>
#include <string.h>

#ifndef NO_XRANDR
#include <xcb/randr.h>
#endif
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <X11/Xlib-xcb.h>

#include "bindings.h"
#include "globals.h"
#include "logger.h"
#include "monitors.h"
#include "system.h"
#include "time.h"
#include "user-events.h"
#include "xsession.h"


xcb_atom_t MAX_DEVICES;
xcb_atom_t WM_ACTIVE_MASTER;
xcb_atom_t WM_DELETE_WINDOW;
xcb_atom_t WM_FAKE_MONITORS;
xcb_atom_t WM_HINTS;
xcb_atom_t WM_INTERPROCESS_COM;
xcb_atom_t WM_MASTER_WINDOWS;
xcb_atom_t WM_NAME;
xcb_atom_t WM_SELECTION_ATOM;
xcb_atom_t WM_STATE_NO_TILE;
xcb_atom_t WM_STATE_ROOT_FULLSCREEN;
xcb_atom_t WM_TAKE_FOCUS;
xcb_atom_t WM_WINDOW_ROLE;
xcb_atom_t WM_WORKSPACE_LAYOUT_INDEXES;
xcb_atom_t WM_WORKSPACE_LAYOUT_NAMES;
xcb_atom_t WM_WORKSPACE_MONITORS;
xcb_atom_t WM_WORKSPACE_WINDOWS;

static int maxNumDevices;
uint32_t getMaxNumberOfMasterDevices(bool force) {
    return (getMaxNumberOfDevices(force) - 2) / 4;
}
uint32_t getMaxNumberOfDevices(bool force) {
    if(force || maxNumDevices == 0) {
        maxNumDevices = 40; // the max devices as of 2019-10-19
        xcb_get_property_cookie_t cookie;
        xcb_get_property_reply_t* reply;
        LOG(LOG_LEVEL_TRACE, "Loading active Master\n");
        cookie = xcb_get_property(dis, 0, root, MAX_DEVICES, XCB_ATOM_INTEGER, 0, sizeof(int));
        if((reply = xcb_get_property_reply(dis, cookie, NULL)) && xcb_get_property_value_length(reply))
            maxNumDevices = *(int*)xcb_get_property_value(reply);
        else
            xcb_change_property(dis, XCB_PROP_MODE_REPLACE, root, MAX_DEVICES, XCB_ATOM_INTEGER, 32, 1, &maxNumDevices);
        if(reply)
            free(reply);
    }
    return maxNumDevices;
}
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
        int overrideRedirect = 1;
        compliantWindowManagerIndicatorWindow = xcb_generate_id(dis);
        xcb_create_window(dis, 0, compliantWindowManagerIndicatorWindow, root, 0, 0, 1, 1, 0,
            XCB_WINDOW_CLASS_INPUT_ONLY, 0, XCB_CW_OVERRIDE_REDIRECT, &overrideRedirect);
    }
    return compliantWindowManagerIndicatorWindow;
}


static std::unordered_map<uint32_t, WindowMask> atomStateToMask;
static std::unordered_map<uint32_t, WindowMask> atomActionToMask;
static void createMaskAtomMapping() {
    atomStateToMask[ewmh->_NET_WM_STATE_ABOVE] = ABOVE_MASK;
    atomStateToMask[ewmh->_NET_WM_STATE_BELOW] = BELOW_MASK;
    atomStateToMask[ewmh->_NET_WM_STATE_FULLSCREEN] = FULLSCREEN_MASK;
    atomStateToMask[ewmh->_NET_WM_STATE_HIDDEN] = HIDDEN_MASK;
    atomStateToMask[WM_STATE_NO_TILE] = NO_TILE_MASK;
    atomStateToMask[WM_STATE_ROOT_FULLSCREEN] = ROOT_FULLSCREEN_MASK;
    atomStateToMask[ewmh->_NET_WM_STATE_STICKY] = STICKY_MASK;
    atomStateToMask[ewmh->_NET_WM_STATE_DEMANDS_ATTENTION] = URGENT_MASK;
    atomStateToMask[ewmh->_NET_WM_STATE_MAXIMIZED_HORZ] = X_MAXIMIZED_MASK;
    atomStateToMask[ewmh->_NET_WM_STATE_MAXIMIZED_VERT] = Y_MAXIMIZED_MASK;
    atomActionToMask[ewmh->_NET_WM_ACTION_ABOVE] = ABOVE_MASK;
    atomActionToMask[ewmh->_NET_WM_ACTION_BELOW] = BELOW_MASK;
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
        if(pair.second & masks)
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
        //*value = (char*)malloc((valueReply->name_len + 1) * sizeof(char));
        //memcpy(*value, xcb_get_atom_name_name(valueReply), (valueReply->name_len)*sizeof(char));
        //(*value)[valueReply->name_len] = 0;
        free(valueReply);
    }
    return str;
}

WindowID createWindow(WindowID parent, xcb_window_class_t clazz, uint32_t mask, uint32_t* valueList,
    const RectWithBorder& dims) {
    WindowID win = xcb_generate_id(dis);
    xcb_void_cookie_t c = xcb_create_window_checked(dis,
            XCB_COPY_FROM_PARENT,          /* depth (same as root)*/
            win,
            parent,
            dims.x, dims.y,
            dims.width, dims.height,
            dims.border,
            clazz,
            screen->root_visual,
            mask, valueList);
    catchError(c);
    return win;
}
int destroyWindow(WindowID win) {
    assert(win);
    LOG(LOG_LEVEL_DEBUG, "Destroying window %d\n", win);
    return catchError(xcb_destroy_window_checked(dis, win));
}
WindowID mapWindow(WindowID id) {
    logger.trace() << "Mapping " << id << std::endl;
    xcb_map_window(dis, id);
    return id;
}
void unmapWindow(WindowID id) {
    logger.trace() << "UnMapping " << id << std::endl;
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
    xcb_intern_atom_cookie_t* cookie;
    bool applyRule = ewmh == NULL;
    ewmh = (xcb_ewmh_connection_t*)malloc(sizeof(xcb_ewmh_connection_t));
    cookie = xcb_ewmh_init_atoms(dis, ewmh);
    xcb_ewmh_init_atoms_replies(ewmh, cookie, NULL);
    _CREATE_ATOM(MAX_DEVICES);
    _CREATE_ATOM(WM_ACTIVE_MASTER);
    _CREATE_ATOM(WM_DELETE_WINDOW);
    _CREATE_ATOM(WM_FAKE_MONITORS);
    _CREATE_ATOM(WM_HINTS);
    _CREATE_ATOM(WM_INTERPROCESS_COM);
    _CREATE_ATOM(WM_MASTER_WINDOWS);
    _CREATE_ATOM(WM_NAME);
    _CREATE_ATOM(WM_STATE_NO_TILE);
    _CREATE_ATOM(WM_STATE_ROOT_FULLSCREEN);
    _CREATE_ATOM(WM_TAKE_FOCUS);
    _CREATE_ATOM(WM_WORKSPACE_LAYOUT_INDEXES);
    _CREATE_ATOM(WM_WORKSPACE_LAYOUT_NAMES);
    _CREATE_ATOM(WM_WORKSPACE_MONITORS);
    _CREATE_ATOM(WM_WORKSPACE_WINDOWS);
    _CREATE_ATOM(WM_WINDOW_ROLE);
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
        LOG(LOG_LEVEL_DEBUG, "closing X connection\n");
        xcb_ewmh_connection_wipe(ewmh);
        free(ewmh);
        ewmh = NULL;
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
            _ADD_EVENT_TYPE_CASE(POSSIBLE_STATE_CHANGE);
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
void dumpAtoms(xcb_atom_t* atoms, int numberOfAtoms) {
    std::cout << "Dumping Atoms: ";
    for(int i = 0; i < numberOfAtoms; i++) {
        if(i)
            std::cout << ", ";
        std::cout << getAtomName(atoms[i]) << " (" << atoms[i] << ")";
    }
    std::cout << "\n";
}
static volatile int idle;
static unsigned int periodCounter;
int getIdleCount() {
    return idle;
}

static xcb_generic_event_t* eventBuffer[2048];
// next index to read from
static uint32_t bufferIndexRead = 0;
// 1 past the last valid index
static uint32_t bufferIndexWrite = 0;
static uint32_t lastDetectedEventSequence = 0;

uint32_t getLastDetectedEventSequenceNumber() {return lastDetectedEventSequence;}

static inline xcb_generic_event_t* getEventFromBuffer() {
    if(bufferIndexWrite == bufferIndexRead) {
        bufferIndexRead = 0;
        bufferIndexWrite = 0;
        return NULL;
    }
    return eventBuffer[bufferIndexRead++];
}
static inline bool addEventToBuffer(xcb_generic_event_t* event) {
    assert(bufferIndexRead == 0);
    assert(bufferIndexWrite < LEN(eventBuffer));
    if(event)
        eventBuffer[bufferIndexWrite++] = event;
    assert(bufferIndexRead <= bufferIndexWrite);
    return event && bufferIndexWrite < LEN(eventBuffer);
}
static inline void enqueueEvents(xcb_generic_event_t* event) {
    while(addEventToBuffer(xcb_poll_for_queued_event(dis)));
    assert(event);
    lastDetectedEventSequence = (bufferIndexWrite ? eventBuffer[bufferIndexWrite - 1] : event)->sequence;
    LOG(LOG_LEVEL_TRACE, "Size of event queue is now %d\n", bufferIndexWrite);
}
static inline xcb_generic_event_t* pollForEvent() {
    xcb_generic_event_t* event;
    for(int i = POLL_COUNT - 1; i >= 0; i--) {
        if(i)
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
    lock();
    if(event)
        enqueueEvents(event);
    unlock();
    return event;
}

static inline xcb_generic_event_t* getNextEvent() {
    if(EVENT_PERIOD && ++periodCounter > EVENT_PERIOD) {
        periodCounter = 0;
        applyEventRules(PERIODIC, NULL);
    }
    static xcb_generic_event_t* event;
    event = getEventFromBuffer();
    if(!event)
        event = pollForEvent();
    if(!event && !xcb_connection_has_error(dis)) {
        periodCounter = 0;
        lock();
        applyEventRules(PERIODIC, NULL);
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
        LOG(LOG_LEVEL_INFO, "Idle %d\n", idle);
        if(!isShuttingDown())
            event = waitForEvent();
    }
    return event;
}

int isSyntheticEvent() {
    xcb_generic_event_t* event = (xcb_generic_event_t*)getLastEvent();
    return event->response_type > 127;
}


void* runEventLoop(void* arg __attribute__((unused))) {
    xcb_generic_event_t* event;
    while(!isShuttingDown() && dis) {
        event = getNextEvent();
        if(isShuttingDown() || xcb_connection_has_error(dis) || !event) {
            do
                free(event);
            while(event = getEventFromBuffer());
            if(isShuttingDown())
                LOG(LOG_LEVEL_INFO, "shutting down\n");
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
        LOG(LOG_LEVEL_TRACE, "event processed\n");
    }
    LOG(LOG_LEVEL_INFO, "Exited event loop\n");
    return NULL;
}
int loadGenericEvent(xcb_ge_generic_event_t* event) {
    LOG(LOG_LEVEL_TRACE, "processing generic event; ext: %d type: %d event type %d seq %d\n",
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
