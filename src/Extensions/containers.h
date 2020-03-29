/**
 * @file
 * @brief Implementation of contains, a hybrid of a monitor and a window
 */
#ifndef MPX_EXT_CONTAINERS
#define MPX_EXT_CONTAINERS

#include "../mywm-structs.h"

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

#endif
