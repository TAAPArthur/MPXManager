/**
 * @file functions.c
 * Functions user can bind keys to
 */
/// \cond
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
/// \endcond
#include "functions.h"
#include "wmfunctions.h"
#include "mywm-util.h"
#include "logger.h"
#include "bindings.h"
#include "globals.h"

void cycleWindows( int delta){
    setFocusStackFrozen(1);
    int size=getSize(getActiveMasterWindowStack());
    for(int i=0;i<size;i++){
        Node* nextWindowNode=getNextWindowInFocusStack(delta);
        if(activateWindow(getValue(nextWindowNode)))
            break;
    }
}

void endCycleWindows(void){
    setFocusStackFrozen(0);
}

//////Run or Raise code


Node* findWindow(Rule*rule,Node*searchList,Node*ignoreList){
    UNTIL_FIRST(searchList,
            (!ignoreList||!isInList(ignoreList,getIntValue(searchList)))&&
            isActivatable(getValue(searchList))&&
            doesWindowMatchRule(rule, getValue(searchList)));

    assert(searchList==NULL||doesWindowMatchRule(rule, getValue(searchList)));
    return searchList;
}
static int processFindResult(Node*target){
    if(target){
        WindowInfo*winInfo=getValue(target);
        activateWindow(winInfo);
        updateWindowCache(winInfo);
        return winInfo->id;
    }
    else return 0;
}
int findAndRaiseLazy(Rule*rule){
    return processFindResult(findWindow(rule,getAllWindows(),NULL));
}
int findAndRaise(Rule*rule){
    Node*topWindow=getActiveMasterWindowStack();
    if(isNotEmpty(topWindow))
        updateWindowCache(getValue(topWindow));

    Node* target=NULL;
    Node *windowsToIgnore = getWindowCache(getActiveMaster());
    target=findWindow(rule,topWindow,windowsToIgnore);
    if(!target){
        target=findWindow(rule,getAllWindows(),windowsToIgnore);
        if(!target && isNotEmpty(windowsToIgnore)){
            clearWindowCache();
            target=findWindow(rule,topWindow,NULL);
        }
    }
    return processFindResult(target);
}


void spawnPipe(char*command){
    LOG(LOG_LEVEL_INFO,"running command %s\n",command);

    if(statusPipeFD[0]){
        close(statusPipeFD[0]);
        close(statusPipeFD[1]);
    }
    pipe(statusPipeFD);
    int pid=fork();
    if(pid==0){
        close(statusPipeFD[1]);
        dup2(statusPipeFD[0],0);
        execl(SHELL,SHELL, "-c",command,(char*)0);
    }
    else if(pid<0)
        err(1, "error forking\n");
    dup2(statusPipeFD[1],1);
    close(statusPipeFD[0]);
}

void spawn(char* command){
    LOG(LOG_LEVEL_INFO,"running command %s\n",command);
    int pid=fork();
    if(pid==0){
        setsid();
        execl(SHELL,SHELL, "-c",command,(char*)0);
    }
    else if(pid<0)
        err(1, "error forking\n");
}

Node* getNextWindowInStack(int dir){
    Node*activeWindows=getActiveWindowStack();
    Node*node=NULL;
    if(getFocusedWindow())
        node=isInList(activeWindows, getFocusedWindow()->id);

    if(!node)
        node=activeWindows;


    if(dir==0)
        return node;
    Node*starting=node;
    do{
        if(dir>0)
            node=node->next?node->next:activeWindows;
        else
            node=node->prev?node->prev:getLast(activeWindows);
        if(isInteractable(getValue(node)))
            break;
    }while(node!=starting);
    return node;
}

int focusBottom(void){
    return activateWindow(getValue(getLast(getActiveWindowStack())));
}
int focusTop(void){
    return activateWindow(getValue(getActiveWindowStack()));
}
void shiftTop(void){
    shiftToHead(getActiveWindowStack(), getNextWindowInStack(0));
}
void swapWithTop(void){
    swap(getActiveWindowStack(),getNextWindowInStack(0));
}
void swapPosition(int dir){
    swap(getNextWindowInStack(0),getNextWindowInStack(dir));
}
int shiftFocus(int dir){
    return activateWindow(getValue(getNextWindowInStack(dir)));
}

/*
void swapWithNextMonitor(int index){
    swapMonitors(getActiveWorkspaceIndex(), getActiveWorkspaceIndex()+index);
}
void sendToNextMonitor(int index){

}*/

void sendWindowToWorkspaceByName(WindowInfo*winInfo,char*name){
    moveWindowToWorkspace(winInfo, getIndexFromName(name));
}

void cycleLayouts(int dir){
    Node*n=getActiveWorkspace()->layouts;
    if(dir>=0)
        FOR_AT_MOST(n,dir)
    else
        FOR_AT_MOST_REVERSED(n,-dir)

    getActiveWorkspace()->layouts=n;
    if(setActiveLayout(getValue(n)))
        retile();
}
void toggleLayout(Layout* layout){
    if(layout!=getActiveLayout()){
        if(setActiveLayout(layout))
            retile();
    }
    else cycleLayouts(0);
}
void retile(void){
    tileWorkspace(getActiveWorkspaceIndex());
}



void setLastKnowMasterPosition(int x,int y){
    memcpy(getActiveMaster()->prevMousePos,getActiveMaster()->currentMousePos,sizeof(getActiveMaster()->prevMousePos));
    getActiveMaster()->currentMousePos[0]=x;
    getActiveMaster()->currentMousePos[1]=y;
}
void getMouseDelta(Master*master,short result[2]){
    for(int i=0;i<2;i++)
        result[i]=master->currentMousePos[i]-master->prevMousePos[i];
}

void resizeWindowWithMouse(WindowInfo*winInfo){
    assert(winInfo);
    short values[4];
    short delta[2];
    getMouseDelta(getActiveMaster(),delta); 
    for(int i=0;i<4;i++)
        values[i]=getGeometry(winInfo)[i]+(i>=2)*delta[i-2];
    for(int i=0;i<2;i++)
        if(values[i+2]<0){
            values[i+2]=-values[i];
            values[i]+=delta[i];
        }
    processConfigureRequest(winInfo->id, values, 0, 0, XCB_CONFIG_WINDOW_X|XCB_CONFIG_WINDOW_Y|XCB_CONFIG_WINDOW_WIDTH|XCB_CONFIG_WINDOW_HEIGHT);
    //TODO check success
    for(int i=0;i<4;i++)
        getGeometry(winInfo)[i]=values[i];
}
void moveWindowWithMouse(WindowInfo*winInfo){
    assert(winInfo);
    short values[2];
    short delta[2];
    getMouseDelta(getActiveMaster(),delta); 
    for(int i=0;i<2;i++)
        values[i]=getGeometry(winInfo)[i]+delta[i];
    
    processConfigureRequest(winInfo->id, values, 0, 0, XCB_CONFIG_WINDOW_X|XCB_CONFIG_WINDOW_Y);
    //TODO check success
    getGeometry(winInfo)[0]=values[0];
    getGeometry(winInfo)[1]=values[1];
}

int focusActiveWindow(WindowInfo *winInfo){
    return winInfo && focusWindowInfo(winInfo);
}
void killFocusedWindow(void){
    if(getFocusedWindow())
        killWindowInfo(getFocusedWindow());
}
void floatWindow(WindowInfo* winInfo){
    addMask(winInfo, FLOATING_MASK);
    moveWindowToLayer(winInfo,UPPER_LAYER);

}
void sinkWindow(WindowInfo* winInfo){
    removeMask(winInfo, FLOATING_MASK);
    moveWindowToLayer(winInfo,NORMAL_LAYER);
}
void setMasterTarget(int id){
    getActiveMaster()->targetWindow=id;
}
void startMouseOperation(WindowInfo*winInfo){
    setMasterTarget(winInfo->id);
    lockWindow(winInfo);
}
void stopMouseOperation(WindowInfo*winInfo){
    setMasterTarget(0);
    unlockWindow(winInfo);
}
