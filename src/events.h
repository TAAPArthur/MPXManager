/**
 * @file events.h
 *
 * @brief Handles geting and processing XEvents
 */

#ifndef EVENTS_H_
#define EVENTS_H_

#include <xcb/xcb.h>

#include <X11/extensions/XInput2.h>
#include <xcb/xinput.h>
#include "mywm-structs.h"

/// The last supported standard x event
#define GENERIC_EVENT_OFFSET (LASTEvent-1)
/// max value of supported X events (not total events)
#define LAST_REAL_EVENT  (GENERIC_EVENT_OFFSET+XI_LASTEVENT+1)

//TODO consistent naming
enum {
    /**
     * X11 events that are >= LASTEvent but not the first XRANDR event.
     * In other words events that are unknown/unaccounted for.
     * The value 1 is safe to use because is reserved for X11 replies
     */
    ExtraEvent = 1,
    ///if all rules are passed through, then the window is added as a normal window
    onXConnection = LAST_REAL_EVENT,
    /// Run after properties have been loaded
    PropertyLoad,
    /// determines if a newly detected window should be recorded/monitored/controlled by us
    ProcessingWindow,
    /// called after the newly created window has been added to a workspace
    RegisteringWindow,
    /// triggered when root screen is changed
    onScreenChange,
    /// when a workspace is tiled
    TileWorkspace,
    /**
     * Called anytime a managed window is configured. The filtering out of ignored windows is one of the main differences between this and XCB_CONFIGURE_NOTIFY. THe other being that the WindowInfo object will be passed in when the rule is applied.
     */
    onWindowMove,
    /// called after a set number of events or when the connection is idle
    Periodic,
    /// called when the connection is idle
    Idle,
    /// max value of supported events
    NUMBER_OF_EVENT_RULES
};



/**
 * Returns a monotonically increasing counter indicating the number of times the event loop has been idle. Being idle means event loop has nothing to do at the moment which means it has responded to all prior events
*/
int getIdleCount(void);
/**
 * @param i index of eventRules
 * @return the specified event list
 */
ArrayList* getEventRules(int i);
/**
 * Removes all rules from the all eventRules
 */
void clearAllRules(void);

/**
 * @param i index of batch event rules
 * @return the specified event list
 */
ArrayList* getBatchEventRules(int i);
/**
 * Increment the counter for a batch rules.
 * All batch rules with a non zero counter will be trigged
 * on the next call to incrementBatchEventRuleCounter()
 *
 * @param i the event type
 */
void incrementBatchEventRuleCounter(int i);
/**
 * For every event rule that has been triggered since the last call
 * incrementBatchEventRuleCounter(), trigger the corresponding batch rules
 *
 */
void applyBatchRules(void);
/**
 * Clear all batch event rules
 */
void clearAllBatchRules(void);

/**
 * TODO rename
 * Continually listens and responds to event and applying corresponding Rules.
 * This method will only exit when the x connection is lost
 * @param arg unused
 */
void* runEventLoop(void* arg);
/**
 * Attempts to translate the generic event receive into an extension event and applies corresponding Rules.
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
 * @return the error code if an error occurred
 */
int registerForWindowEvents(WindowID window, int mask);

/**
 * Apply the list of rules designated by the type
 *
 * @param type
 * @param winInfo
 * @return the result
 */
int applyEventRules(int type, WindowInfo* winInfo);

/**
 * To be called when a generic event is received
 * loads info related to the generic event which can be accessed by getLastEvent()
 */
int loadGenericEvent(xcb_ge_generic_event_t* event);

/**
 * Grab all specified keys/buttons and listen for select device events on events
 */
void registerForDeviceEvents();
/**
 * Request to be notified when info related to the monitor/screen changes
 * This method does nothing if compiled without XRANDR support
 */
void registerForMonitorChange();

/**
 * @return 1 iff the last event was synthetic (not sent by the XServer)
 */
int isSyntheticEvent();
#endif /* EVENTS_H_ */
