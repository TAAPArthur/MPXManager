/**
 * @file workspaces.h
 * @brief Provides methods to manage workspaces
 */
#ifndef WORKSPACES_H
#define WORKSPACES_H

#include "mywm-structs.h"
#include <string>
#include <assert.h>

/// Placeholder for WindowInfo->workspaceIndex; indicates the window is not in a workspace
#ifndef NO_WORKSPACE
    #define NO_WORKSPACE -1
#endif
//
///@see Workspace::getNextWorkspace()
typedef enum {
    ///only return visible workspaces
    VISIBLE = 1,
    ///only return invisible workspaces
    HIDDEN = 2,
    ///only return non empty workspaces
    NON_EMPTY = 4,
    ///only return empty workspaces
    EMPTY = 8,
} WorkSpaceFilter;

const ArrayList<Workspace*>& getAllWorkspaces();
/**
 *
 * @return the number of workspaces
 * @see createContext
 */
int getNumberOfWorkspaces();
/**
 * Creates and adds num workspace
 * @param num the number of workspaces to add
 */
void addWorkspaces(int num);
/**
 * Removes (and frees) the last num workspaces
 *
 * @param num number of workspace to remove
 */
void removeWorkspaces(int num);
void removeAllWorkspaces();
///metadata on Workspace
typedef struct Workspace : WMStruct {
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
    /**
     * All windows in this workspace are treated as if they had this mask in addition to any mask they may already have
     */
    WindowMask mask = 0;

    ///an windows stack
    ArrayList<WindowInfo*> windows;

    ///the currently applied layout; does not have to be in layouts
    Layout* activeLayout = NULL;
    ///a circular list of layouts to cycle through
    ArrayList<Layout*> layouts;
    /// offset into layouts when cycling
    uint32_t layoutOffset = 0;

public:
    Workspace(): WMStruct(getNumberOfWorkspaces()), name(std::to_string(id + 1)) {  }
    /**
     * @param workspace
     * @return the monitor associate with the given workspace if any
     */
    Monitor* getMonitor() {return monitor;};
    const std::string& getName()const {return name;};
    void setName(std::string n) {name = n;};

    /**
     * Get the next workspace in the given direction according to the filters
     * @param dir the interval of workspaces to check
     * @param mask species a filter for workspaces @see WorkspaceFilter
     * @return the next workspace in the given direction that matches the criteria
     */
    Workspace* getNextWorkspace(int dir, int mask);
    /**
     *
     * @param i the workspace index
     * @return 1 iff the workspace has been assigned a monitor
     */
    bool isVisible()const {
        return ((Workspace*)(this))->getMonitor() ? 1 : 0;
    }
    /**
     * @param workspace
     * @return the windows stack of the workspace
     */
    ArrayList<WindowInfo*>& getWindowStack() {return windows;}
    friend std::ostream& operator<<(std::ostream& strm, const Workspace& w) ;
    /**
     * Assigns a workspace to a monitor. The workspace is now considered visible
     * @param w
     * @param m
     */
    void setMonitor(Monitor* m);
    /**
     * Adds a mask to the workspace
     * @see Workspace::mask
     *
     * @param w
     * @param mask
     */
    void addMask(WindowMask mask) {
        this->mask |= mask;
    }
    /**
     * Removes a mask to the workspace
     * @see Workspace::mask
     *
     * @param w
     * @param mask
     */
    void removeMask(WindowMask mask) {
        this->mask &= ~mask;
    }
    /**
     * @return the mask of w
     */
    int getMask() {
        return mask;
    }
    int hasPartOfMask(WindowMask mask) {
        return getMask()& mask;
    }
    /**
     * @param mask
     *
     * @return 1 if w fully contains mask
     */
    bool hasMask(WindowMask mask) {
        return hasPartOfMask(mask) == mask;
    }
    void toggleMask(WindowMask mask) {
        if(hasMask(mask))
            removeMask(mask);
        else addMask(mask);
    }
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
     * @return layouts of w
     */
    ArrayList<Layout*>& getLayouts() {
        return layouts;
    }
    void cycleLayouts(int dir);
    void setLayoutOffset(int offset);
    uint32_t getLayoutOffset() {return layoutOffset;}
    int toggleLayout(Layout* layout);
} Workspace;



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
