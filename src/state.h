/**
 * @file state.h
 * Tracks state of workspaces
 */
#ifndef STATE_H
#define STATE_H
#include <cstdint>
///Return type of updateState that dictates what changed
typedef enum {
    //nothing changed
    NO_CHANGE = 0,
    /// window related state changed
    WORKSPACE_WINDOW_CHANGE = 1,
    /// workspace-monitor pairing changed
    WORKSPACE_MONITOR_CHANGE = 2,
    WINDOW_CHANGE = 4,
} StateChangeType;

bool isStateMarked(void);
/**
 * Marks that the state may have possibility changed
 */
void markState(void);
/**
 * Unmarks that the state may have possibility changed
 */
void unmarkState(void);
/**
 * Calls onChange (if not NULL) for every (currently visible) workspace changed since
 * this function was last called. A workspace if considered to have changed if
 * <ul>
 * <li> the bounding box of the active monitor has changed </li>
 * <li> the number of interactable windows has changed </li>
 * <li> the stacking order of the interactable windows has changed </li>
 * <li> the user mask for any interactable window has changed </li>
 * </ul>
 * @param onWorkspaceWindowChange the callback function to be trigger for every workspace when set of window state changes
 * @param onWorkspaceMonitorChange the callback function to be trigger for every workspace when workspace-monitor pair changes
 * that has changed
 * @return 1 iff the state has actually changed
 */
int updateState();

#endif
