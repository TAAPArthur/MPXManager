#include <assert.h>
#include <stdlib.h>
#include <string.h>


#include "bindings.h"
#include "util/logger.h"
#include "monitors.h"
#include "state.h"
#include "user-events.h"
#include "windows.h"
#include "wmfunctions.h"
#include "workspaces.h"

#ifdef TODO_REMOVE

/**
 * Holds metadata about all the workspaces
 * This metadata will be compared to the current workspaces and if they
 * don't match, we consider the workspaces to have changed
 */
struct WorkspaceState {
    ///representing the view port of the workspaces' monitors
    Rect monitorViewport;
    /// if the workspace is known to be visible
    bool visible;
    bool forceRetile;
    /// the size of windowIds and windowMasks arrays
    int size;
    ///list of window ids in workspace stack order
    WindowID* windowIds;
    /// list of user masks of windows in workspace stack order
    WindowMask* windowMasks;
    /// the layout of the workspace
    void* layout;
    ~WorkspaceState() {
        if(size) {
            delete[] windowIds;
            delete[] windowMasks;
        }
    }
};

static WorkspaceID numberOfRecordedWorkspaces;
static WorkspaceState* savedStates;

static void destroyCurrentState() {
    delete[] savedStates;
    savedStates = NULL;
}

static WorkspaceState* computeState() {
    WorkspaceState* states = new WorkspaceState[getNumberOfWorkspaces()];
    for(WorkspaceID i = 0; i < getNumberOfWorkspaces(); i++) {
        Workspace* w = getWorkspace(i);
        if(!w->isVisible() && i < numberOfRecordedWorkspaces)
            states[i].monitorViewport = savedStates[i].monitorViewport;
        else
            states[i].monitorViewport = w->getMonitor() ? w->getMonitor()->getViewport() : (Rect) {0, 0, 0, 0};
        if(i < numberOfRecordedWorkspaces)
            states[i].forceRetile = savedStates[i].forceRetile;
        states[i].visible = w->isVisible();
        states[i].layout = w->getActiveLayout();
        int size = w->getWindowStack().size();
        int j = 0;
        if(size) {
            states[i].windowIds = new WindowID[size];
            states[i].windowMasks = new WindowMask[size];
            for(WindowInfo* winInfo : w->getWindowStack()) {
                if(winInfo->hasPartOfMask(RETILE_MASKS)) {
                    states[i].windowIds[j] = winInfo->getEffectiveID();
                    states[i].windowMasks[j] = winInfo->hasPartOfMask(RETILE_MASKS);
                    j++;
                }
            }
        }
        states[i].size = j;
    }
    return states;
}

static inline void _printStateComparison(WorkspaceState* currentState, WorkspaceID i) {
    if(currentState[i].size || i < numberOfRecordedWorkspaces && savedStates[i].size) {
        VERBOSE("Index:  " << i << "\n" << "Current:" << currentState[i]);
        if(i < numberOfRecordedWorkspaces)
            VERBOSE("Saved: " << savedStates[i]);
    }
}

static bool updateWindowsNotInWorkspaces() {
    static Index<WindowMask> key;
    bool changed = 0;
    for(WindowInfo* winInfo : getAllWindows()) {
        if(!winInfo->getWorkspace()) {
            auto* mask = get(key, winInfo);
            if((winInfo->getMask() ^ *mask) & (BELOW_MASK | ABOVE_MASK)) {
                raiseWindow(winInfo);
                *mask = winInfo->getMask();
                changed = 1;
            }
        }
        else {
            remove(key, winInfo);
        }
    }
    return changed;
}

/**
 * Compares the current state with the last saved state.
 * The current state is then saved
 * @return 1 iff the state has actually changed
 */
static int compareState() {
    WorkspaceState* currentState = computeState();
    assert(currentState);
    int changed = 0;
    ArrayList<WorkspaceID>workspaceIDs;
    for(Workspace* w : getAllWorkspaces())
        if(w->isVisible())
            workspaceIDs.add(w->getID());
    for(Workspace* w : getAllWorkspaces())
        if(!w->isVisible())
            workspaceIDs.add(w->getID());
    assert(workspaceIDs.size() == getNumberOfWorkspaces());
    for(WorkspaceID i : workspaceIDs) {
        _printStateComparison(currentState, i);
        if(currentState[i].forceRetile || (savedStates || currentState[i].size) &&
            (i >= numberOfRecordedWorkspaces || savedStates[i].size != currentState[i].size ||
                savedStates[i].monitorViewport != currentState[i].monitorViewport ||
                savedStates[i].layout != currentState[i].layout ||
                savedStates[i].size && (
                    memcmp(savedStates[i].windowIds, currentState[i].windowIds, sizeof(WindowID)*savedStates[i].size) ||
                    memcmp(savedStates[i].windowMasks, currentState[i].windowMasks, sizeof(WindowMask)*savedStates[i].size)
                )
            )
        ) {
            if(currentState[i].visible || !currentState[i].forceRetile) {
                changed = 1;
            }
            if(currentState[i].visible) {
                LOG(LOG_LEVEL_DEBUG, "Detected WORKSPACE_WINDOW_CHANGE in %d", i);
                tileWorkspace(getWorkspace(i));
            }
            currentState[i].forceRetile = !currentState[i].visible;
        }
    }
    LOG(LOG_LEVEL_TRACE, "State changed %d", changed);
    if(savedStates)
        destroyCurrentState();
    numberOfRecordedWorkspaces = getNumberOfWorkspaces();
    savedStates = currentState;
    return changed;
}

int updateState() {
    return updateWindowsNotInWorkspaces() | compareState();
}
#endif
