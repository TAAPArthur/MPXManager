
#include <assert.h>
#include <pthread.h>

#include <xcb/randr.h>

#include "logger.h"
#include "bindings.h"
#include "mywm-util.h"
#include "defaults.h"
#include "devices.h"
#include "xsession.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOCK pthread_mutex_lock(&mutex);
#define UNLOCK pthread_mutex_unlock(&mutex);


pthread_t runInNewThread(void *(*method) (void *),void*arg){
    pthread_t thread;
    assert(pthread_create(&thread, NULL, method, arg)==0);
    return thread;
}
void *runEventLoop(void*c __attribute__((unused))){
    LOG(LOG_LEVEL_TRACE,"starting event loop\n");
    xcb_generic_event_t *event;
    while(dis) {
        LOG(LOG_LEVEL_TRACE,"Waiting for event\n");

        xcb_flush(dis);
        event = xcb_wait_for_event (dis);
        LOG(LOG_LEVEL_TRACE,"got event for event\n");
        if(xcb_connection_has_error(dis)){
            LOG(LOG_LEVEL_WARN,"X connection has been lost\n");
            break;
        }

        int type=event->response_type;
        if(!IGNORE_SEND_EVENT)
            type &= 127;
        assert(type<NUMBER_OF_EVENT_RULES);
        LOG(LOG_LEVEL_DEBUG,"Event detected %d %s %d\n",
                        event->response_type,eventTypeToString(type),getSize(eventRules[type]));
        LOCK
        setLastEvent(event);
        if(type>35)type=35;
        applyEventRules(eventRules[type],NULL);
        UNLOCK
        free(event);
        LOG(LOG_LEVEL_TRACE,"event proccesed\n");
    }
    LOG(LOG_LEVEL_DEBUG,"Exited event loop\n");
    return NULL;
}

void registerForEvents(){
    xcb_void_cookie_t cookie;
    cookie=xcb_change_window_attributes_checked(dis, root,XCB_CW_EVENT_MASK, &ROOT_EVENT_MASKS);
    checkError(cookie, root, "could not sent window masks");
    registerForDeviceEvents();
}

void onGenericEvent(){
    //    xcb_randr_select_input(context->dis, context->root, XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);

    xcb_ge_generic_event_t*event=getLastEvent();
    LOG(LOG_LEVEL_TRACE,"%d %d %d",
            xcb_get_extension_data(dis, &xcb_randr_id)->major_opcode,
            xcb_get_extension_data(dis, &xcb_randr_id)->first_event,
            xcb_get_extension_data(dis, &xcb_randr_id)->first_error);
    LOG(LOG_LEVEL_TRACE,"%d %d %d",
            xcb_get_extension_data(dis, &xcb_input_id)->major_opcode,
            xcb_get_extension_data(dis, &xcb_input_id)->first_event,
            xcb_get_extension_data(dis, &xcb_input_id)->first_error);
    LOG(LOG_LEVEL_TRACE,"processing generic event %d \n",event->extension);

    if(event->extension == xcb_get_extension_data(dis, &xcb_input_id)->major_opcode){
        LOG(LOG_LEVEL_TRACE,"generic event detected %d %s\n",event->event_type,genericEventTypeToString(event->event_type));
        LOG(LOG_LEVEL_TRACE,"generic event detected %d %d\n",event->response_type,event->event_type);
        applyEventRules(eventRules[event->event_type+GENERIC_EVENT_OFFSET], NULL);
    }
    else //if(event->extension == xcb_get_extension_data(dis, &xcb_randr_id)->major_opcode)
    {
        applyEventRules(eventRules[1+MONITOR_EVENT_OFFSET], NULL);
    }
    //else LOG(LOG_LEVEL_ERROR,"could not get event data\n");

}

