/**
 * @file monitors.h
 * Contains methods related to the Monitor struct
 */
#ifndef MONITORS_H_
#define MONITORS_H_

#include "mywm-structs.h"

/**
 * holds top-left coordinates and width/height of the bounding box
 */
typedef struct {
    /// top left coordinate of the bounding box
    short x;
    /// top left coordinate of the bounding box
    short y;
    /// width of the bounding box
    short width;
    /// height of the bounding box
    short height;
} Rect;
/**
 * Represents a rectangular region where workspaces will be tiled
 */
typedef struct Monitor {
    /**id for monitor*/
    MonitorID id;
    /**1 iff the monitor is the primary*/
    char primary;
    /**The unmodified size of the monitor*/
    Rect base;
    /** The modified size of the monitor after docks are avoided */
    Rect view;

} Monitor;
///Masks used to determine the whether two monitors are duplicates
typedef enum {
    /// Monitors are the same iff they have exactly the same bounds
    SAME_DIMS = 1,
    /// Monitors are duplicates if one monitor completely fits inside the other
    CONTAINS = 2,
    /// Monitors are duplicates if one intersects with the other.
    INTERSECTS = 4,
} MonitorDuplicationPolicy;
/// Masks used to dictate how we deal with duplicates monitors
typedef enum {
    /// Take the primary monitors
    TAKE_PRIMARY = 1,
    /// Keep the larger by area (ties are broken arbitrary)
    TAKE_LARGER = 2,
    /// Keep the smaller by area (ties are broken arbitrary)
    TAKE_SMALLER = 4,
} MonitorDuplicationResolution;

/// Used to determine whether two monitors are duplicates of each other
extern MonitorDuplicationPolicy MONITOR_DUPLICATION_POLICY;
/// Given to monitors are duplicates, how do we determine which one to keep
extern MonitorDuplicationResolution MONITOR_DUPLICATION_RESOLUTION;

///Types of docks
typedef enum {DOCK_LEFT, DOCK_RIGHT, DOCK_TOP, DOCK_BOTTOM} DockTypes;

/**
 *
 * @return list of all monitors
 */
ArrayList* getAllMonitors(void);
/**
 *
 * @return list of docks
 */
ArrayList* getAllDocks(void);

/**
 * Creates a new X11 monitor with the given bounds.
 *
 * This method was designed to emulate a picture-in-picture(pip) experience, but can be used to create any arbitrary monitor
 *
 * @param bounds the position of the new monitor
 */
void createMonitor(Rect bounds);
/**
 * Clears all fake monitors
 * @see pip
 */
void clearFakeMonitors(void);

/**
 * True if the monitor is marked as primary
 * @param monitor
 * @return
 */
bool isPrimary(Monitor* monitor);
/**
 * Marks this monitor as the primary. There can only be 1 primary monitor, so the current primary monitor will lose its status
 *
 * @param monitor
 * @param primary the value of primary
 * @param sync if true we update the Xserver in addition to our local structs. If there is no XRANDR support this parameter will be treated as 0
 * @return 0 if no error
 */
int setPrimary(Monitor* monitor, bool primary, bool sync);

/**
 *
 * @param id id of monitor
 * @param geometry an array containing the x,y,width,height of the monitor
 * @param autoAssignWorkspace if true, then monitor will be given a free workspace if possible
 * @return 1 iff a new monitor was added
 */
bool updateMonitor(MonitorID id, Rect geometry, bool autoAssignWorkspace);
/**
 * Removes a monitor and frees related info
 * @param id identifier of the monitor
 * @return 1 iff the monitor was removed
 */
int removeMonitor(MonitorID id);
/**
 * Resets the viewport to be the size of the rectangle
 * @param m
 */
void resetMonitor(Monitor* m);


/**
 * Add properties to winInfo that will be used to avoid docks
 * @param winInfo the window in question
 * @param numberofProperties number of properties
 * @param properties list of properties
 * @see xcb_ewmh_wm_strut_partial_t
 * @see xcb_ewmh_get_extents_reply_t
 * @see avoidStruct
 */
void setDockArea(WindowInfo* winInfo, int numberofProperties, int* properties);

/**
 * Reads (one of) the struct property to loads the info into properties and
 * add a dock to the list of docks.
 * @param info the dock
 * @return true if properties were successfully loaded
 */
int loadDockProperties(WindowInfo* info);

/**
 * Query for all monitors
 */
void detectMonitors(void);
/**
 * Loops over all monitors and assigns the ones without a workspace to an arbitrary empty workspace
 */
void assignUnusedMonitorsToWorkspaces(void);

/**
 *
 * @param arg1 x,y,width,height
 * @param arg2 x,y,width,height
 * @return 1 iff the 2 specified rectangles intersect
 */
int intersects(Rect arg1, Rect arg2);
/**
 * Checks to see if arg1 contains arg2. For the check to pass arg2 must completely reside within all borders of arg2
 * @param arg1
 * @param arg2
 *
 * @return 1 iff arg1 completely contains arg2
 */
int contains(Rect arg1, Rect arg2);
/**
 *
 *
 * @param arg1
 * @param arg2
 *
 * @return 1 if arg1 has a larger area than arg2
 */
int isLarger(Rect arg1, Rect arg2);

/**
 * Resizes the monitor such that its viewport does not intersect the given dock
 * @param m
 * @param winInfo the dock to avoid
 * @return if the size was changed
 */
int resizeMonitorToAvoidStruct(Monitor* m, WindowInfo* winInfo);
/**
 * @see resizeMonitorToAvoidStruct
 */
void resizeAllMonitorsToAvoidAllStructs(void);
/**
 * This method should only be used if a new dock is being added or an existing dock is being grown as this method can only reduce a monitor's viewport
 * @param winInfo the dock to avoid
 * @see resizeMonitorToAvoidStruct
 */
void resizeAllMonitorsToAvoidStruct(WindowInfo* winInfo);
/**
 * Resizes all monitors such that its viewport does not intersect the given dock
 *
 * @param winInfo
 */
void resizeAllMonitorsToAvoidStructs(WindowInfo* winInfo);

/**
 *
 * @return the width of the root window
 */
int getRootWidth(void);
/**
 *
 * @return the height of the root window
 */
int getRootHeight(void);
#endif
