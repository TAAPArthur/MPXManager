/**
 * @file workspaces.h
 * @brief Provides methods to manage workspaces
 */
#ifndef WORKSPACES_H
#define WORKSPACES_H
#include <assert.h>

#include "mywm-structs.h"
#include "window-masks.h"
#include "masters.h"
#include "util/arraylist.h"
#include "util/rect.h"

/// Placeholder for WindowInfo->workspaceIndex; indicates the window is not in a workspace
#ifndef NO_WORKSPACE
#define NO_WORKSPACE ((WorkspaceID)-1)
#endif
//
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

///metadata on Workspace
/**
 *
 * All windows in this workspace are treated as if they had this mask in addition to any mask they may already have
 */
struct Workspace {
    WorkspaceID id;
    ///user facing name of the workspace
    char name[MAX_NAME_LEN];
    ///the monitor the workspace is on
    Monitor* monitor;

    /** if the workspace is mapped
     *
     * A workspace should be mapped when it is visible, but assigning a monitor does not does not automatically cause all windows in the workspace to be mapped.
     * This field is here to sync the two states
     *
     */
    bool mapped ;
    Layout* lastTiledLayout;
    Rect lastBounds;
    bool dirty;

    ///an windows stack
    ArrayList windows;

    ///the currently applied layout; does not have to be in layouts
    Layout* activeLayout ;
    ///a circular list of layouts to cycle through
    ArrayList layouts;
    /// offset into layouts when cycling
    uint32_t layoutOffset ;
    WindowMask mask;
};


/**
 * @param index
 * @return the workspace at a given index
 */
static inline Workspace* getWorkspace(WorkspaceID index) {
    return index < getAllWorkspaces()->size ? getElement(getAllWorkspaces(), index) : NULL;
}

/**
 * @return the windows stack of the workspace
 */
ArrayList* getWorkspaceWindowStack(Workspace* workspace);

/**
 *
 * @return the workspace index the window is in or NO_WORKSPACE
 */
WorkspaceID getWorkspaceIndexOfWindow(const WindowInfo* winInfo);
Workspace* getWorkspaceOfWindow(const WindowInfo* winInfo);

/// @return the active Workspace or NULL
static inline Workspace* getActiveWorkspace(void) {return getWorkspace(getActiveWorkspaceIndex());}
/// @return the workspace of the active master
static inline ArrayList* getActiveWindowStack() {return getWorkspaceWindowStack(getActiveWorkspace());}

/**
 * @return the monitor associate with the given workspace if any
 */
Monitor* getMonitor(Workspace*);
/**
 * Assigns a workspace to a monitor. The workspace is now considered visible
 * @param m
 */
void setMonitor(Workspace* workspace, Monitor* m);

/**
 * @return 1 iff the workspace has been assigned a monitor
 */
bool isWorkspaceVisible();

/**
 * The currently used layout for the workspace.
 * Note that this layout does not have to be in the list of layout for the active workspace
 * @param layout the new active layout
 */
void setLayout(Workspace* workspace, Layout* layout);
/**
 *
 * @return the active layout for the active workspace
 */
Layout* getLayout(Workspace* workspace);
static inline Layout* getActiveLayout(void) { return getLayout(getActiveWorkspace());};

/**
 *
 * @return the active layout for the active workspace
 */
void addLayout(Workspace* workspace, Layout* layout);

/**
 * @param mask
 * @return true if there exists at least one window in workspace with the given mask
 */
bool hasWindowWithMask(Workspace* workspace, WindowMask mask) ;
/**
 * Set the layout specified by offset to be the active layout
 *
 * @param offset
 */
void setLayoutOffset(Workspace* workspace, int offset);
/**
 * Returns what would be the new active layout if cycleLayouts(0) was set,
 * which does not have to be the currently active layout
 * @return the layout at position offset
 */
uint32_t getLayoutOffset(Workspace* workspace);
/**
 * If layout != getActiveLayout(), then make layout the new active layout
 * Else call cycleWindow(0)
 *
 * @param layout
 *
 * @return 1 iff the layout changed
 */
bool toggleLayout(Workspace* workspace, Layout* layout);
static inline bool toggleActiveLayout(Layout* layout) {
    return toggleLayout(getActiveWorkspace(), layout);
}

/**
 * Set the next layout in the list to be the active layout
 *
 * @param dir -1, 0 or 1
 */
void cycleLayouts(Workspace* workspace, int dir);
static inline void cycleActiveLayouts(int dir) {
    cycleLayouts(getActiveWorkspace(), dir);
}




/**
 * Swaps the monitors associated with the given workspaces
 * @param index1
 * @param index2
 */
void swapMonitors(WorkspaceID index1, WorkspaceID index2);
static inline void swapActiveMonitor(WorkspaceID index2) {swapMonitors(getActiveWorkspaceIndex(), index2);};


/**
 * Adds the states give by mask to the window
 * @param mask
 */
static inline void addWorkspaceMask(Workspace* workspace, WindowMask mask) {
    workspace->mask |= mask;
}
/**
 * Removes the states give by mask from the window
 * @param mask
 */
static inline void removeWorkspaceMask(Workspace* workspace, WindowMask mask) {
    workspace->mask &= ~mask;
}
/**
 * Adds or removes the mask depending if the window already contains
 * the complete mask
 * @param mask
 */
static inline void toggleWorkspaceMask(Workspace* workspace, WindowMask mask) {
    if((workspace->mask & mask) == mask)
        removeWorkspaceMask(workspace, mask);
    else addWorkspaceMask(workspace, mask);
}
void markWorkspaceOfWindowDirty(WindowInfo* winInfo);
#endif
