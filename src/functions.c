/**
 * @file functions.c
 * Functions user can bind keys to
 */
/// \cond
#include <assert.h>
#include <unistd.h>
#include <err.h>
/// \endcond
#include "functions.h"
#include "wmfunctions.h"
#include "mywm-util.h"
#include "logger.h"
#include "bindings.h"
#include "globals.h"

#define UP 1
#define DOWN -1

void cycleWindows( int delta){
    setFocusStackFrozen(1);
    int size=getSize(getMasterWindowStack());
    for(int i=0;i<size;i++){
        Node* nextWindowNode=getNextWindowInFocusStack(delta);
        xcb_window_t nextWindow=getIntValue(nextWindowNode);
        assert(nextWindow!=0);
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
int processFindResult(Node*target){
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
        setsid();
        execl(SHELL,SHELL, "-c",command,(char*)0);
    }
    else if(pid<0)
        err(1, "error forking\n");
}

void killFocusedWindow(){
    killWindow(getFocusedWindow()->id);
}


void focusBottom(){
    activateWindow(getValue(getLast(getActiveWindowStack())));
}
void focusTop(){
    focusWindow(getIntValue(getMasterWindowStack()));
}
void shiftTop(){
    shiftToHead(getActiveWindowStack(), getNextWindowInStack(0));
}
void swapWithTop(){
    swap(getActiveWindowStack(),getNextWindowInStack(0));
}
void shiftPosition(int dir){
    swap(getActiveWindowStack(),getNextWindowInStack(dir));
}
void shiftFocus(int dir){
    focusWindow(getIntValue(getNextWindowInStack(dir)));
}

/*
void swapWithNextMonitor(int index){
    swapMonitors(getActiveWorkspaceIndex(), getActiveWorkspaceIndex()+index);
}
void sendToNextMonitor(int index){

}*/

void moveToWorkspace(int index){
    activateWorkspace(index);
}
void sendToWorkspace(int index){
    moveWindowToWorkspace(getFocusedWindow(), index);
}
void cloneToWorkspace(int index){

}
void moveToNextWorkspace(int dir){
    switchToWorkspace((getActiveWorkspaceIndex()+dir)%getNumberOfWorkspaces());
}


void floatFocusedWindow(){
    floatWindow(getFocusedWindow());
}
void sinkFocusedWindow(){
    sinkWindow(getFocusedWindow());
}


void toggleStateForActiveWindow(int mask){
    toggleMask(getFocusedWindow(), mask);
}


int sendWindowToWorkspaceByName(char*name,int window){
    //TODO implement
    return 0;
}

void cycleLayouts(int dir){
    for(int i=0;i<abs(dir);i++)
        if(dir>0)
            getActiveWorkspace()->layouts=getActiveWorkspace()->layouts->next;
        else
            getActiveWorkspace()->layouts=getActiveWorkspace()->layouts->prev;
    setActiveLayout(getValue(getActiveWorkspace()->layouts));
}
void toggleLayout(Layout* layout){
    if(layout==getActiveWorkspace()->activeLayout)
        setActiveLayout(layout);
    else cycleLayouts(0);
}
void retile(){
    tileWorkspace(getActiveWorkspaceIndex());
}

