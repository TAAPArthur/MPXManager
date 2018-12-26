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

#include "mywm-util.h"
#include "wmfunctions.h"
#include "logger.h"


typedef struct {
    long monitor;
    int noFree;
    int size;
    int *windowIds;
    int *windowMasks;
    void* layout;
}WorkspaceState;
typedef struct{
    int numberOfWorkspaces;
    WorkspaceState*state;
}State;
static State*savedState;
static char couldStateHaveChanged;

void markState(void){
    LOG(LOG_LEVEL_TRACE,"marking state\n");
    couldStateHaveChanged=1;
}

void destroyCurrentState(){
    if(!savedState)
        return;
    for(int i=0;i<savedState->numberOfWorkspaces;i++){
        if(savedState->state[i].noFree)continue;
        free(savedState->state[i].windowIds);
        free(savedState->state[i].windowMasks);
    }

    free(savedState->state);
    free(savedState);
    savedState=NULL;
}

static long getMonitorLocationFromWorkspace(Workspace*workspace){
    assert(workspace);
    Monitor *m=getMonitorFromWorkspace(workspace);
    if(!m)return 0;
    long l;
    memcpy(&l,&m->viewX,8);
    return l;
}
State* computeState(){
    State* gloalState=malloc(sizeof(State));
    WorkspaceState* states=calloc(getNumberOfWorkspaces(), sizeof(WorkspaceState));
    gloalState->numberOfWorkspaces=getNumberOfWorkspaces();
    gloalState->state=states;
    for(int i=0;i<getNumberOfWorkspaces();i++){
        if(!isWorkspaceVisible(i)&&savedState){
            memcpy(&states[i],&savedState->state[i],sizeof(WorkspaceState));
            savedState->state[i].noFree=1;
            states[i].noFree=0;
            continue;
        }
        states[i].layout=getActiveLayoutOfWorkspace(i);
        long m=getMonitorLocationFromWorkspace(getWorkspaceByIndex(i));
        states[i].monitor=m?m:savedState?savedState->state[i].monitor:0;
        Node*node=getWindowStack(getWorkspaceByIndex(i));
        int size=getSize(node);
        int j=0;
        if(size){
            states[i].windowIds=malloc(size*sizeof(int));
            states[i].windowMasks=malloc(size*sizeof(int));
            FOR_EACH(node,
                WindowInfo*winInfo=getValue(node);
                if(isTileable(winInfo)){
                    states[i].windowIds[j]=winInfo->id;
                    states[i].windowMasks[j]=getUserMask(winInfo);
                    j++;
                }
            )
        }
        states[i].size=j;
    }
    return gloalState;
}


/**
 * Compares the current state with the last saved state.
 * The current state is then saved
 * @return 1 iff the state has actually changed
 */
int compareState(void(*onChange)(int)){

    State* currentState=computeState();
    assert(currentState);
    int changed=0;
    for(int i=0;i<currentState->numberOfWorkspaces;i++)
        if(
            !savedState||savedState->state[i].size != currentState->state[i].size||
            savedState->state[i].monitor!=currentState->state[i].monitor||
            savedState->state[i].layout!=currentState->state[i].layout||
            memcmp(savedState->state[i].windowIds, currentState->state[i].windowIds, sizeof(int)*savedState->state[i].size)
            ||memcmp(savedState->state[i].windowMasks, currentState->state[i].windowMasks, sizeof(int)*savedState->state[i].size)
            ){
                couldStateHaveChanged=0;
                changed=1;
                LOG(LOG_LEVEL_TRACE,"Detected change in workspace %d. \n",i);
                if(onChange)
                    onChange(i);
                else
                    break;
        }

    destroyCurrentState();
    savedState=currentState;

    return changed;
}



int updateState(void(*onChange)(int)){
    return (!savedState || couldStateHaveChanged) && compareState(onChange);
}
