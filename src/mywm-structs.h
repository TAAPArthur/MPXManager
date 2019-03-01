#include "util.h"

#ifndef MYWM_STRUCTS_H
#define MYWM_STRUCTS_H

/// Fixed size of all struct fields the keep an array of arbitary sized names
#ifndef NAME_BUFFER
    #define NAME_BUFFER 32
#endif
///holds data on a window
typedef struct {
    /**Window id */
    unsigned int id;
    /**Window mask */
    unsigned int mask;
    /// set to 1 iff the window is a dock
    int dock;
    /**xcb_atom representing the window type*/
    int type;
    /**string xcb_atom representing the window type*/
    char* typeName;
    /**class name of window*/
    char* className;
    /** instance name of window*/
    char* instanceName;
    /**title of window*/
    char* title;

    /**1 iff the window type was not explicilty set*/
    char implicitType;
    /**
     * Used to indicate the "last seen in" workspace.
     * A window should only be tiled if the this field matches
     * the tiling workspace's index
     *
     * Note that his field may not contain all the workspaces an window belongs to
     */
    int workspaceIndex;

    /// used to tell if a window has attempted to be mapped before
    char mappedBefore;
    ///Time window was first detected
    int creationTime;
    /**The time this window was minimized*/
    int minimizedTimeStamp;
    ///time the window was last pinged
    int pingTimeStamp;

    ///counter for how many clients have requested the this window to be locked
    /// when this field is zero, the the geometry field is allowed to be updated in response to XEvents
    char geometrySemaphore;
    ///Window gravity
    char gravity;
    /** Set to override tiling */
    short config[5];
    /** The last know size of the window */
    short geometry[5];

    /**id of src window iff window is a clone*/
    int cloneOrigin;
    /**List of clones*/
    ArrayList cloneList;
    ///x,y offset for cloned windows; they will clone the original starting at this offset
    int cloneOffset[2];
    /**
     * The window this window is a transient for
     */
    unsigned int transientFor;
    /**
     * Window id of related window
     */
    unsigned int groupId;
    /// the parent of this window
    unsigned int parent;
    /**
     * Indicator of the masks an event has. This will be bitwise added
     * to the NON_ROOT_DEFAULT_WINDOW_MASKS when processing a new window
     */
    int eventMasks;

    /**The dock is only applied to the primary monitor*/
    char onlyOnPrimary;
    /**
     * Special properties the window may have
     * Like the reserved space on a monitor
     * */
    int properties[12];
} WindowInfo;

struct binding_struct;
///holds data on a master device pair like the ids and focus history
typedef struct {
    /**keyboard master id;*/
    int id;
    /**pointer master id associated with id;*/
    int pointerId;
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
    /**Pointer to last binding triggered by this master device*/
    struct binding_struct* lastBindingTriggered;
    /**List of the active chains with the newest first*/
    ArrayList activeChains;
    /**When true, the focus stack won't be updated on focus change*/
    int freezeFocusStack;

    /** pointer to head of node list where the values point to WindowInfo */
    ArrayList windowsToIgnore;

    /**Index of active workspace; this workspace will be used for
     * all workspace operations if a workspace is not specified
     * */
    int activeWorkspaceIndex;
    /// When set, device event rules will be passed this window instead of the active window or focused
    int targetWindow;

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
    int id;
    ///user facing name of the workspace
    char* name;
    ///the monitor the workspace is on
    Monitor* monitor;


    /// is hiding all windows
    char showingDesktop;

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
