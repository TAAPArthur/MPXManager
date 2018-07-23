/*
 * event-loop.c
 *
 *  Created on: Jul 6, 2018
 *      Author: arthur
 */
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

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOCK pthread_mutex_lock(&mutex);
#define UNLOCK pthread_mutex_unlock(&mutex);

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
    }*/
    return NULL;

}
void *runOtherLoop(void*c){
    return NULL;
}
void *runEventLoop(void*c){
    //pthread_t xcbThread;
    //pthread_t scriptingThread;
    //pthread_create(&xcbThread, NULL, runXcbEventLoop, NULL);
    //pthread_create(&scriptingThread, NULL, runOtherLoop, NULL);
    LOG(LOG_LEVEL_TRACE,"starting event loop\n");

    xcb_generic_event_t *event;
    while(!isShuttingDown() && dpy) {
        LOG(LOG_LEVEL_TRACE,"Waiting for event\n");
        event = xcb_wait_for_event (dis);
        while(event){

            if(!dis || !event){
                LOG(LOG_LEVEL_DEBUG,"X connection has been lost\n");
                continue;
            }
            LOG(LOG_LEVEL_TRACE,"refular event detected %d %s %d\n",
                    event->response_type,eventTypeToString(event->response_type),getSize(eventRules[event->response_type]));
            LOCK
            setLastEvent(&event);
            applyEventRules(event->response_type,NULL);
            event=xcb_poll_for_event(dis);
            if(!isShuttingDown())
                break;
        }
         if(isDirty()){
            tileWindows();
            xcb_flush(dis);
         }

        UNLOCK
        LOG(LOG_LEVEL_TRACE,"event proccesed\n");
    }
    LOG(LOG_LEVEL_WARN,"Exited event loop\n");

    return NULL;
}

