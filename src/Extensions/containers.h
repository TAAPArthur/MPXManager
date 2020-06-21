/**
 * @file
 * @brief Implementation of contains, a hybrid of a monitor and a window
 */
#ifndef MPX_EXT_CONTAINERS
#define MPX_EXT_CONTAINERS

#include "../mywm-structs.h"
#include "../boundfunction.h"

/**
 * Creates a fake Monitor and WindowInfo that are linked
 * @return a window id
 */
WindowID createContainer();
/**
 * Checks to see if the monitor id corresponds to a container
 * @param mon
 * @return the WindowInfo* associated with mon
 */
WindowInfo* getWindowInfoForContainer(MonitorID mon);
/**
 * Checks to see if the WindowID corresponds to a container
 * @param win
 * @return the Monitor* associated with win
 */
Monitor* getMonitorForContainer(WindowID win);

/**
 * Adds a X_CONNECTION rule to restore contains based on the presence of fake monitors
 *
 * @param remove
 */
void addResumeContainerRules(bool remove = 0);


/**
 * Create a container and move all windows in the active workspace matching func to said container
 * This is a no-op if containedWorkspace is the active workspace
 *
 * @param containedWorkspace
 * @param func
 * @param name name of container
 *
 * @return the monitor of the newly created container
 */
Monitor* containWindows(Workspace* containedWorkspace, const BoundFunction& func, const char* name = NULL);
/**
 * Move all windows in container to the active workspace
 *
 * @param container
 */
void releaseWindows(Monitor* container);
/**
 * Move all windows in all containers to the active workspace
 */
void releaseAllWindows();
/**
 * Either activate the container window or the container workspace depending if winInfo is a contained window or a container
 *
 * @param winInfo The container window or a contained window
 */
void toggleContainer(WindowInfo* winInfo = getFocusedWindow());
#endif
