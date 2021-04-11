/**
 * @file monitors.h
 * Contains methods related to the Monitor struct
 */
#ifndef MONITORS_H_
#define MONITORS_H_

#include "mywm-structs.h"
#include "util/rect.h"
#include "util/arraylist.h"

/**
 *
 * @return list of all monitors
 */
ArrayList* getAllMonitors(void);
Monitor* getMonitorByID(MonitorID id);
/**
 * Represents a rectangular region where workspaces will be tiled
 */
struct Monitor {
    const MonitorID id;
    /**The unmodified size of the monitor*/
    Rect base;
    /** The modified size of the monitor after docks are avoided */
    Rect view;
    /// human readable name given by the XServer
    char name[MAX_NAME_LEN];
    const bool fake;
    bool _mark;
    uint32_t output;
    bool inactive;
    /// Raise windows relative to this window (default 0)
    WindowID stackingWindow;
};

Monitor* newMonitor(MonitorID id, Rect base, const char* name, bool fake);
void freeMonitor(Monitor* monitor);

/**
 * @param win the new value of stackingWindow
 */
void setStackingWindow(Monitor* monitor, WindowID win);
/**
 * @return WindowID that normal windows in the workspace associated with monitor should be tiled above
 */
WindowID getStackingWindow(Monitor* monitor);
/// @return true if the monitor isn't backed by X
bool isFakeMonitor(Monitor* monitor);
/**
 * Returns the workspace currently displayed by the monitor or null
 * @return
 */
Workspace* getWorkspaceOfMonitor(Monitor* monitor);
/**
 * if workspace is not null, the workspace will be displayed on this monitor;
 * Else an arbitrary unclaimed Workspace will be chosen
 *
 * @param workspace
 */
void assignWorkspace(Monitor* monitor, Workspace* workspace);
void assignEmptyWorkspace(Monitor* monitor);
/**
 * @return True if the monitor is marked as primary
 */
bool isPrimary(Monitor* monitor);
/**
 * Marks this monitor as the primary. There can only be 1 primary monitor, so the current primary monitor will lose its status
 *
 * @param monitor the primary monitor or 0
 */
void setPrimary(MonitorID monitor);
/**
 * Adjusts the viewport so that it doesn't intersect any docs
 *
 * @return 1 iff the size changed
 */
bool resizeToAvoidAllDocks(Monitor* monitor);
/// @return true iff a Workspace with this monitor can be considered visibile
bool isMonitorActive(Monitor* monitor);

///Masks used to determine the whether two monitors are duplicates
enum MonitorDuplicationPolicy {
    /// Monitors are never considered duplicates
    NO_DUPS = 0,
    /// Monitors are duplicates if they have exactly the same base
    SAME_DIMS = 1,
    /// Monitors are duplicates if one monitor completely fits inside the other
    CONTAINS = 2,
    /// Monitors are duplicates if one monitor completely fits inside the other and is not touching any edges
    CONTAINS_PROPER = 4,
    /// Monitors are duplicates if one intersects with the other.
    INTERSECTS = 8,
    /// Consider fake monitors
    CONSIDER_ONLY_NONFAKES = 16,
} ;
/// Masks used to dictate how we deal with duplicates monitors
enum MonitorDuplicationResolution {
    /// Take the primary monitors
    TAKE_PRIMARY = 1,
    /// Keep the larger by area (ties are broken arbitrary)
    TAKE_LARGER = 2,
    /// Keep the smaller by area (ties are broken arbitrary)
    TAKE_SMALLER = 4,
};

/// Used to determine whether two monitors are duplicates of each other
extern uint32_t MONITOR_DUPLICATION_POLICY;
/// Given to monitors are duplicates, how do we determine which one to keep
extern uint32_t MONITOR_DUPLICATION_RESOLUTION;


/**
 * @return  the primary monitor or NULL
 */
Monitor* getPrimaryMonitor() ;
/**
 * @return  the workspace of the primary monitor or NULL
 */
static inline Workspace* getPrimaryWorkspace() {
    Monitor* m = getPrimaryMonitor();
    return m ? getWorkspaceOfMonitor(m) : NULL;
}

/**
 * Removes monitors that are duplicates according to MONITOR_DUPLICATION_POLICY
 * and MONITOR_DUPLICATION_RESOLUTION
 */
void removeDuplicateMonitors(void);

/**
 * Query for all monitors
 */
void detectMonitors(void);
/**
 * Loops over all monitors and assigns the ones without a workspace to an arbitrary empty workspace
 */
void assignUnusedMonitorsToWorkspaces(void);


/**
 * @see resizeMonitorToAvoidStruct
 */
void resizeAllMonitorsToAvoidAllDocks(void);
/**
 * This method should only be used if a new dock is being added or an existing dock is being grown as this method can only reduce a monitor's viewport
 * @param winInfo the dock to avoid
 * @see resizeMonitorToAvoidStruct
 */
void resizeAllMonitorsToAvoidDock(WindowInfo* winInfo);
/**
 * Resizes all monitors such that its viewport does not intersect the given dock
 *
 * @param winInfo
 */
void resizeAllMonitorsToAvoidStructs(WindowInfo* winInfo);

/**
 * Adds a monitors with id 1 at pos 0,0 and size getRootWidth()xgetRootHeigh() if it doesn't already exists
 */
Monitor* addRootMonitor();
/**
 * Sets the root width and height
 *
 * @param s an array of size 2
 */
void setRootDims(uint16_t w, uint16_t h);
/**
 *
 * @return the width of the root window
 */
uint16_t getRootWidth(void);
/**
 *
 * @return the height of the root window
 */
uint16_t getRootHeight(void);
/**
 * Creates a new X11 monitor with the given bounds.
 *
 * This method was designed to emulate a picture-in-picture(pip) experience, but can be used to create any arbitrary monitor
 *
 * @param bounds the position of the new monitor
 * @param name
 */
Monitor* addFakeMonitorWithName(Rect bounds, const char* name);
static inline Monitor* addFakeMonitor(Rect bounds) {return addFakeMonitorWithName(bounds, "");}

void setBase(Monitor* monitor, const Rect rect);

void clearAllFakeMonitors(void);
#endif
