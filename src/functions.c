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
    int size=getSize(getMasterWindowStack());
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
    Node*topWindow=getMasterWindowStack(getActiveMaster());
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


void spawn(char* command){
    LOG(LOG_LEVEL_INFO,"running command %s\n",command);
    int pid=fork();
    if(pid==0){
        execl(SHELL,SHELL, "-c",command,(char*)0);
    }
    else if(pid<0)
        err(1, "error forking\n");
}

Node* getNextWindowInStack(int dir){
    Node*activeWindows=getActiveWindowStack();
    Node*node=NULL;
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
        if(isActivatable(getValue(node)))
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
    setActiveLayout(getValue(n));
    retile();
}
void toggleLayout(Layout* layout){
    if(layout!=getActiveLayout()){
        setActiveLayout(layout);
        retile();
    }
    else cycleLayouts(0);
}
void retile(void){
    tileWorkspace(getActiveWorkspaceIndex());
}



void setLastKnowMasterPosition(int x,int y,int relativeX,int relativeY){
    getActiveMaster()->mousePos[0]=x;
    getActiveMaster()->mousePos[1]=y;
    getActiveMaster()->relativeMousePos[0]=relativeX;
    getActiveMaster()->relativeMousePos[1]=relativeY;

}
void setRefefrencePointForMaster(int x,int y,int relativeX,int relativeY){
    getActiveMaster()->refPoint[0]=x;
    getActiveMaster()->refPoint[1]=y;
    getActiveMaster()->relativeRefPoint[0]=relativeX;
    getActiveMaster()->relativeRefPoint[1]=relativeY;
}


static short*addArr(short*result,short*arr1,short*arr2,int size,char sign){
    for(int i=0;i<size;i++)
        result[i]=arr1[i]+arr2[i]*sign;
    return result;
}

void resizeWindowWithMouse(WindowInfo*winInfo){
    assert(winInfo);
    short values[2];
    addArr(values,getActiveMaster()->mousePos,getActiveMaster()->refPoint,2,-1);
    addArr(values,&getConfig(winInfo)[2],values,2,1);
    processConfigureRequest(winInfo->id, values, 0, 0, XCB_CONFIG_WINDOW_WIDTH|XCB_CONFIG_WINDOW_HEIGHT);
}
void moveWindowWithMouse(WindowInfo*winInfo){
    assert(winInfo);
    short values[2];
    addArr(values,getActiveMaster()->mousePos,getActiveMaster()->relativeRefPoint,2,-1);
    processConfigureRequest(winInfo->id, values, 0, 0, XCB_CONFIG_WINDOW_X|XCB_CONFIG_WINDOW_Y);
}

int focusActiveWindow(WindowInfo *winInfo){
    return winInfo && focusWindowInfo(winInfo);
}
void killFocusedWindow(void){
    if(getFocusedWindow())
        killWindowInfo(getFocusedWindow());
}
