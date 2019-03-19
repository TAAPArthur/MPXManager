/**
 * @file state.c
 * @copybrief state.h
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>


#include "globals.h"
#include "logger.h"
#include "monitors.h"
#include "state.h"
#include "windows.h"
#include "wmfunctions.h"
#include "workspaces.h"

/**
 * Holds metadata about all the workspaces
 * This metada will be compared to the current workspaces and if they
 * don't match, we consider the workspaces to have changed
 */
typedef struct {
    ///representing  the view port of the workspaces' monitors
    Rect monitorViewport;
    ///if this struct can be safely freed
    int noFree;
    /// the size of windowIds and windowMasks arrays
    int size;
    ///list of window ids in workspace stack order
    WindowID* windowIds;
    /// list of user masks of windows in workspace stack order
    WindowMask* windowMasks;
    /// the layout of the workspace
    void* layout;
} WorkspaceState;

static int numberOfRecordedWorkspaces;
static WorkspaceState* savedStates;
static char couldStateHaveChanged;

void markState(void){
    LOG(LOG_LEVEL_TRACE, "marking state\n");
    couldStateHaveChanged = 1;
}
void unmarkState(void){
    LOG(LOG_LEVEL_TRACE, "unmarking state\n");
    couldStateHaveChanged = 0;
}

static void destroyCurrentState(){
    for(int i = 0; i < numberOfRecordedWorkspaces; i++){
        if(savedStates[i].noFree)continue;
        free(savedStates[i].windowIds);
        free(savedStates[i].windowMasks);
    }
    free(savedStates);
    savedStates = NULL;
}

static inline Rect getMonitorLocationFromWorkspace(Workspace* workspace){
    assert(workspace);
    Monitor* m = getMonitorFromWorkspace(workspace);
    if(!m)
        return (Rect){0};
    Rect rect;
    memcpy(&rect, &m->view, sizeof(Rect));
    return rect;
}
static WorkspaceState* computeState(){
    WorkspaceState* states = calloc(getNumberOfWorkspaces(), sizeof(WorkspaceState));
    for(int i = 0; i < getNumberOfWorkspaces(); i++){
        if(!isWorkspaceVisible(i) && i < numberOfRecordedWorkspaces){
            memcpy(&states[i], &savedStates[i], sizeof(WorkspaceState));
            savedStates[i].noFree = 1;
            states[i].noFree = 0;
            continue;
        }
        states[i].layout = getActiveLayoutOfWorkspace(i);
        states[i].monitorViewport = getMonitorLocationFromWorkspace(getWorkspaceByIndex(i));
        ArrayList* list = getWindowStack(getWorkspaceByIndex(i));
        int size = getSize(list);
        int j = 0;
        if(size){
            states[i].windowIds = malloc(size * sizeof(int));
            states[i].windowMasks = malloc(size * sizeof(int));
            FOR_EACH(WindowInfo*, winInfo, list){
                if(isTileable(winInfo)){
                    states[i].windowIds[j] = winInfo->id;
                    states[i].windowMasks[j] = hasPartOfMask(winInfo, RETILE_MASKS);
                    j++;
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
static int compareState(void(*onChange)(int)){
    WorkspaceState* currentState = computeState();
    assert(currentState);
    int changed = 0;
    for(int i = 0; i < getNumberOfWorkspaces(); i++)
        if((savedStates || currentState[i].size) &&
                (i >= numberOfRecordedWorkspaces || savedStates[i].size != currentState[i].size ||
                 memcmp(&savedStates[i].monitorViewport, &currentState[i].monitorViewport, sizeof(Rect)) ||
                 savedStates[i].layout != currentState[i].layout ||
                 memcmp(savedStates[i].windowIds, currentState[i].windowIds, sizeof(WindowID)*savedStates[i].size)
                 || memcmp(savedStates[i].windowMasks, currentState[i].windowMasks, sizeof(WindowMask)*savedStates[i].size))
          ){
            changed = 1;
            LOG(LOG_LEVEL_TRACE, "Detected change in workspace %d. \n", i);
            if(onChange)
                onChange(i);
            else
                break;
        }
    unmarkState();
    if(savedStates)
        destroyCurrentState();
    numberOfRecordedWorkspaces = getNumberOfWorkspaces();
    savedStates = currentState;
    return changed;
}



int updateState(void(*onChange)(int)){
    return (!savedStates || couldStateHaveChanged) && compareState(onChange);
}
