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
#include "logger.h"


typedef struct {
    int index;
    int monitor;
    int size;
    int *windowIds;
    int *windowMasks;
    void* layout;
}WorkspaceState;
typedef struct{
    int numberOfVisibleWorkspaces;
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
    for(int i=0;i<savedState->numberOfVisibleWorkspaces;i++)
        if(savedState->state[i].size){
            free(savedState->state[i].windowIds);
            free(savedState->state[i].windowMasks);
        }

    free(savedState->state);
    free(savedState);
    savedState=NULL;
}

State* computeState(){
    State* gloalState=malloc(sizeof(State));
    int numberOfActiveStacks=getSize(getAllMonitors());
    WorkspaceState* states=calloc(numberOfActiveStacks, sizeof(WorkspaceState));
    gloalState->numberOfVisibleWorkspaces=numberOfActiveStacks;
    gloalState->state=states;
    for(int n=0,i=0;i<getNumberOfWorkspaces();i++)
        if(isWorkspaceVisible(i)){
            assert(n<numberOfActiveStacks);
            states[n].index=i;
            states[n].layout=getActiveLayoutOfWorkspace(i);
            states[n].monitor=getMonitorFromWorkspace(getWorkspaceByIndex(i))->name;
            Node*node=getWindowStack(getWorkspaceByIndex(i));
            int size=getSize(node);
            states[n].size=size;
            if(size){
                states[n].windowIds=malloc(size*sizeof(int));
                states[n].windowMasks=malloc(size*sizeof(int));
                int j=0;
                FOR_EACH(node,
                    WindowInfo*winInfo=getValue(node);
                    states[n].windowIds[j]=winInfo->id;
                    states[n].windowMasks[j]=winInfo->mask;
                    j++;
                )
            }
            n++;

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
    if(!savedState || savedState->numberOfVisibleWorkspaces==currentState->numberOfVisibleWorkspaces)
        for(int i=0;i<currentState->numberOfVisibleWorkspaces;i++)
            if(
                !savedState||savedState->state[i].size != currentState->state[i].size||
                savedState->state[i].index!=currentState->state[i].index||
                savedState->state[i].monitor!=currentState->state[i].monitor||
                savedState->state[i].layout!=currentState->state[i].layout||
                memcmp(savedState->state[i].windowIds, currentState->state[i].windowIds, sizeof(int)*savedState->state[i].size)
                ||memcmp(savedState->state[i].windowMasks, currentState->state[i].windowMasks, sizeof(int)*savedState->state[i].size)
                ){
                    LOG(LOG_LEVEL_TRACE,"index changed: %d %d\n\n",savedState?savedState->state[i].index:-1,currentState->state[i].index);
                    couldStateHaveChanged=0;
                    changed=1;
                    if(onChange)
                        onChange(currentState->state[i].index);
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
