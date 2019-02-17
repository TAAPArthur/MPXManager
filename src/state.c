/**
 * @file state.c
 * Tracks state of workspaces
 */
/// \cond
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"
/// \endcond

#include "workspaces.h"
#include "monitors.h"
#include "windows.h"
#include "wmfunctions.h"
#include "logger.h"

/**
 * Holds metadata about all the workspaces
 * This metada will be compared to the current workspaces and if they 
 * don't match, we consider the workspaces to have changed
 */
typedef struct {
    /**a char array representing  the x,y,width, height of the last 
    * monitor for a workspace
    */
    long monitor;
    ///if this struct can be safely freed
    int noFree;
    /// the size of windowIds and windowMasks arrays
    int size;
    ///list of window ids in workspace stack order
    int *windowIds;
    /// list of user masks of windows in workspace stack order
    int *windowMasks;
    /// the layout of the workspace
    void* layout;
}WorkspaceState;

static int numberOfRecordedWorkspaces;
static WorkspaceState*savedStates;
static char couldStateHaveChanged;

void markState(void){
    LOG(LOG_LEVEL_TRACE,"marking state\n");
    couldStateHaveChanged=1;
}
void unmarkState(void){
    LOG(LOG_LEVEL_TRACE,"marking state\n");
    couldStateHaveChanged=0;
}

static void destroyCurrentState(){
    for(int i=0;i<numberOfRecordedWorkspaces;i++){
        if(savedStates[i].noFree)continue;
        free(savedStates[i].windowIds);
        free(savedStates[i].windowMasks);
    }
    free(savedStates);
    savedStates=NULL;
}

static long getMonitorLocationFromWorkspace(Workspace*workspace){
    assert(workspace);
    Monitor *m=getMonitorFromWorkspace(workspace);
    if(!m)return 0;
    long l;
    //copy the next 4 shorts after m->view.x into l
    memcpy(&l,&m->view.x,sizeof(long));
    return l;
}
static WorkspaceState* computeState(){
    WorkspaceState* states=calloc(getNumberOfWorkspaces(), sizeof(WorkspaceState));
    for(int i=0;i<getNumberOfWorkspaces();i++){
        if(!isWorkspaceVisible(i)&&i<numberOfRecordedWorkspaces){
            memcpy(&states[i],&savedStates[i],sizeof(WorkspaceState));
            savedStates[i].noFree=1;
            states[i].noFree=0;
            continue;
        }
        states[i].layout=getActiveLayoutOfWorkspace(i);
        long m=getMonitorLocationFromWorkspace(getWorkspaceByIndex(i));
        states[i].monitor=m?m:i<numberOfRecordedWorkspaces?savedStates[i].monitor:0;
        ArrayList*list=getWindowStack(getWorkspaceByIndex(i));
        int size=getSize(list);
        int j=0;
        if(size){
            states[i].windowIds=malloc(size*sizeof(int));
            states[i].windowMasks=malloc(size*sizeof(int));
            FOR_EACH(WindowInfo*winInfo,list,
                
                if(isTileable(winInfo)){
                    states[i].windowIds[j]=winInfo->id;
                    states[i].windowMasks[j]=getUserMask(winInfo);
                    j++;
                }
            )
        }
        states[i].size=j;
    }
    return states;
}


/**
 * Compares the current state with the last saved state.
 * The current state is then saved
 * @return 1 iff the state has actually changed
 */
static int compareState(void(*onChange)(int)){

    WorkspaceState* currentState=computeState();
    assert(currentState);
    int changed=0;
    for(int i=0;i<getNumberOfWorkspaces();i++)
        if(
            i>=numberOfRecordedWorkspaces||savedStates[i].size != currentState[i].size||
            savedStates[i].monitor!=currentState[i].monitor||
            savedStates[i].layout!=currentState[i].layout||
            memcmp(savedStates[i].windowIds, currentState[i].windowIds, sizeof(int)*savedStates[i].size)
            ||memcmp(savedStates[i].windowMasks, currentState[i].windowMasks, sizeof(int)*savedStates[i].size)
            ){
                unmarkState();
                changed=1;
                LOG(LOG_LEVEL_TRACE,"Detected change in workspace %d. \n",i);
                if(onChange)
                    onChange(i);
                else
                    break;
        }

    if(savedStates)
        destroyCurrentState();
    numberOfRecordedWorkspaces=getNumberOfWorkspaces();
    savedStates=currentState;

    return changed;
}



int updateState(void(*onChange)(int)){
    return (!savedStates || couldStateHaveChanged) && compareState(onChange);
}
