/*
 * event-loop.c
 *
 *  Created on: Jul 6, 2018
 *      Author: arthur
 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include <xcb/randr.h>

#include "mywm-structs.h"
#include "bindings.h"
#include "wmfunctions.h"
#include "logger.h"
#include "mywm-util.h"
#include "wmfunctions.h"
#include "defaults.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOCK pthread_mutex_lock(&mutex);
#define UNLOCK pthread_mutex_unlock(&mutex);
/*
void *runXcbEventLoop(void*arg){
    getActiveMaster();
    /*
    xcb_randr_select_input(context->dis, context->root, XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
    xcb_generic_event_t* evt;
  //xcb_randr_screen_change_notify_event_t* randr_evt;

    while ((evt = xcb_wait_for_event(context->dis)) != NULL) {
        LOCK
            printf("xcb event detected: type%d",evt->response_type);
            if (evt->response_type & XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE) {
                detectMonitors(context);
                //randr_evt = (xcb_randr_screen_change_notify_event_t*) evt;

            }
            free(evt);
        UNLOCK
    }*
    return NULL;

}*/
void *runOtherLoop(void*c){
    return NULL;
}
void *runEventLoop(void*c){
    //pthread_t xcbThread;
    pthread_t scriptingThread;

    pthread_create(&scriptingThread, NULL, runOtherLoop, NULL);
    LOG(LOG_LEVEL_TRACE,"starting event loop\n");


    xcb_generic_event_t *event;
    while(!isShuttingDown() && dis) {
        LOG(LOG_LEVEL_TRACE,"Waiting for event\n");
        event = xcb_wait_for_event (dis);
        char type=event->response_type;
        if(!IGNORE_SEND_EVENT)
            type &= 127;
        LOG(LOG_LEVEL_TRACE,"got event for event\n");

        if(!dis || !event){
            LOG(LOG_LEVEL_DEBUG,"X connection has been lost\n");
            dis=NULL;
            break;
        }
        assert(type<NUMBER_OF_EVENT_RULES);
        LOG(LOG_LEVEL_TRACE,"refular event detected %d (%d)%s\n",
                                event->response_type,type,eventTypeToString(type));
        LOG(LOG_LEVEL_TRACE,"refular event detected %d %s %d\n",
                        event->response_type,eventTypeToString(type),getSize(eventRules[type]));
        LOCK
        setLastEvent(event);
        applyEventRules(eventRules[type],NULL);
        LOG(LOG_LEVEL_TRACE,"Flusing %d\n",getSize(getAllWindows()));
         if(dis && isDirty()){
            tileWindows();
            xcb_flush(dis);
         }
        UNLOCK
        LOG(LOG_LEVEL_TRACE,"event proccesed\n");
    }
    LOG(LOG_LEVEL_WARN,"Exited event loop\n");

    return NULL;
}

