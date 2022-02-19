/**
 * @file user-events.h
 * @brief Defines the types of hooks that can be triggered at various points in the code
 */

#ifndef MPX_USER_EVENTS
#define MPX_USER_EVENTS
#include <X11/X.h>
#include <stdbool.h>
#include <xcb/xinput.h>
/// The last supported standard x event
#define GENERIC_EVENT_OFFSET (LASTEvent-1)
/// max value of supported X events (not total events)
#define LAST_REAL_EVENT (GENERIC_EVENT_OFFSET+XCB_INPUT_BARRIER_LEAVE+1)

typedef enum {
    /**
     * X11 events that are >= LASTEvent
     * In other words events that are unknown/unaccounted for.
     * The value 1 is safe to use because is reserved for X11 replies
     */
    EXTRA_EVENT = 1,
    /// Called on startup after the x11 display has been opened
    X_CONNECTION = LAST_REAL_EVENT,
    /// Called after the newly created window has been added to our internal lists
    POST_REGISTER_WINDOW,
    /// Called right before a window is remove from our internal lists
    UNREGISTER_WINDOW,
    /// Translated from Key/Button press/release and/or motion events
    DEVICE_EVENT,
    /// Triggered when the client allows the window to be mapped (MapRequest)
    /// Windows that circumvent MapRequest and go straight to MapNotify are implicitly allowed to be mapped
    CLIENT_MAP_ALLOW,
    /// Triggered when the client allows the window to be mapped (Synthetic UnmapRequest)
    CLIENT_MAP_DISALLOW,
    /// triggered when root screen is changed (indicated by a XCB_CONFIGURE_NOTIFY to the root window)
    SCREEN_CHANGE,
    /// Called after workspace is tilled.
    TILE_WORKSPACE,
    /// Called when a window leaves a workspace
    WORKSPACE_WINDOW_REMOVE,
    /// Called when a window joins a workspace
    WORKSPACE_WINDOW_ADD,
    /// called right after a monitor changes workspaces
    MONITOR_WORKSPACE_CHANGE,
    /**
     * Called anytime a managed window is configured. The filtering out of ignored windows is one of the main differences between this and XCB_CONFIGURE_NOTIFY. The other being that the WindowInfo object will be passed in when the rule is applied.
     */
    WINDOW_MOVE,
    /// called when the connection is idle
    IDLE,
    /// called when the connection is idle (even after the calls to IDLE. )
    /// These calls should not affect the XState
    TRUE_IDLE,
    /// max value of supported events
    NUMBER_OF_MPX_EVENTS
} UserEvent;
#endif
