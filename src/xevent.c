#include <assert.h>
#include <string.h>
#include "unistd.h"

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <X11/Xlib-xcb.h>

#include "boundfunction.h"
#include "globals.h"
#include "monitors.h"
#include "user-events.h"
#include "util/logger.h"
#include "util/time.h"
#include "xevent.h"
#include "xutil/xsession.h"

static volatile int idle;
int getIdleCount() {
    return idle;
}
static int lastQueuedEventSequenceNumber;
static int lastEventSequenceNumber;
uint32_t getLastDetectedEventSequenceNumber() {return MAX(lastQueuedEventSequenceNumber, lastEventSequenceNumber);}
uint16_t getCurrentSequenceNumber(void) {
    return lastEventSequenceNumber;
}

typedef struct {
    xcb_generic_event_t* arr[MPX_EVENT_QUEUE_SIZE];
    /// next index to read from
    uint16_t bufferIndexRead;
    /// next index to write to
    uint16_t bufferIndexWrite;
} EventQueue;
static EventQueue eventQueue;
int getEventQueueSize() {
    return (eventQueue.bufferIndexWrite - eventQueue.bufferIndexRead + MPX_EVENT_QUEUE_SIZE) % MPX_EVENT_QUEUE_SIZE;
}
bool isEventQueueEmpty() {
    return eventQueue.bufferIndexWrite == eventQueue.bufferIndexRead;
}

static bool pushEvent(xcb_generic_event_t* event) {
    if(!event) {
        TRACE("No more events to read");
        return NULL;
    }
    eventQueue.arr[eventQueue.bufferIndexWrite++ % MPX_EVENT_QUEUE_SIZE] = event;
    lastQueuedEventSequenceNumber = event->sequence;
    return (eventQueue.bufferIndexWrite - eventQueue.bufferIndexRead) % MPX_EVENT_QUEUE_SIZE;
}

static xcb_generic_event_t* popEvent() {
    return eventQueue.arr[eventQueue.bufferIndexRead++ % MPX_EVENT_QUEUE_SIZE];
}


static inline void enqueueEvents(void) {
    TRACE("Reading events on the X queue");
    while(pushEvent(xcb_poll_for_queued_event(dis)));
    TRACE("Finished reading events off of the X queue");
}
static inline xcb_generic_event_t* pollForEvent() {
    xcb_generic_event_t* event;
    TRACE("Polling for event");
    for(int i = POLL_COUNT - 1; i >= 0; i--) {
        msleep(POLL_INTERVAL);
        event = xcb_poll_for_event(dis);
        if(event) {
            enqueueEvents();
            return event;
        }
    }
    return NULL;
}
static inline xcb_generic_event_t* waitForEvent() {
    xcb_generic_event_t* event = xcb_wait_for_event(dis);
    if(event) {
        enqueueEvents();
    }
    return event;
}

static inline xcb_generic_event_t* getNextEvent() {
    xcb_generic_event_t* event = NULL;
    if(!isEventQueueEmpty())
        event = popEvent();
    if(!event)
        event = pollForEvent();
    if(!event && !xcb_connection_has_error(dis)) {
        if(applyEventRules(IDLE, NULL)) {
            flush();
            event = pollForEvent();
            if(event) {
                return event;
            }
        }
        applyEventRules(TRUE_IDLE, NULL);
        idle++;
        setWindowPropertyInt(getPrivateWindow(), MPX_IDLE_PROPERTY, XCB_ATOM_CARDINAL, getIdleCount());
        flush();
        INFO("Idle %d", idle);
        if(!isShuttingDown())
            event = waitForEvent();
    }
    return event;
}



static volatile bool shuttingDown;
void requestShutdown() {
    shuttingDown = 1;
}
bool isShuttingDown(void) {
    return shuttingDown;
}
void runEventLoop() {
    shuttingDown = 0;
    xcb_generic_event_t* event = NULL;
    applyBatchEventRules();
    while(!isShuttingDown() && dis) {
        event = getNextEvent();
        if(isShuttingDown() || xcb_connection_has_error(dis) || !event) {
            if(event)
                free(event);
            break;
        }
        int type = event->response_type & 127;
        // TODO pre event processing rule
        type = type < LASTEvent ? type : EXTRA_EVENT;
        lastEventSequenceNumber = event->sequence;
        applyEventRules(type, event);
        free(event);
#ifdef DEBUG
        XSync(dpy, 0);
        fflush(NULL);
#endif
        TRACE("event processed");
    }
    if(isShuttingDown() || xcb_connection_has_error(dis) || !event) {
        if(isShuttingDown())
            INFO("shutting down");
    }
    INFO("Exited event loop");
}

void onGenericEvent(xcb_ge_generic_event_t* event) {
    int type = loadGenericEvent(event);
    if(type)
        applyEventRules(type, event);
}
void addXIEventSupport() {
    addEvent(XCB_GE_GENERIC, DEFAULT_EVENT(onGenericEvent));
}

int loadGenericEvent(xcb_ge_generic_event_t* event) {
    TRACE("processing generic event; ext: %d type: %d event type %d seq %d",
        event->extension, event->response_type, event->event_type, event->sequence);
    if(event->extension == xcb_get_extension_data(dis, &xcb_input_id)->major_opcode) {
        int type = event->event_type + GENERIC_EVENT_OFFSET;
        return type;
    }
    WARN("Could not process generic event");
    return 0;
}
int isSyntheticEvent(void* event) {
    return ((xcb_generic_event_t*)event)->response_type > 127;
}

void addShutdownOnIdleRule() {
    addEvent(TRUE_IDLE, DEFAULT_EVENT(requestShutdown, LOWEST_PRIORITY));
}
