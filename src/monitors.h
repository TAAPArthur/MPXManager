/**
 * @file monitors.h
 * Contains methods related to the Monitor struct
 */
#ifndef MONITORS_H_
#define MONITORS_H_

#include "mywm-structs.h"

/**
 * holds topleft coordinates and width/height of the bounding box
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
 * True if the monitor is marked as primary
 * @param monitor
 * @return
 */
int isPrimary(Monitor* monitor);
/**
 *
 * @param id id of monitor TODO need to convert handle long to int conversion
 * @param primary   if the monitor the primary
 * @param geometry an array containing the x,y,width,height of the monitor
 * @return 1 iff a new monitor was added
 */
int updateMonitor(MonitorID id, int primary, Rect geometry);
/**
 * Removes a monitor and frees related info
 * @param id identifier of the monitor
 * @return 1 iff the montior was removed
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
 *
 * @param arg1 x,y,width,height
 * @param arg2 x,y,width,height
 * @return 1 iff the 2 specified rectangles intersect
 */
int intersects(Rect arg1, Rect arg2);

/**
 * Resizes the monitor such that its viewport does not intersec the given dock
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
 * @param winInfo the dock to avoid
 * @see resizeMonitorToAvoidStruct
 */
void resizeAllMonitorsToAvoidStruct(WindowInfo* winInfo);
/**
 * Resizes all monitors such that its viewport does not intersec the given dock
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
