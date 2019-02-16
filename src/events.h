/**
 * @file events.h
 *
 * @brief Handles geting and processing XEvents
 */

#ifndef EVENTS_H_
#define EVENTS_H_

#include <pthread.h>
#include <xcb/xcb.h>

/**
 * Locks/unlocks the global mutex
 *
 * It is not safe to modify most structs from multiple threads so the main event loop lock/unlocks a
 * global mutex. Any addition thread that runs alongside the main thread of if in general, there 
 * is a race, lock/unlock should be used
 */
void lock(void);
///cpoydoc lock(void)
void unlock(void);

/**
 * Requests all threads to terminate
 */
void requestShutdown(void);
/**
 * Indicate to threads that the system is shutting down;
 */
int isShuttingDown(void);

/**
 *Returns a monotonically increasing counter indicating the number of times the event loop has been idel. Being idle means event loop has nothing to do at the moment which means it has responded to all prior events
*/
int getIdleCount(void);
/**
 * Runs method in a new thread
 * @param method the method to run
 * @param arg the argument to pass into method
 * @param detached creates a detached thread; When a detached thread terminates, its resources are automatically released back to the system without the need for another thread to join with the terminated thread.
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

/**
 * To be called when a generic event is received
 * loads info related to the generic event which can be accesed by getLastEvent()
 */
int loadGenericEvent(xcb_ge_generic_event_t*event);

/**
 * Grab all specified keys/buttons and listen for select device events on events
 */
void registerForDeviceEvents();
/**
 * Request to be notifed when info related to the monitor/screen changes
 */
void registerForMonitorChange();

/**
 * @return 1 iff the last event was synthetic (not sent by the XServer)
 */
int isSyntheticEvent();
#endif /* EVENTS_H_ */
