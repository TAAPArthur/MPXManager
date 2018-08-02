/**
 * @file event-loop.h
 *
 * @brief Handles geting and processing XEvents
 */

#ifndef EVENTS_H_
#define EVENTS_H_
#include "mywm-structs.h"

/**
 * Runs method in a new thread
 * @param method the method to run
 * @param arg the argument to pass into method
 * @return a pthread identifier
 */
pthread_t runInNewThread(void *(*method) (void *),void*arg);
void *runEventLoop(void*arg);
void onGenericEvent();
void requestEventLoopShutdown();
void registerForEvents();
#endif /* EVENTS_H_ */
