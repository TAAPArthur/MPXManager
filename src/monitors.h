/**
 * @file monitors.h
 * Contains methods related to the Monitor struct
 */
#ifndef MONITORS_H_
#define MONITORS_H_

#include "mywm-structs.h"
#include "masters.h"
#include <string>
#include "rect.h"

/**
 *
 * @return list of all monitors
 */
ArrayList<Monitor*>& getAllMonitors(void);
/**
 * Represents a rectangular region where workspaces will be tiled
 */
struct Monitor : WMStruct {
private:
    /**The unmodified size of the monitor*/
    Rect base;
    /** The modified size of the monitor after docks are avoided */
    Rect view;
    /**1 iff the monitor is the primary*/
    bool primary;
    /// human readable name given by the XServer
    std::string name;
public:
    /**
     * @param id the X unique id of the monitor
     * @param base the bounds the monitor occupies
     * @param primary 1 iff it is the primary monitor
     * @param name
     *
     */
    Monitor(MonitorID id, Rect base, bool primary = 0, std::string name = ""): WMStruct(id), base(base), view(base),
        primary(primary),
        name(name) {}

    ~Monitor();
    /**
     * Returns the workspace currently displayed by the monitor or null

     * @return
     */
    Workspace* getWorkspace(void);
    /// @return the human readable name
    const std::string& getName() const {return name;}
    /// @param str the new name of the monitor
    void setName(std::string str) {name = str;}
    /// @return the bounds of the monitor
    const Rect& getBase() const {return base;}
    /// @param rect the new bounds of the monitor
    void setBase(Rect rect);
    /// @return the area in which windows will be tiled
    const Rect& getViewport()const {return view;}
    /// @param rect the new area in which windows will be tiled
    void setViewport(Rect rect) {view = rect;}
    /**
     * if workspace is not null, the workspace will be displayed on this monitor;
     * Else an arbitrary unclaimed Workspace will be chosen
     *
     * @param workspace
     */
    void assignWorkspace(Workspace* workspace = NULL);
    /**
     * @return True if the monitor is marked as primary
     */
    bool isPrimary();
    /**
     * Marks this monitor as the primary. There can only be 1 primary monitor, so the current primary monitor will lose its status
     *
     * @param primary the value of primary
     */
    void setPrimary(bool primary = true);
    /**
     * Resets the viewport to be the size of the rectangle
     */
    void reset();
    /**
     * Adjusts the viewport so that it doesn't intersect any docs
     *
     * @return 1 iff the size changed
     */
    bool resizeToAvoidAllDocks();
    /**
     *
     * Adjusts the viewport so that it doesn't intersect winInfo
     *
     * @param winInfo WindowInfo
     *
     * @return 1 iff the size changed
     */
    bool resizeToAvoidDock(WindowInfo* winInfo);
};
///Masks used to determine the whether two monitors are duplicates
enum MonitorDuplicationPolicy {
    /// Monitors are duplicates if they have exactly the same base
    SAME_DIMS = 1,
    /// Monitors are duplicates if one monitor completely fits inside the other
    CONTAINS = 2,
    /// Monitors are duplicates if one monitor completely fits inside the other and is not touching any edges
    CONTAINS_PROPER = 4,
    /// Monitors are duplicates if one intersects with the other.
    INTERSECTS = 8,
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

///Types of docks
enum DockTypes {DOCK_LEFT, DOCK_RIGHT, DOCK_TOP, DOCK_BOTTOM} ;


/**
 * @return  the primary monitor or NULL
 */
Monitor* getPrimaryMonitor() ;



/**
 * Reads (one of) the struct property to loads the info into properties and
 * add a dock to the list of docks.
 * @param info the dock
 * @return true if properties were successfully loaded
 */
int loadDockProperties(WindowInfo* info);

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
void addRootMonitor();
/**
 * Sets the root width and height
 *
 * @param s an array of size 2
 */
void setRootDims(uint16_t* s);
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
 * Swaps the monitors associated with the given workspaces
 * @param index1
 * @param index2
 */
void swapMonitors(WorkspaceID index1, WorkspaceID index2 = getActiveMaster()->getWorkspaceIndex());
#endif
