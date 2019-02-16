/**
 * @file globals.h
 */
#ifndef GLOBALS_H_
#define GLOBALS_H_

/// \cond
#include <X11/extensions/XInput2.h>
#include <xcb/xinput.h>
/// \endcond


///Returns the field descriptor used to commuicate WM status to an external program
#define STATUS_FD statusPipeFD[1]

///The modifier that will match any other modifier
#ifndef WILDCARD_MODIFIER
    #define WILDCARD_MODIFIER AnyModifier
#endif


///Returns the field descriptors used to commuicate WM status to an external program
extern int statusPipeFD[2];

///If true, then check used saved window properties as defaults
extern char LOAD_SAVED_STATE;
/// How long to wait for a window to die after sening a WM_DELETE_REQUEST
extern long KILL_TIMEOUT;
/**Mask of all events we listen for on the root window*/
extern int ROOT_EVENT_MASKS;
/**Mask of all events we listen for on non-root windows*/
extern int NON_ROOT_EVENT_MASKS;
/**Mask of all events we listen for on relateding to Master devices
 * and the root window.
 */
extern int ROOT_DEVICE_EVENT_MASKS;
/**Mask of all events we listen for on relateding to Master devices
 * and non-root window.
 */
extern int NON_ROOT_DEVICE_EVENT_MASKS;

/// All masks refering to keyboard events
extern int KEYBOARD_MASKS;
/// all masks refering to pointer/mouse events
extern int POINTER_MASKS;

/// The default SHELL; This defaults to the SHELL environment var
extern char* SHELL;

/// How often we update the display on cloned windows (in ms)
extern int CLONE_REFRESH_RATE;
///Defaults masks to be applied to windows
extern int DEFAULT_WINDOW_MASKS;
///If unspecifed the mask of a Binding
extern int DEFAULT_BINDING_MASKS;
///If unspecifed the mask, used during a chaing binding grab
#define DEFAULT_CHAIN_BINDING_GRAB_MASK XCB_INPUT_XI_EVENT_MASK_KEY_PRESS

///The started workspace if !LOAD_STATE or the root CURRENT_DESKTOP property is not set
extern int DEFAULT_WORKSPACE_INDEX;

///Masks to ignore; Default is ModMask2 (NUM_LOCK)
extern int IGNORE_MASK;
///IF True we will crash if an error is received
/// This should only be used for testing as errors can happen just by
/// the async nature of X11
extern int CRASH_ON_ERRORS;

///Number of workspaces to create
extern int NUMBER_OF_WORKSPACES;


///If true ignore all events with the send_event bit
extern char IGNORE_SEND_EVENT;
/// If true ignor all device events with the key repeat flag set
extern char IGNORE_KEY_REPEAT;


/// The last supported standard x event
#define GENERIC_EVENT_OFFSET LASTEvent
/// offset for all monitor related events
#define MONITOR_EVENT_OFFSET GENERIC_EVENT_OFFSET+XCB_INPUT_XI_SELECT_EVENTS+1
/// max value of supported X events (not total events)
#define LAST_REAL_EVENT   MONITOR_EVENT_OFFSET+8

//TODO consistent naming
enum{
    ///if all rules are passed through, then the window is added as a normal window
    onXConnection=LAST_REAL_EVENT,
    /// deterime if a newly detected window should be recorded/monitored/controlled by us
    ProcessingWindow,
    /// called after the newly created window has been added to a workspace
    RegisteringWindow,
    TileWorkspace,
    /// called when the connection is idle
    Idle,
    /// max value of supported events
    NUMBER_OF_EVENT_RULES
};

/// Used for select xcb functions
extern int defaultScreenNumber;

/**Border width when borders for windows*/
extern int DEFAULT_BORDER_WIDTH;
/**Border color for unfocused windows and windows whose last focused master is not set*/
extern int DEFAULT_BORDER_COLOR;

/**Border color for unfocused windows and windows whose last focused master is not set*/
extern int DEFAULT_UNFOCUS_BORDER_COLOR;
/**Default Border color for focused windows*/
extern int DEFAULT_FOCUS_BORDER_COLOR;

/**
 * Indicates how long we should poll for events before switching to blocking
 * We will wait POLL_INTERVAL ms OLL_COUNT times (for aa total of POLL_COUNT*POLL_INTERVAL ms)
 * before deciding that a event connection is idle
 */
extern int POLL_COUNT;
/// @copydoc POLL_COUNT
extern int POLL_INTERVAL;

///  File path of config file dictating ideal master(s)/slaves configuration
extern char * MASTER_INFO_PATH;

/// the X pointer that exists by default
extern int DEFAULT_POINTER;
/// the X keboard that exists by default
extern int DEFAULT_KEYBOARD;
/**
 *Set the last event received.
 * @param event the last event received
 * @see getLastEvent
 */
void setLastEvent(void* event);
/**
 * Retries the last event received
 * @see getLastEvent
 */
void* getLastEvent(void);

/**
 * User can set preStartUpMethod to run an arbitrary function before the startup method is called
 */
extern void (*preStartUpMethod)(void);
/**
 * User can set preStartUpMethod to run an arbitrary function right after the startup method is called
 */
extern void (*startUpMethod)(void);

/**
 * The user can set this method which would be called to write out the WM status to an external program
 */
extern void (*printStatusMethod)(void);
#endif
