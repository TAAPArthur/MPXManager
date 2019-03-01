#ifndef WORKSPACES_H
#define WORKSPACES_H

#include "mywm-structs.h"

/**
 *
 * @return the number of workspaces
 * @see createContext
 */
int getNumberOfWorkspaces();
ArrayList* getListOfWorkspaces(void);
/**
 *
 * @return the windows in the active workspace at the NORMAL_LAYER
 */
ArrayList* getActiveWindowStack();


/**
 * @param index
 * @return the workspace at a given index
 */
Workspace* getWorkspaceByIndex(int index);

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
 * @param mask species a filter for workpspaces @see WorkSpaceFilter
 * @return the next workspace in the given direction that matches the criteria
 */
Workspace* getNextWorkspace(int dir, int mask);
/**
 * The the currently used layout for the active workspace.
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

ArrayList* getLayouts(Workspace* w);
/**
 * @return newly created workspace
 */
Workspace* createWorkspace();
/**
 * Does a simple seach to see if the window is in the workspace's stack
 * Does not handle advaned cases like cloning
 * @param winInfo
 * @param workspaceIndex
 * @return true if the window with the given id is the specifed workspace
 */
ArrayList* isWindowInWorkspace(WindowInfo* winInfo, int workspaceIndex);
/**
 *
 * @param winInfo
 * @return 1 if the window is in a visible workspace
 * @see isWindowInWorkspace
 */
int isWindowInVisibleWorkspace(WindowInfo* winInfo);

/**
 * Removes a window from a workspace
 * @param winInfo
 * @param workspaceIndex
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
 * @return the monitor assoicated with the given workspace if any
 */
Monitor* getMonitorFromWorkspace(Workspace* workspace);

/**
 * Swaps the monitors assosiated with the given workspaces
 * @param index1
 * @param index2
 */
void swapMonitors(int index1, int index2);



/**
 * Assgins a workspace to a monitor. The workspace is now considered visible
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
#endif
