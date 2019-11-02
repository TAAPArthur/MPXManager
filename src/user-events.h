/**
 * @file user-events.h
 * @brief Defines the types of hooks that can be triggered at various points in the code
 */

#ifndef MPX_USER_EVENTS
#define MPX_USER_EVENTS
#include <stdbool.h>
#include <X11/X.h>
#include <X11/extensions/XInput2.h>
#include <xcb/xinput.h>
/// The last supported standard x event
#define GENERIC_EVENT_OFFSET (LASTEvent-1)
/// max value of supported X events (not total events)
#define LAST_REAL_EVENT (GENERIC_EVENT_OFFSET+XI_LASTEVENT+1)

//TODO consistent naming
enum {
    /**
     * X11 events that are >= LASTEvent but not the first XRANDR event.
     * In other words events that are unknown/unaccounted for.
     * The value 1 is safe to use because is reserved for X11 replies
     */
    ExtraEvent = 1,
    /// Called onStartup after the xdisplay has been opened
    onXConnection = LAST_REAL_EVENT,
    /// Triggered when the client allows the window to be mapped (MapRequest)
    /// Windows that circumvent MapRequest and go straight to MapNotify are implicitly allowed to be mapped
    ClientMapAllow,
    /**
     * Called before the newly created window has been added to our internal lists
     * If the result of these rules is 0, then the window won't be added to out list
     */
    PreRegisterWindow,
    /// Called after the newly created window has been added to our internal lists
    PostRegisterWindow,
    /// Called right before a window is remove from our internal lists
    UnregisteringWindow,
    /// called right after a window changes workspaces
    WindowWorkspaceMove,
    /// called right after a monitor changes workspaces
    MonitorWorkspaceChange,
    /// triggered when root screen is changed
    onScreenChange,
    ProcessDeviceEvent,
    /**
     * Called anytime a managed window is configured. The filtering out of ignored windows is one of the main differences between this and XCB_CONFIGURE_NOTIFY. THe other being that the WindowInfo object will be passed in when the rule is applied.
     */
    onWindowMove,
    /// Called when a non-empty workspace is tilled. An arbitrary window of the workspace will be passed in.
    TileWorkspace,
    /// called after a set number of events or when the connection is idle
    Periodic,
    /// called when the connection is idle
    Idle,
    /// called when the connection is idle (even after the calls to Idle. )
    /// These calls should not affect the XState
    TrueIdle,
    /// max value of supported events
    MPX_LAST_EVENT
};
#endif
