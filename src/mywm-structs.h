/**
 * @file mywm-structs.h
 * @brief contains global struct definitions
 */
#include <stdbool.h>

#include "util.h"

#ifndef MYWM_STRUCTS_H
#define MYWM_STRUCTS_H

/// Fixed size of all struct fields the keep an array of arbitrary sized names
#ifndef NAME_BUFFER
    #define NAME_BUFFER 32
#endif

/// typeof WindowInfo::mask
typedef unsigned int WindowMask;
/// typeof WindowInfo::id
typedef unsigned int WindowID;
/// typeof Master::id
typedef unsigned int MasterID;
/// typeof Workspace::id
typedef unsigned int WorkspaceID;
/// typeof Monitor::id
typedef unsigned int MonitorID;


///holds data on a window
typedef struct {
    /**Window id */
    WindowID id;
    /**Window id used to determine changes in window layouts */
    WindowID effectiveId;
    /**Window mask */
    WindowMask mask;
    /// set to 1 iff the window is a dock
    bool dock;
    /// 1 iff override_redirect flag set
    bool overrideRedirect;
    /**xcb_atom representing the window type*/
    unsigned int type;
    /**string xcb_atom representing the window type*/
    char* typeName;
    /**class name of window*/
    char* className;
    /** instance name of window*/
    char* instanceName;
    /**title of window*/
    char* title;

    /**
     * Used to indicate the "last seen in" workspace.
     * A window should only be tiled if the this field matches
     * the tiling workspace's index
     *
     * Note that his field may not contain all the workspaces an window belongs to
     */
    WorkspaceID workspaceIndex;

    /// used to tell if a window has attempted to be mapped before
    bool mappedBefore;
    ///Time window was first detected
    unsigned int creationTime;
    /**The time this window was minimized*/
    unsigned int minimizedTimeStamp;
    ///time the window was last pinged
    unsigned int pingTimeStamp;

    ///counter for how many clients have requested the this window to be locked
    /// when this field is zero, the geometry field is allowed to be updated in response to XEvents
    char geometrySemaphore;
    ///Window gravity
    char gravity;
    /// 5 bits each to enable 1 element of config
    unsigned char tilingOverrideEnabled;
    /** Set to override tiling */
    short tilingOverride[5];
    /** The last know size of the window */
    short geometry[5];

    /**
     * The window this window is a transient for
     */
    WindowID transientFor;
    /**
     * Window id of related window
     */
    WindowID groupId;
    /// the parent of this window
    WindowID parent;
    /**
     * Indicator of the masks an event has. This will be bitwise added
     * to the NON_ROOT_DEFAULT_WINDOW_MASKS when processing a new window
     */
    int eventMasks;

    /**The dock is only applied to the primary monitor*/
    bool onlyOnPrimary;
    /**
     * Special properties the window may have
     * Like the reserved space on a monitor
     * */
    int properties[12];
} WindowInfo;

struct Binding;
///holds data on a master device pair like the ids and focus history
typedef struct {
    /**keyboard master id;*/
    MasterID id;
    /**pointer master id associated with id;*/
    MasterID pointerId;
    /// the name of the master keyboard
    char name[NAME_BUFFER];
    /// the color windows when this device has the most recent focus
    unsigned int focusColor;

    /**Stack of windows in order of most recently focused*/
    ArrayList windowStack;
    /**Contains the window with current focus,
     * will be same as top of window stack if freezeFocusStack==0
     * */
    int focusedWindowIndex;

    /**Time the focused window changed*/
    unsigned int focusedTimeStamp;
    /// The current binding mode
    int bindingMode;
    /**Pointer to last binding triggered by this master device*/
    struct Binding* lastBindingTriggered;
    /**List of the active chains with the newest first*/
    ArrayList activeChains;
    /**When true, the focus stack won't be updated on focus change*/
    bool freezeFocusStack;

    /** pointer to head of node list where the values point to WindowInfo */
    ArrayList windowsToIgnore;

    /**Index of active workspace; this workspace will be used for
     * all workspace operations if a workspace is not specified
     * */
    WorkspaceID activeWorkspaceIndex;
    /// When set, device event rules will be passed this window instead of the active window or focused
    WindowID targetWindow;

    ///Starting point for operations like resizing the window with mouse
    short prevMousePos[2];
    ///Last recorded mouse position for a device event
    short currentMousePos[2];
    /**
     * stack of visited workspaces with most recent at top
     */
    ArrayList workspaceHistory;

} Master;

typedef struct Monitor Monitor;

typedef struct Layout Layout;

///metadata on Workspace
typedef struct {
    ///workspace index
    WorkspaceID id;
    ///user facing name of the workspace
    char name[NAME_BUFFER];
    ///the monitor the workspace is on
    Monitor* monitor;

    /** if the workspace is mapped
     *
     * A work space should be mapped when it is visible, but assigning a monitor does not does not automatically cause all windows in the workspace to be mapped.
     * This field is here to sync the two states
     *
     */
    bool mapped;
    /**
     * All windows in this workspace are treated as if they had this mask in addition to any mask they may already have
     */
    WindowMask mask;

    ///an windows stack
    ArrayList windows;

    ///the currently applied layout; does not have to be in layouts
    Layout* activeLayout;
    ///a circular list of layouts to cycle through
    ArrayList layouts;
    /// offset into layouts when cycling
    int layoutOffset;
} Workspace;
#endif
