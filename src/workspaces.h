/**
 * @file workspaces.h
 * @brief Provides methods to manage workspaces
 */
#ifndef WORKSPACES_H
#define WORKSPACES_H

#include "mywm-structs.h"
#include "window-masks.h"
#include <string>
#include <assert.h>

/// Placeholder for WindowInfo->workspaceIndex; indicates the window is not in a workspace
#ifndef NO_WORKSPACE
#define NO_WORKSPACE ((WorkspaceID)-1)
#endif
//
///@see Workspace::getNextWorkspace()
enum WorkSpaceFilter {
    ///only return visible workspaces
    VISIBLE = 1,
    ///only return invisible workspaces
    HIDDEN = 2,
    ///only return non empty workspaces
    NON_EMPTY = 4,
    ///only return empty workspaces
    EMPTY = 8,
} ;

/**
 * @return a list of all workspaces
 */
const ArrayList<Workspace*>& getAllWorkspaces();
/**
 *
 * @return the number of workspaces
 * @see createContext
 */
uint32_t getNumberOfWorkspaces();
/**
 * Creates and adds num workspace
 * @param num the number of workspaces to add
 */
void addWorkspaces(int num);
/**
 * Removes (and frees) the last num workspaces
 * At least one workspace will be left by this function
 *
 * @param num number of workspace to remove
 */
void removeWorkspaces(int num);
/**
 * Destroys every single workspaces
 */
void removeAllWorkspaces();
///metadata on Workspace
/**
 *
 * All windows in this workspace are treated as if they had this mask in addition to any mask they may already have
 */
struct Workspace : WMStruct, HasMask {
private:
    ///user facing name of the workspace
    std::string name;
    ///the monitor the workspace is on
    Monitor* monitor = NULL;

    /** if the workspace is mapped
     *
     * A workspace should be mapped when it is visible, but assigning a monitor does not does not automatically cause all windows in the workspace to be mapped.
     * This field is here to sync the two states
     *
     */
    bool mapped = 0;

    ///an windows stack
    ArrayList<WindowInfo*> windows;

    ///the currently applied layout; does not have to be in layouts
    Layout* activeLayout = NULL;
    ///a circular list of layouts to cycle through
    ArrayList<Layout*> layouts;
    /// offset into layouts when cycling
    uint32_t layoutOffset = 0;

public:
    /**
     * Creates a new Workspace with id equal the current number of workspaces
     * and name with the string conversion of its id
     *
     */
    Workspace(): WMStruct(getNumberOfWorkspaces()), name(std::to_string(id + 1)) { }
    /**
     * @return the monitor associate with the given workspace if any
     */
    Monitor* getMonitor() {return monitor;};
    /// returns the name of this workspace
    const std::string& getName()const {return name;};
    /// @param n the new name of the workspace
    void setName(std::string n) {name = n;};

    /**
     * Get the next workspace in the given direction according to the filters
     * @param dir the interval of workspaces to check
     * @param mask species a filter for workspaces @see WorkspaceFilter
     * @return the next workspace in the given direction that matches the criteria
     */
    Workspace* getNextWorkspace(int dir, int mask);
    /**
     * @return 1 iff the workspace has been assigned a monitor
     */
    bool isVisible()const {
        return ((Workspace*)(this))->getMonitor() ? 1 : 0;
    }
    /**
     * @return the windows stack of the workspace
     */
    ArrayList<WindowInfo*>& getWindowStack() {return windows;}
    /**
     * Prints the workspace
     *
     * @param strm
     * @param w
     *
     * @return
     */
    friend std::ostream& operator<<(std::ostream& strm, const Workspace& w) ;
    /**
     * Assigns a workspace to a monitor. The workspace is now considered visible
     * @param m
     */
    void setMonitor(Monitor* m);
    /**
     * The currently used layout for the workspace.
     * Note that this layout does not have to be in the list of layout for the active workspace
     * @param layout the new active layout
     */
    void setActiveLayout(Layout* layout) {
        activeLayout = layout;
    }
    /**
     *
     * @return the active layout for the active workspace
     */
    Layout* getActiveLayout(void)const {return activeLayout;}
    /**
     * @param mask
     * @return true if there exists at least one window in workspace with the given mask
     */
    bool hasWindowWithMask(WindowMask mask) ;

    /**
     * @return layouts of w
     */
    ArrayList<Layout*>& getLayouts() {
        return layouts;
    }
    /**
     * Set the next layout in the list to be the active layout
     *
     * @param dir -1, 0 or 1
     */
    void cycleLayouts(int dir);
    /**
     * Set the layout specified by offset to be the active layout
     *
     * @param offset
     */
    void setLayoutOffset(int offset);
    /**
     * Returns what would be the new active layout if cycleClayouts(0) was set,
     * which does not have to be the currently active layout
     * @return the layout at position offset
     */
    uint32_t getLayoutOffset() {return layoutOffset;}
    /**
     * If layout != getActiveLayout(), then make layout the new active layout
     * Else call cycleWindow(0)
     *
     * @param layout
     *
     * @return 1 iff the layout changed
     */
    bool toggleLayout(Layout* layout);
};



/**
 * @param name
 *
 * @return the workspace with the specified name
 */
Workspace* getWorkspaceByName(std::string name);
/**
 * @param index
 * @return the workspace at a given index
 */
static inline Workspace* getWorkspace(WorkspaceID index) {
    assert(index < getAllWorkspaces().size());
    return getAllWorkspaces()[index];
}
#endif
