#include <assert.h>
#include <stdlib.h>
#include <string.h>


#include "logger.h"
#include "monitors.h"
#include "state.h"
#include "layouts.h"
#include "windows.h"
#include "workspaces.h"
#include "wmfunctions.h"
#include "user-events.h"
#include "bindings.h"

/**
 * Holds metadata about all the workspaces
 * This metadata will be compared to the current workspaces and if they
 * don't match, we consider the workspaces to have changed
 */
struct WorkspaceState {
    ///representing  the view port of the workspaces' monitors
    Rect monitorViewport = {0, 0, 0, 0};
    /// if the workspace is known to be visible
    bool visible = 0;
    /// the size of windowIds and windowMasks arrays
    int size = 0;
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
} ;
std::ostream& operator<<(std::ostream& strm, const WorkspaceState& state) {
    strm << "{" << state.monitorViewport << " Visible:" << state.visible << " Size:" << state.size;
    for(int i = 0; i < state.size; i++)
        strm << " (" << state.windowIds[i] << " " << state.windowMasks[i]  << ")";
    return strm << " }";
}

static WorkspaceID numberOfRecordedWorkspaces;
static WorkspaceState* savedStates;
static char couldStateHaveChanged = 1;

bool isStateMarked(void) { return couldStateHaveChanged;}
void markState(void) {
    LOG(LOG_LEVEL_TRACE, "marking state\n");
    couldStateHaveChanged = 1;
}
void unmarkState(void) {
    LOG(LOG_LEVEL_TRACE, "unmarking state\n");
    couldStateHaveChanged = 0;
}

static void destroyCurrentState() {
    delete[] savedStates;
    savedStates = NULL;
}

static inline Rect getMonitorLocationFromWorkspace(Workspace* workspace) {
    assert(workspace);
    Monitor* m = workspace->getMonitor();
    if(!m)
        return (Rect) {0};
    return m->getViewport();
}
static WorkspaceState* computeState() {
    WorkspaceState* states = new WorkspaceState[getNumberOfWorkspaces()];
    for(WorkspaceID i = 0; i < getNumberOfWorkspaces(); i++) {
        Workspace* w = getWorkspace(i);
        if(!w->isVisible() && i < numberOfRecordedWorkspaces)
            states[i].monitorViewport = savedStates[i].monitorViewport;
        else
            states[i].monitorViewport = w->getMonitor() ? w->getMonitor()->getViewport() : (Rect) {0, 0, 0, 0};
        states[i].visible = w->isVisible();
        states[i].layout = w->getActiveLayout();
        int size = w->getWindowStack().size();
        int j = 0;
        if(size) {
            states[i].windowIds = new WindowID[size];
            states[i].windowMasks = new WindowMask[size];
            for(WindowInfo* winInfo : w->getWindowStack()) {
                if(winInfo->isTileable()) {
                    states[i].windowIds[j] = winInfo->getEffectiveID();
                    states[i].windowMasks[j] = winInfo->hasPartOfMask(RETILE_MASKS);
                    j++;
                    if(j == size)
                        break;
                }
            }
        }
        states[i].size = j;
    }
    return states;
}


/**
 * Compares the current state with the last saved state.
 * The current state is then saved
 * @return 1 iff the state has actually changed
 */
static int compareState() {
    WorkspaceState* currentState = computeState();
    assert(currentState);
    int changed = NO_CHANGE;
    for(WorkspaceID i = 0; i < getNumberOfWorkspaces(); i++) {
        LOG_RUN(LOG_LEVEL_VERBOSE,
            std::cout << "Index: " << i << "\n";
            std::cout <<    "Current:" << currentState[i] << "\n";
            if(i < numberOfRecordedWorkspaces)
            std::cout << "Saved:  " << savedStates[i] << "\n";
        );
        if((savedStates || currentState[i].size) &&
            (i >= numberOfRecordedWorkspaces || savedStates[i].size != currentState[i].size ||
                memcmp(&savedStates[i].monitorViewport, &currentState[i].monitorViewport, sizeof(Rect)) ||
                savedStates[i].layout != currentState[i].layout ||
                savedStates[i].size && (
                    memcmp(savedStates[i].windowIds, currentState[i].windowIds, sizeof(WindowID)*savedStates[i].size) ||
                    memcmp(savedStates[i].windowMasks, currentState[i].windowMasks, sizeof(WindowMask)*savedStates[i].size)
                )
            )
        ) {
            changed |= WORKSPACE_WINDOW_CHANGE;
            LOG(LOG_LEVEL_DEBUG, "Detected WORKSPACE_WINDOW_CHANGE in %d\n", i);
            tileWorkspace(i);
        }
        if((savedStates || currentState[i].visible) &&
            (i >= numberOfRecordedWorkspaces ||
                savedStates[i].visible != currentState[i].visible)) {
            changed |= WORKSPACE_MONITOR_CHANGE;
            LOG(LOG_LEVEL_DEBUG, "Detected WORKSPACE_MONITOR_CHANGE %d\n", i);
            syncMappedState(i);
        }
        else {
            Workspace* w = getWorkspace(i);
            for(WindowInfo* winInfo : w->getWindowStack())
                if(winInfo->hasMask(MAPPABLE_MASK) && (w->isVisible() ^ winInfo->hasMask(MAPPED_MASK))) {
                    LOG(LOG_LEVEL_DEBUG, "Detected WINDOW_CHANGE Workspace: %d (Visible %d) for %d \n", i, w->isVisible(),
                        winInfo->getID());
                    updateWindowWorkspaceState(winInfo);
                    changed |= WINDOW_CHANGE ;
                }
        }
    }
    LOG(LOG_LEVEL_TRACE, "State changed %d\n", changed);
    unmarkState();
    if(savedStates)
        destroyCurrentState();
    numberOfRecordedWorkspaces = getNumberOfWorkspaces();
    savedStates = currentState;
    return changed;
}



int updateState() {
    if(!isStateMarked())
        LOG(LOG_LEVEL_TRACE, "State could not have changed \n");
    return isStateMarked() ? (StateChangeType) compareState() : NO_CHANGE;
}
