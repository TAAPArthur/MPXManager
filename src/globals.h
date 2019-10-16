/**
 * @file globals.h
 * Provides global variable declarations
 */
#ifndef MPX_GLOBALS_H_
#define MPX_GLOBALS_H_

#include <X11/X.h>
#include <X11/extensions/XInput2.h>
#include <xcb/xinput.h>
#include <string>

/// Sets of masks that only a WM should have
#define WM_MASKS (XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_STRUCTURE_NOTIFY)

/// All masks referring to keyboard events
#define KEYBOARD_MASKS (XCB_INPUT_XI_EVENT_MASK_KEY_PRESS | XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE)

/// all masks referring to pointer/mouse events
#define POINTER_MASKS  (XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS | XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE | XCB_INPUT_XI_EVENT_MASK_MOTION)


///The modifier that will match any other modifier
#ifndef WILDCARD_MODIFIER
#define WILDCARD_MODIFIER AnyModifier
#endif


/// the X pointer that exists by default
#ifndef DEFAULT_POINTER
#define DEFAULT_POINTER 2
#endif
/// the X keyboard that exists by default
#ifndef DEFAULT_KEYBOARD
#define DEFAULT_KEYBOARD 3
#endif

#define __CAT(x, y) x ## y
#define _CAT(x, y) __CAT(x, y)

/**
 * Window Manager name used to comply with EWMH
 */
#define WINDOW_MANAGER_NAME "MPX Manager"


/// if true, then preload LD_PRELOAD_PATH
extern bool LD_PRELOAD_INJECTION;
/**
 * If true, then we won't automatically ignore windows with the override redirect flag set.
 * Even so we cannot properly manage then; Effectively the flags STICKY and FLOATING would be set (we set them by default too)
 */

/**
 * If true (default), then this program is functioning as a WM.
 * Else, WM specific operations, like grabbing any of WM_MASKS on the root window or trying to grab the WM_SELECTION will not happen (by default)
 */
extern bool RUN_AS_WM;


/**
 * If the WM_SELECTION is owned by another client, take it anyways
 */
extern bool STEAL_WM_SELECTION;


/// the path of the lib to preload
extern std::string LD_PRELOAD_PATH;

///  File path of config file dictating ideal master(s)/slaves configuration
extern std::string MASTER_INFO_PATH;


/// The default SHELL; This defaults to the SHELL environment var
extern std::string SHELL;

/// time (in seconds) between when windows is detected and mapped, that it will be auto focused (local state is updated immediately); set to -1 to disable
extern uint32_t AUTO_FOCUS_NEW_WINDOW_TIMEOUT;

/// How often we update the display on cloned windows (in ms)
extern uint32_t CLONE_REFRESH_RATE;

///IF True we will crash if an error is received and a<<(error type) |CRASH_ON_ERRORS is nonzero
/// This should only be used for testing as errors can happen just by
/// the async nature of X11
extern uint32_t CRASH_ON_ERRORS;


/**Border color for unfocused windows and windows whose last focused master is not set*/
extern uint32_t DEFAULT_BORDER_COLOR;
/**Border width when borders for windows*/
extern uint32_t DEFAULT_BORDER_WIDTH;
///Default modifier for default bindings; Default is Mod4Mask (HYPER)
extern uint32_t DEFAULT_MOD_MASK;
///Number of workspaces to create
extern uint32_t DEFAULT_NUMBER_OF_WORKSPACES;


/**Border color for unfocused windows and windows whose last focused master is not set*/
extern uint32_t DEFAULT_UNFOCUS_BORDER_COLOR;

/// after this number of events (or when the connection is idle) Period events with will be triggered
extern uint32_t EVENT_PERIOD;


///Masks to ignore; Default is Mod2Mask (NUM_LOCK)
extern uint32_t IGNORE_MASK;
/// How long to wait for a window to die after sending a WM_DELETE_REQUEST
extern uint32_t KILL_TIMEOUT;
/**Mask of all events we listen for on relating to Master devices
 * and non-root window.
 */
extern uint32_t NON_ROOT_DEVICE_EVENT_MASKS;


/**Mask of all events we listen for on non-root windows*/
extern uint32_t NON_ROOT_EVENT_MASKS;
/**
 * Indicates how long we should poll for events before switching to blocking
 * We will wait POLL_INTERVAL ms POLL_COUNT times (for a total of POLL_COUNT*POLL_INTERVAL ms)
 * before deciding that a event connection is idle
 */
extern uint32_t POLL_COUNT;
/// @copydoc POLL_COUNT
extern uint32_t POLL_INTERVAL;
/**Mask of all events we listen for on relating to Master devices
 * and the root window.
 */
extern uint32_t ROOT_DEVICE_EVENT_MASKS;
/**Mask of all events we listen for on the root window*/
extern uint32_t ROOT_EVENT_MASKS;


#endif
