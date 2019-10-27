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
xcb_atom_t WM_NAME;
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
static xcb_atom_t MAX_DEVICES;


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
        if((reply = xcb_get_property_reply(dis, cookie, NULL)) && xcb_get_property_value_length(reply)) {
            maxNumDevices = *(int*)xcb_get_property_value(reply);
            free(reply);
        }
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
static int xrandrFirstEvent = -1;

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


static std::unordered_map<WindowMasks, uint32_t> maskToAtom;
static void createMaskAtomMapping() {
    maskToAtom[ABOVE_MASK] = ewmh->_NET_WM_STATE_ABOVE;
    maskToAtom[BELOW_MASK] = ewmh->_NET_WM_STATE_BELOW;
    maskToAtom[FULLSCREEN_MASK] = ewmh->_NET_WM_STATE_FULLSCREEN;
    maskToAtom[HIDDEN_MASK] = ewmh->_NET_WM_STATE_HIDDEN;
    maskToAtom[NO_TILE_MASK] = WM_STATE_NO_TILE;
    maskToAtom[ROOT_FULLSCREEN_MASK] = WM_STATE_ROOT_FULLSCREEN;
    maskToAtom[STICKY_MASK] = ewmh->_NET_WM_STATE_STICKY;
    maskToAtom[URGENT_MASK] = ewmh->_NET_WM_STATE_DEMANDS_ATTENTION;
    maskToAtom[WM_DELETE_WINDOW_MASK] = WM_DELETE_WINDOW;
    maskToAtom[WM_TAKE_FOCUS_MASK] = WM_TAKE_FOCUS;
    maskToAtom[X_MAXIMIZED_MASK] = ewmh->_NET_WM_STATE_MAXIMIZED_HORZ;
    maskToAtom[Y_MAXIMIZED_MASK] = ewmh->_NET_WM_STATE_MAXIMIZED_VERT;
    maskToAtom[URGENT_MASK] = ewmh->_NET_WM_STATE_DEMANDS_ATTENTION;
}
WindowMasks getMaskFromAtom(xcb_atom_t atom) {
    for(auto pair : maskToAtom)
        if(pair.second == atom)
            return pair.first;
    return NO_MASK;
}
int getAtomsFromMask(WindowMask masks, xcb_atom_t* arr) {
    int count = 0;
    for(auto pair : maskToAtom)
        if(pair.first & masks)
            arr[count++] = pair.second;
    return count;
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
#ifndef NO_XRANDR
    xrandrFirstEvent = xcb_get_extension_data(dis, &xcb_randr_id)->first_event;
#endif
    bool applyRule = ewmh == NULL;
    ewmh = (xcb_ewmh_connection_t*)malloc(sizeof(xcb_ewmh_connection_t));
    cookie = xcb_ewmh_init_atoms(dis, ewmh);
    xcb_ewmh_init_atoms_replies(ewmh, cookie, NULL);
    _CREATE_ATOM(WM_TAKE_FOCUS);
    _CREATE_ATOM(WM_DELETE_WINDOW);
    _CREATE_ATOM(WM_NAME);
    _CREATE_ATOM(WM_STATE_NO_TILE);
    _CREATE_ATOM(WM_STATE_ROOT_FULLSCREEN);
    _CREATE_ATOM(WM_INTERPROCESS_COM);
    _CREATE_ATOM(WM_WORKSPACE_LAYOUT_NAMES);
    _CREATE_ATOM(WM_WORKSPACE_MONITORS);
    _CREATE_ATOM(WM_WORKSPACE_LAYOUT_INDEXES);
    _CREATE_ATOM(WM_WORKSPACE_WINDOWS);
    _CREATE_ATOM(WM_MASTER_WINDOWS);
    _CREATE_ATOM(WM_ACTIVE_MASTER);
    _CREATE_ATOM(MAX_DEVICES);
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
        applyEventRules(onXConnection, NULL);
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
            _ADD_EVENT_TYPE_CASE(ExtraEvent);
            _ADD_EVENT_TYPE_CASE(onXConnection);
            _ADD_EVENT_TYPE_CASE(ClientMapAllow);
            _ADD_EVENT_TYPE_CASE(PreRegisterWindow);
            _ADD_EVENT_TYPE_CASE(PostRegisterWindow);
            _ADD_EVENT_TYPE_CASE(UnregisteringWindow);
            _ADD_EVENT_TYPE_CASE(MonitorWorkspaceChange);
            _ADD_EVENT_TYPE_CASE(WindowWorkspaceMove);
            _ADD_EVENT_TYPE_CASE(onScreenChange);
            _ADD_EVENT_TYPE_CASE(ProcessDeviceEvent);
            _ADD_EVENT_TYPE_CASE(onWindowMove);
            _ADD_EVENT_TYPE_CASE(TileWorkspace);
            _ADD_EVENT_TYPE_CASE(Periodic);
            _ADD_EVENT_TYPE_CASE(Idle);
            _ADD_EVENT_TYPE_CASE(TrueIdle);
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
    LOG(LOG_LEVEL_DEBUG, "Queued event %p %d %d\n", event, bufferIndexRead, bufferIndexWrite);
    assert(bufferIndexRead == 0);
    assert(bufferIndexWrite < LEN(eventBuffer));
    if(event)
        eventBuffer[bufferIndexWrite++] = event;
    assert(bufferIndexRead <= bufferIndexWrite);
    return event && bufferIndexWrite < LEN(eventBuffer);
}
static inline void enqueueEvents(xcb_generic_event_t* event) {
    while(addEventToBuffer(xcb_poll_for_queued_event(dis)));
    if(event)
        lastDetectedEventSequence = (bufferIndexWrite ? eventBuffer[bufferIndexWrite - 1] : event)->sequence;
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
    if(event)
        enqueueEvents(event);
    return event;
}

static inline xcb_generic_event_t* getNextEvent() {
    if(EVENT_PERIOD && ++periodCounter > EVENT_PERIOD) {
        periodCounter = 0;
        applyEventRules(Periodic, NULL);
    }
    static xcb_generic_event_t* event;
    event = getEventFromBuffer();
    if(!event)
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
        LOG(LOG_LEVEL_DEBUG, "Idle %d\n", idle);
        if(!isShuttingDown())
            event =  waitForEvent();
        LOG(LOG_LEVEL_DEBUG, "No longer idle; Size of queue %d \n", bufferIndexWrite);
    }
    return event;
}

int isSyntheticEvent() {
    xcb_generic_event_t* event = (xcb_generic_event_t*)getLastEvent();
    return event->response_type > 127;
}


void* runEventLoop(void* arg __attribute__((unused))) {
    xcb_generic_event_t* event;
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
