/**
 * @file events.c
 * @copybrief events.h
 */
#include <assert.h>
#include <pthread.h>

#include <xcb/randr.h>

#include "bindings.h"
#include "devices.h"
#include "globals.h"
#include "logger.h"
#include "mywm-util.h"
#include "state.h"
#include "wmfunctions.h"
#include "xsession.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void lock(void){
    pthread_mutex_lock(&mutex);
}
void unlock(void){
    pthread_mutex_unlock(&mutex);
}

static volatile int shuttingDown = 0;
void requestShutdown(void){
    shuttingDown = 1;
}
int isShuttingDown(void){
    return shuttingDown;
}
pthread_t runInNewThread(void* (*method)(void*), void* arg, int detached){
    pthread_t thread;
    int result __attribute__((unused)) = pthread_create(&thread, NULL, method, arg) == 0;
    assert(result);
    if(detached)
        pthread_detach(thread);
    return thread;
}
static volatile int idle;
static int periodCounter;
int getIdleCount(){
    return idle;
}


static inline xcb_generic_event_t* getNextEvent(){
    if(++periodCounter > EVENT_PERIOD){
        periodCounter = 0;
        applyRules(getEventRules(Periodic), NULL);
    }
    static xcb_generic_event_t* event;
    event = xcb_poll_for_event(dis);
    if(!event && !xcb_connection_has_error(dis)){
        for(int i = 0; i < POLL_COUNT; i++){
            msleep(POLL_INTERVAL);
            event = xcb_poll_for_event(dis);
            if(event)return event;
        }
        periodCounter = 0;
        lock();
        applyRules(getEventRules(Periodic), NULL);
        applyRules(getEventRules(Idle), NULL);
        idle++;
        flush();
        unlock();
        LOG(LOG_LEVEL_VERBOSE, "Idle %d\n", idle);
        event = xcb_wait_for_event(dis);
    }
    return event;
}

int isSyntheticEvent(){
    xcb_generic_event_t* event = getLastEvent();
    return event->response_type > 127;
}

void* runEventLoop(void* arg __attribute__((unused))){
    xcb_generic_event_t* event;
    int xrandrFirstEvent = xcb_get_extension_data(dis, &xcb_randr_id)->first_event;
    LOG(LOG_LEVEL_TRACE, "starting event loop; xrandr event %d\n", xrandrFirstEvent);
    while(!isShuttingDown() && dis){
        event = getNextEvent();
        if(isShuttingDown() || xcb_connection_has_error(dis) || !event){
            if(event)free(event);
            if(isShuttingDown())
                LOG(LOG_LEVEL_INFO, "shutting down\n");
            break;
        }
        int type = event->response_type;
        if(!IGNORE_SEND_EVENT)
            type &= 127;
        if(type < 127){
            lock();
            setLastEvent(event);
            if(type == xrandrFirstEvent){
                applyRules(getEventRules(onScreenChange), NULL);
            }
            if(type >= LASTEvent){
                LOG(LOG_LEVEL_WARN, "unknown event %d\n", event->response_type);
                applyRules(getEventRules(ExtraEvent), NULL);
            }
            else {
                LOG(LOG_LEVEL_VERBOSE, "Event detected %d %s number of rules: %d\n",
                    event->response_type, eventTypeToString(type), getSize(getEventRules(type)));
                assert(type < LASTEvent);
                applyRules(getEventRules(type), NULL);
            }
            unlock();
        }
        free(event);
#ifdef DEBUG
        XSync(dpy, 0);
#endif
        LOG(LOG_LEVEL_VERBOSE, "event proccesed\n");
    }
    LOG(LOG_LEVEL_DEBUG, "Exited event loop\n");
    return NULL;
}
int loadGenericEvent(xcb_ge_generic_event_t* event){
    LOG(LOG_LEVEL_VERBOSE, "processing generic event; ext: %d type: %d event type %d  seq %d\n",
        event->extension, event->response_type, event->event_type, event->sequence);
    if(event->extension == xcb_get_extension_data(dis, &xcb_input_id)->major_opcode){
        LOG(LOG_LEVEL_VERBOSE, "generic event detected %d %d %s number of rules: %d\n",
            event->event_type, event->response_type,
            genericEventTypeToString(event->event_type),
            getSize(getEventRules(event->event_type + GENERIC_EVENT_OFFSET)));
        return event->event_type + GENERIC_EVENT_OFFSET;
    }
    return 0;
}

void registerForMonitorChange(){
    catchError(xcb_randr_select_input_checked(dis, root, XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE));
}
void registerForDeviceEvents(){
    ArrayList* list = getDeviceBindings();
    LOG(LOG_LEVEL_DEBUG, "Grabbing %d buttons/keys\n", getSize(list));
    FOR_EACH(Binding*, binding, list){
        initBinding(binding);
        grabBinding(binding);
    }
    LOG(LOG_LEVEL_DEBUG, "listening for device event;  masks: %d\n", ROOT_DEVICE_EVENT_MASKS);
    passiveGrab(root, ROOT_DEVICE_EVENT_MASKS);
}
int registerForWindowEvents(WindowID window, int mask){
    xcb_void_cookie_t cookie;
    cookie = xcb_change_window_attributes_checked(dis, window, XCB_CW_EVENT_MASK, &mask);
    return catchErrorSilent(cookie);
}
void registerForEvents(){
    registerForWindowEvents(root, ROOT_EVENT_MASKS);
    registerForDeviceEvents();
    registerForMonitorChange();
}
