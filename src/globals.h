/**
 * @file globals.h
 * Provides global variable declarations
 */
#ifndef GLOBALS_H_
#define GLOBALS_H_

#include <stdbool.h>
#include <X11/X.h>

/// Sets of masks that only a WM should have
#define WM_MASKS (XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_STRUCTURE_NOTIFY)

///Returns the field descriptor used to communicate WM status to an external program
#define STATUS_FD statusPipeFD[1]
///Returns the field descriptor used to communicate WM status to an external program
#define STATUS_FD_READ statusPipeFD[2]

///The modifier that will match any other modifier
#ifndef WILDCARD_MODIFIER
    #define WILDCARD_MODIFIER AnyModifier
#endif

/**
 * Offset for SRC_INDICATION_* masks
 * These masks specify which type of external sources are allowed to modify the window
 * Adding [0,2] and raising 2 to that power yields OTHER,APP and PAGER SRC_INDICATION_ masks respectively
 */
#define SRC_INDICATION_OFFSET 12
/**
 * Various flags that detail how a window should be treated.
 *
 */
typedef enum {
    /// no special properties
    NO_MASK = 0,
    /**The window's X size will equal the size of the monitor's viewport*/
    X_MAXIMIZED_MASK =      1 << 0,
    /**The window's Y size will equal the size of the monitor's viewport*/
    Y_MAXIMIZED_MASK =      1 << 1,
    /// X & Y MAXIMIZED MASK
    MAXIMIZED_MASK =     Y_MAXIMIZED_MASK | X_MAXIMIZED_MASK,
    /**The window's size will equal the size of the monitor (not viewport)     */
    FULLSCREEN_MASK =       1 << 2,
    /**The window's size will equal the size of the screen (union of all monitors)*/
    ROOT_FULLSCREEN_MASK =  1 << 3,

    ///windows will be below normal windows unless ABOVE_MASK is set is set
    BELOW_MASK =            1 << 4,
    ///windows will be above other windows
    ABOVE_MASK =            1 << 5,
    ///Window will not be tiled
    NO_TILE_MASK =          1 << 6,

    /// or of all masks that will cause the layout function to skip a window
    ALL_NO_TILE_MASKS = FULLSCREEN_MASK | ROOT_FULLSCREEN_MASK | BELOW_MASK | ABOVE_MASK | NO_TILE_MASK,

    /**The window's size will equal the size of the monitor (not viewport)     */
    TRUE_FULLSCREEN_MASK =       FULLSCREEN_MASK | ABOVE_MASK,
    /**The window's size will equal the size of the screen (union of all monitors)*/
    TRUE_ROOT_FULLSCREEN_MASK =  ROOT_FULLSCREEN_MASK | ABOVE_MASK,

    ///The window will be treated as unmapped until this mask is removed
    HIDDEN_MASK =        1 << 7,

    USER_MASKS = ((1 << 8) - 1),

    /// Window can be externally resized (configure requests are allowed)
    EXTERNAL_RESIZE_MASK =  1 << 8,
    /// Window can be externally moved (configure requests are allowed)
    EXTERNAL_MOVE_MASK =    1 << 9,
    /// Window border size can be externally changed
    EXTERNAL_BORDER_MASK =    1 << 10,
    /// Window cannot be externally raised/lowered (configure requests are blocked)
    EXTERNAL_RAISE_MASK =   1 << 11,
    /// Window won't be tiled and can be freely moved like by external programs/mouse
    EXTERNAL_CONFIGURABLE_MASK =            EXTERNAL_RESIZE_MASK | EXTERNAL_MOVE_MASK | EXTERNAL_BORDER_MASK | EXTERNAL_RAISE_MASK,
    /// Window is floating; Not tiled, above other windows and can be freely moved like by external programs/mouse and is above other windows
    FLOATING_MASK =            NO_TILE_MASK | ABOVE_MASK | EXTERNAL_CONFIGURABLE_MASK,

    ///Allow client requests from older clients who don't specify what they are
    SRC_INDICATION_OTHER =   1 << SRC_INDICATION_OFFSET,
    ///Allow client requests from normal applications
    SRC_INDICATION_APP =     1 << (SRC_INDICATION_OFFSET + 1),
    ///Allow client requests from pagers
    SRC_INDICATION_PAGER =   1 << (SRC_INDICATION_OFFSET + 2),
    /// allow from any source
    SRC_ANY =                SRC_INDICATION_OTHER | SRC_INDICATION_APP | SRC_INDICATION_PAGER,

    IGNORE_WORKSPACE_MASKS_MASK = 1 << 15,

    /**The window is an input only window (more importantly it cannot have a border)*/
    INPUT_ONLY_MASK =           1 << 16,
    /**The window can receive input focus*/
    INPUT_MASK =           1 << 17,
    /// Will not update our internal focus representation when a window with this mask is focused
    NO_RECORD_FOCUS =      1 << 18,
    /// Windows with this mask will not be activatable
    NO_ACTIVATE_MASK =      1 << 19,

    /**The WM will not forcibly set focus but request the application focus itself*/
    WM_TAKE_FOCUS_MASK =    1 << 20,

    /**The WM will not forcibly delete windows immediately but request the application dies*/
    WM_DELETE_WINDOW_MASK = 1 << 21,
    /**Used in conjunction with WM_DELETE_WINDOW_MASK to kill the window */
    WM_PING_MASK =          1 << 22,
    /// the window type was not set explicitly
    IMPLICIT_TYPE =         1 << 23,

    /**
     * Best effort will be made to place all windows with this mask above any other window without it.
     * One particular flaw the implementation is that if a window with this mask is lowered, it will not automatically be re-raised
     * This mask is implemented via a onWindowMove Rule
     */
    ALWAYS_ON_TOP = 1 << 24,
    /**
     * Best effort will be made to place all windows with this mask below any other window without it.
     * One particular flaw the implementation is that if a window with this mask is raised, it will not automatically be re-lowered
     * This mask is implemented via a onWindowMove Rule
     */
    ALWAYS_ON_BOTTOM = 1 << 25,

    ///Window is effectively associated with its monitor instead of its workspace
    /// (it is move dded between workspaces to stay on its monitor)
    STICKY_MASK =   1 << 26,


    /**Marks the window as urgent*/
    URGENT_MASK =           1 << 27,


    ///Keeps track on the visibility state of the window
    PARTIALLY_VISIBLE =     1 << 28,
    ///Keeps track on the visibility state of the window
    FULLY_VISIBLE =         1 << 29 | PARTIALLY_VISIBLE,
    ///Indicates the window is not withdrawn
    MAPPABLE_MASK =           1 << 30,
    ///the window is currently mapped
    MAPPED_MASK =           1 << 31,

    RETILE_MASKS =           USER_MASKS | MAPPED_MASK,
    /// set all masks
    ALL_MASK =              -1

} WindowMasks;


/**
 * If true (default), then this program is functioning as a WM.
 * Else, WM specific operations, like grabbing any of WM_MASKS on the root window or trying to grab the WM_SELECTION will not happen (by default)
 */
extern bool RUN_AS_WM;
/**
 * If the WM_SELECTION is owned by another client, take it anyways
 */
extern bool STEAL_WM_SELECTION;

/**
 * If true, then we won't automatically ignore windows with the override redirect flag set.
 * Even so we cannot properly manage then; Effectively the flags STICKY and FLOATING would be set (we set them by default too)
 */
extern bool MANAGE_OVERRIDE_REDIRECT_WINDOWS;

/// Holds environmental var names used to pass client pointer to children
extern char* CLIENT[];
/// if true, then set special ENV when spawning commands
extern bool SPAWN_ENV;
/// if true, then preload LD_PRELOAD_PATH
extern bool LD_PRELOAD_INJECTION;
/// the path of the lib to preload
extern char* LD_PRELOAD_PATH;

///Returns the field descriptors used to communicate WM status to an external program
extern int statusPipeFD[4];
/// When receiving certain external communications, fork and print to the sender's stdout
extern bool ENABLE_FORK_ON_RECEIVE;

///If true, then check used saved window properties as defaults
extern bool LOAD_SAVED_STATE;
/// How long to wait for a window to die after sending a WM_DELETE_REQUEST
extern long KILL_TIMEOUT;
/**Mask of all events we listen for on the root window*/
extern int ROOT_EVENT_MASKS;
/**Mask of all events we listen for on non-root windows*/
extern int NON_ROOT_EVENT_MASKS;
/**Mask of all events we listen for on relating to Master devices
 * and the root window.
 */
extern int ROOT_DEVICE_EVENT_MASKS;
/**Mask of all events we listen for on relating to Master devices
 * and non-root window.
 */
extern int NON_ROOT_DEVICE_EVENT_MASKS;

/// All masks referring to keyboard events
extern int KEYBOARD_MASKS;
/// all masks referring to pointer/mouse events
extern int POINTER_MASKS;

/// The default SHELL; This defaults to the SHELL environment var
extern char* SHELL;

/// How often we update the display on cloned windows (in ms)
extern int CLONE_REFRESH_RATE;

///Syncs _NET_WM_STATE automatically with masks
extern int SYNC_WINDOW_MASKS;

///Defaults masks to be applied to windows
extern int DEFAULT_WINDOW_MASKS;
///If unspecified the mask of a Binding
extern int DEFAULT_BINDING_MASKS;
///If unspecified the mask, used during a chain binding grab
#define DEFAULT_CHAIN_BINDING_GRAB_MASK XCB_INPUT_XI_EVENT_MASK_KEY_PRESS

///The started workspace if !LOAD_STATE or the root CURRENT_DESKTOP property is not set
extern int DEFAULT_WORKSPACE_INDEX;

/// time (in seconds) between when windows is detected and mapped, that it will be auto focused (local state is updated immediately); set to -1 to disable
extern int AUTO_FOCUS_NEW_WINDOW_TIMEOUT;

///Masks to ignore; Default is Mod2Mask (NUM_LOCK)
extern int IGNORE_MASK;
///Default modifier for default bindings; Default is Mod4Mask (HYPER)
extern int DEFAULT_MOD_MASK ;
///IF True we will crash if an error is received and a<<(error type) |CRASH_ON_ERRORS is nonzero
/// This should only be used for testing as errors can happen just by
/// the async nature of X11
extern unsigned int CRASH_ON_ERRORS;

///Number of workspaces to create
extern int DEFAULT_NUMBER_OF_WORKSPACES;
///Number of extra workspaces that will be implicitly hidden from most functions
extern int DEFAULT_NUMBER_OF_HIDDEN_WORKSPACES;


///If true ignore all events with the send_event bit
extern bool IGNORE_SEND_EVENT;
/// If true ignore all device events with the key repeat flag set
extern bool IGNORE_KEY_REPEAT;

/**
 * If true then when we manually set focus, we will update our state
 * immediately instead of waiting for the async focus notification event.
*/
extern bool SYNC_FOCUS;


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

/// if true we ignore non-top level windows
extern bool IGNORE_SUBWINDOWS;

/**
 * Indicates how long we should poll for events before switching to blocking
 * We will wait POLL_INTERVAL ms OLL_COUNT times (for a total of POLL_COUNT*POLL_INTERVAL ms)
 * before deciding that a event connection is idle
 */
extern int POLL_COUNT;
/// @copydoc POLL_COUNT
extern int POLL_INTERVAL;
/// after this number of events (or when the connection is idle) Period events with will be triggered
extern int EVENT_PERIOD;


/// Enables all bindings
#define ALL_MODES 0
/// Enables this bindings to be triggered in any mode
#define ANY_MODE (-1)
/// Default binding mode all bindings
#define NORMAL_MODE 1<<0
/// Custom binding mode used for layouts
#define LAYOUT_MODE 1<<1
/// Default binding mode all bindings that all bindings will have if unspecified
extern int DEFAULT_MODE;


///  File path of config file dictating ideal master(s)/slaves configuration
extern char* MASTER_INFO_PATH;

/// the X pointer that exists by default
extern int DEFAULT_POINTER;
/// the X keyboard that exists by default
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
 * User can set startUpMethod to run an arbitrary function when starting up
 */
extern void (*startUpMethod)(void);
/**
 * User can set postStartUpMethod to run an arbitrary function after starting up (right after the xsession has been initialized)
 */
extern void (*postStartUpMethod)(void);

/**
 * The user can set this method which would be called to write out the WM status to an external program
 */
extern void (*printStatusMethod)(void);
#endif
