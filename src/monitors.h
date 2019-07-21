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
    TimeStamp lastDetected = 0;
public:
    Monitor(MonitorID id, Rect base, bool primary = 0, std::string name = ""): WMStruct(id), base(base), view(base),
        primary(primary),
        name(name) {}

    ~Monitor();
    /**
     * Returns the workspace currently displayed by the monitor or null

     * @param monitor
     * @return
     */
    Workspace* getWorkspace(void);
    const std::string& getName() const {return name;}
    void setName(std::string c) {name = std::string(c);}
    TimeStamp getTimeStamp() {return lastDetected;}
    void setTimeStamp(TimeStamp t) {lastDetected = t;}
    const Rect& getBase() const {return base;}
    void setBase(Rect rect);
    const Rect& getViewport()const {return view;}
    void setViewport(Rect rect) {view = rect;}
    void assignWorkspace(Workspace* workspace = NULL);
    /**
     * True if the monitor is marked as primary
     * @param monitor
     * @return
     */
    bool isPrimary();
    /**
     * Marks this monitor as the primary. There can only be 1 primary monitor, so the current primary monitor will lose its status
     *
     * @param monitor
     * @param primary the value of primary
     */
    void setPrimary(bool primary = true);
    /**
     * Resets the viewport to be the size of the rectangle
     * @param m
     */
    void reset();
    bool resizeToAvoidAllDocks();
    bool resizeToAvoidDock(WindowInfo*);
};
///Masks used to determine the whether two monitors are duplicates
typedef enum {
    /// Monitors are duplicates if they have exactly the same base
    SAME_DIMS = 1,
    /// Monitors are duplicates if one monitor completely fits inside the other
    CONTAINS = 2,
    /// Monitors are duplicates if one monitor completely fits inside the other and is not touching any edges
    CONTAINS_PROPER = 4,
    /// Monitors are duplicates if one intersects with the other.
    INTERSECTS = 8,
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
extern uint32_t MONITOR_DUPLICATION_POLICY;
/// Given to monitors are duplicates, how do we determine which one to keep
extern uint32_t MONITOR_DUPLICATION_RESOLUTION;

///Types of docks
typedef enum {DOCK_LEFT, DOCK_RIGHT, DOCK_TOP, DOCK_BOTTOM} DockTypes;





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

void addRootMonitor();
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
