/**
 * @file workspaces.h
 * @brief Provides methods to manage workspaces
 */
#ifndef WORKSPACES_H
#define WORKSPACES_H

#include "mywm-structs.h"

/// Placeholder for WindowInfo->workspaceIndex; indicates the window is not in a workspace
#ifndef NO_WORKSPACE
    #define NO_WORKSPACE -256
#endif

/**
 *
 * @return the number of workspaces
 * @see createContext
 */
int getNumberOfWorkspaces();
/**
 * Deletes all workspaces
 */
void resetWorkspaces(void);
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

/**
 *
 * @return the windows in the active workspace at the NORMAL_LAYER
 */
ArrayList* getActiveWindowStack();

/**
 * @param workspace
 * @return the windows stack of the workspace
 */
ArrayList* getWindowStack(Workspace* workspace);


/**
 * @param index
 * @return the workspace at a given index
 */
Workspace* getWorkspaceByIndex(int index);


/**
 * @param winInfo
 *
 * @return the workspace index the window is in or NO_WORKSPACE
 */
int getWorkspaceIndexOfWindow(WindowInfo* winInfo);

/**
 * @param winInfo a window that is in a workspace
 * @return the workspace the window is in or NULL
 */
Workspace* getWorkspaceOfWindow(WindowInfo* winInfo);
/**
 * @param winInfo a window that is in a workspace
 * @return the window stack of the workspace the window is in
 */
ArrayList* getWindowStackOfWindow(WindowInfo* winInfo);
/**
 *
 * @param i the workspace index
 * @return 1 iff the workspace has been assigned a monitor
 */
int isWorkspaceVisible(int i);
/**
 *
 * @param index
 * @return if the workspace has at least one window explicitly assigned to it
 */
int isWorkspaceNotEmpty(int index);

///@see getNextWorkspace()
typedef enum {
    ///only return visible workspaces
    VISIBLE = 1,
    ///only return invisible workspaces
    HIDDEN = 2,
    ///only return non empty workspaces
    NON_EMPTY = 1 << 2,
    ///only return empty workspaces
    EMPTY = 2 << 2
} WorkSpaceFilter;
/**
 * Get the next workspace in the given direction according to the filters
 * @param dir the interval of workspaces to check
 * @param mask species a filter for workspaces @see WorkSpaceFilter
 * @return the next workspace in the given direction that matches the criteria
 */
Workspace* getNextWorkspace(int dir, int mask);
/**
 * The currently used layout for the active workspace.
 * Note that this layout does not have to be in the list of layout for the active workspace
 * @param layout the new active layout
 */
void setActiveLayout(Layout* layout);

/**
 *
 * @return the active layout for the active workspace
 */
Layout* getActiveLayout();

/**
 * @param workspaceIndex
 * @return the active layout for the specified workspace
 */
Layout* getActiveLayoutOfWorkspace(int workspaceIndex);

/**
 * @param w
 * @return layouts of w
 */
ArrayList* getLayouts(Workspace* w);
/**
 *
 * @param winInfo
 * @return 1 if the window is in a visible workspace
 * @see isWindowInWorkspace
 */
int isWindowNotInInvisibleWorkspace(WindowInfo* winInfo);

/**
 * Removes a window from a workspace
 * @param winInfo
 * @return 1 iff the window was actually removed
 */
int removeWindowFromWorkspace(WindowInfo* winInfo);
/**
 *
 * @param info
 * @param workspaceIndex
 * @return 1 iff the window was actually added
 * @see addWindowToWorkspaceAtLayer
 */
int addWindowToWorkspace(WindowInfo* info, int workspaceIndex);
/**
 * Returns the workspace currently displayed by the monitor or null

 * @param monitor
 * @return
 */
Workspace* getWorkspaceFromMonitor(Monitor* monitor);

/**
 * @param workspace
 * @return the monitor associate with the given workspace if any
 */
Monitor* getMonitorFromWorkspace(Workspace* workspace);

/**
 * Swaps the monitors associated with the given workspaces
 * @param index1
 * @param index2
 */
void swapMonitors(int index1, int index2);



/**
 * Assigns a workspace to a monitor. The workspace is now considered visible
 * @param w
 * @param m
 */
void setMonitorForWorkspace(Workspace* w, Monitor* m);
/**
 * Return the first non-special workspace that a window is in
 * starting from the least recently focused window
 * @return the active workspace
 */
Workspace* getActiveWorkspace(void);


/**
 *
 * @return the active workspace index
 */
int getActiveWorkspaceIndex(void);
/**
 * Sets the active workspace index
 * @param index
 */
void setActiveWorkspaceIndex(int index);
/**
 * Adds a mask to the workspace
 * @see Workspace::mask
 *
 * @param w
 * @param mask
 */
void addWorkspaceMask(Workspace* w, WindowMask mask);
/**
 * Removes a mask to the workspace
 * @see Workspace::mask
 *
 * @param w
 * @param mask
 */
void removeWorkspaceMask(Workspace* w, WindowMask mask);
/**
 *
 * @param w
 *
 * @return the mask of w
 */
int getWorkspaceMask(Workspace* w);
/**
 *
 *
 * @param w
 * @param mask
 *
 * @return 1 if w fully contains mask
 */
int hasWorkspaceMask(Workspace* w, WindowMask mask);
#endif
