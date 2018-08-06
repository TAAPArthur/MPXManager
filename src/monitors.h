/**
 * @file monitors.h
 */
#ifndef MONITORS_H_
#define MONITORS_H_

#include "mywm-util.h"


/**
 *
 * @return list of docks
 */
Node*getAllDocks(void);

/**
 * Add properties to winInfo that will be used to avoid docks
 * @param winInfo the window in question
 * @param numberofProperties number of properties
 * @param properties list of properties
 * @see xcb_ewmh_wm_strut_partial_t
 * @see xcb_ewmh_get_extents_reply_t
 * @see avoidStruct
 */
void setDockArea(WindowInfo* winInfo,int numberofProperties,int properties[WINDOW_STRUCT_ARRAY_SIZE]);

/**
 * Reads (one of) the struct property to loads the info into properties and
 * add a dock to the list of docks.
 * @param info the dock to add
 * @return true if this dock was added
 */
int addDock(WindowInfo* info);

/**
 * Removes a dock
 * @param winToRemove
 * @return 1 if a dock was actually removed
 */
int removeDock(unsigned int winToRemove);
/**
 * Query for all monitors
 */
void detectMonitors(void);

void avoidStruct(WindowInfo*info);
/**
 *
 * @param arg1 x,y,width,height
 * @param arg2 x,y,width,height
 * @return 1 iff the 2 specified rectangles intersect
 */
int intersects(short int arg1[4],short int arg2[4]);

/**
 * Resizes the monitor such that its viewport does not intersec the given dock
 * @param m
 * @param winInfo the dock to avoid
 * @return if the size was changed
 */
int resizeMonitorToAvoidStruct(Monitor*m,WindowInfo*winInfo);
/**
 * @see resizeMonitorToAvoidStruct
 */
void resizeAllMonitorsToAvoidAllStructs(void);
/**
 * @param winInfo the dock to avoid
 * @see resizeMonitorToAvoidStruct
 */
void resizeAllMonitorsToAvoidStruct(WindowInfo*winInfo);

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
