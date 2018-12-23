/**
 * @file events.h
 *
 * @brief Handles geting and processing XEvents
 */

#ifndef EVENTS_H_
#define EVENTS_H_

#include <pthread.h>
void lock();
void unlock();

/**
 * Requests all threads to terminate
 */
void requestShutdown();
/**
 * Indicate to threads that the system is shutting down;
 */
int isShuttingDown();

/**
 *Returns a monotonically increasing counter indicating the number of times the event loop has been idel. Being idle means event loop has nothing to do at the moment which means it has responded to all prior events
*/
int getIdleCount();
/**
 * Runs method in a new thread
 * @param method the method to run
 * @param arg the argument to pass into method
 * @return a pthread identifier
 */
pthread_t runInNewThread(void *(*method) (void *),void*arg,int detached);
/**
 * TODO rename
 * Continually listens and responds to event and applying corresponding Rules.
 * This method will only exit when the x connection is lost
 * @param arg unused
 */
void *runEventLoop(void*arg);
/**
 * Attemps to translate the generic event receive into an extension event and applyies corresponding Rules.
 */
void onGenericEvent(void);
/**
 * TODO rename
 * Register ROOT_EVENT_MASKS
 * @see ROOT_EVENT_MASKS
 */
void registerForEvents();

/**
 * Declare interest in select window events
 * @param window
 * @param mask
 */
void registerForWindowEvents(int window,int mask);

int loadGenericEvent(xcb_ge_generic_event_t*event);

/**
 * Grab all specified keys/buttons and listen for select device events on events
 */
void registerForDeviceEvents();
void registerForMonitorChange();

#endif /* EVENTS_H_ */
