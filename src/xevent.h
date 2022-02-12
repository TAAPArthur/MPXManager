/**
 * @file xsession.h
 * @brief Create/destroy XConnection and set global X vars
 */

#ifndef MPX_XEVENT_H_
#define MPX_XEVENT_H_
#include <stdbool.h>
#include <xcb/xcb.h>

#define MPX_EVENT_QUEUE_SIZE (1 << 10)


void addExtraEvent(int fd, int mask,  void(*callBack)());

void setIdleProperty();
void addXIEventSupport();

/**
 * Attempts to translate the generic event receive into an extension event and applies corresponding Rules.
 */
void onGenericEvent(xcb_ge_generic_event_t* event);
/**
 * @return 1 iff the last event was synthetic (not sent by the XServer)
 */
int isSyntheticEvent(void* event);
/**
 * Returns a monotonically increasing counter indicating the number of times the event loop has been idle. Being idle means event loop has nothing to do at the moment which means it has responded to all prior events
*/
int getIdleCount(void);
/**
 * @return the sequence number of the last event to be queued or 0
 */
uint32_t getLastDetectedEventSequenceNumber();

/**
 * Continually listens and responds to event and applying corresponding Rules.
 * This method will only exit when the x connection is lost
 */
void runEventLoop();
/**
 * To be called when a generic event is received
 * loads info related to the generic event which can be accessed by getLastEvent()
 */
int loadGenericEvent(xcb_ge_generic_event_t* event);
/**
 * @return the sequence number of the last event that started being processed
 */
uint16_t getCurrentSequenceNumber(void);



/**
 * Requests all threads to terminate
 */
void requestShutdown();
/**
 * Indicate to threads that the system is shutting down;
 */
bool isShuttingDown(void);

void addShutdownOnIdleRule();

int getEventQueueSize();

#endif

