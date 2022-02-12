/**
 * @file
 * @brief Implementation of contains, a hybrid of a monitor and a window
 */
#ifndef MPX_EXT_CONTAINERS
#define MPX_EXT_CONTAINERS

#include "../mywm-structs.h"
#include "../boundfunction.h"
#include "../functions.h"

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

void addContainerRules();

bool createContainerInWorkspace(WindowInfo*winInfo, Workspace* monitorWorkspace);
static inline bool createContainer(WindowInfo*winInfo) {return createContainerInWorkspace(winInfo, NULL);};
/**
 * Either activate the container window or the container workspace depending if winInfo is a contained window or a container
 *
 * @param winInfo The container window or a contained window
 */
void toggleContainer(WindowInfo* winInfo);
#endif
