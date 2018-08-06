/**
 * Functions user can bind keys to
 */

#include <assert.h>
#include <unistd.h>

#include "functions.h"
#include "wmfunctions.h"

#define UP 1
#define DOWN -1

#define NO_ACTION_REQUIRED 2
#define NORMAL_EXIT 1
#define ERROR_EXIT 0

void cycleWindows( int delta){
    Master *master = getActiveMaster();
    if(!isFocusStackFrozen())
        grabDevice(master->id,XI_KeyReleaseMask);
    else
        setFocusStackFrozen(1);

    if(! getMasterWindowStack()->next){
        LOG(LOG_LEVEL_INFO,"not enough windows");
        return;
    }
    while(getIntValue(getMasterWindowStack())){

        Node* nextWindowNode=getNextWindowInFocusStack(delta);
        xcb_window_t nextWindow=getIntValue(nextWindowNode);

        assert(nextWindow!=0);

        activateWindow(getValue(nextWindowNode));
        /*
        if(!activateWindowNode(nextWindowNode)){
            printf("window did not exist; trying again");
            delta+=delta/abs(delta);
            continue;
        }
        */
        break;
    }

}
void cycleWindowsForward(){
    cycleWindows(1);
}
void cycleWindowsReverse(){
    cycleWindows(-1);
}


void endCycleWindows(){
    Master*master=getActiveMaster();
    if(isFocusStackFrozen(master)){
        ungrabDevice(master->id);
        setFocusStackFrozen(0);
        onWindowFocus(getFocusedWindow()->id);
    }
}

//////Run or Raise code

int _runOrRaise(Node*targetWindow,Rules*commandRule){
    LOG(LOG_LEVEL_DEBUG,"RUN OR RAISE RESULT: %d %d\n",targetWindow?1:0,commandRule->ruleTarget);
    if(targetWindow){
        assert(getIntValue(targetWindow));
        assert(getValue(targetWindow));
        WindowInfo*info=getValue(targetWindow);
        activateWindow(info);
        updateWindowCache(info);
        return info->id;
    }
    else{
        for(int i=0;commandRule[i].ruleTarget;i++)
            applyRule(commandRule, NULL);
        return 0;
    }
}

Node* findWindow(Rules*rule,Node*searchList,Node*ignoreList){
    UNTIL_FIRST(searchList,
            (!ignoreList||!isInList(ignoreList,getIntValue(searchList)))&&
            isActivatable(getValue(searchList))&&
            doesWindowMatchRule(rule, getValue(searchList)));

    assert(searchList==NULL||doesWindowMatchRule(rule, getValue(searchList)));
    return searchList;
}

int runOrRaiseLazy(Rules*rule,int numberOfRules){
    Node* targetWindow=NULL;

    int i=0;
    for(;!targetWindow && rule[i].ruleTarget;i++)
        targetWindow=findWindow(&rule[i],getAllWindows(),NULL);
    return _runOrRaise(targetWindow, &rule[i]);
}
int runOrRaise(Rules*rule){
    Node*topWindow=getMasterWindowStack(getActiveMaster());
    if(isNotEmpty(topWindow))
        updateWindowCache(getValue(topWindow));

    Node* targetWindow=NULL;
    int i=0;
    while(rule[i].ruleTarget){
        Node *windowsToIgnore = getWindowCache(getActiveMaster());
        targetWindow=findWindow(&rule[i],topWindow,windowsToIgnore);
        if(!targetWindow){
            targetWindow=findWindow(&rule[i],getAllWindows(),windowsToIgnore);
            if(!targetWindow && isNotEmpty(windowsToIgnore)){
                clearWindowCache();
                targetWindow=findWindow(&rule[i],topWindow,NULL);
            }
        }
        if(targetWindow)
            break;
        i++;
    }
    if(targetWindow)
        dumpWindowInfo(getValue(targetWindow));

    dumpWindowInfo(getValue(targetWindow));
    return _runOrRaise(targetWindow, &rule[i+1]);

}

void spawn(char* command){
    LOG(LOG_LEVEL_INFO,"running command %s\n",command);
    int pid=fork();
    if(pid==0){
        setsid();
        execl(SHELL, "-c",command,(char*)0);
    }
    else if(pid<0){
        LOG(LOG_LEVEL_ERROR,"error forking\n");
        exit(1);
    }
}
/*
void spawnAsMaster(char* target){
    //TODO Merge with MPX Patch
    runCommand(target);
}
*/

void killFocusedWindow(){
    killWindow(getFocusedWindow()->id);
}


void activateLastClickedWindow(){
    activateWindow(getWindowInfo(getLastWindowClicked(getActiveMaster())));
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

void popMin(int index){}
void pushMin(int index){}
void sendToNextWorkNonEmptySpace(int index){}
void swapWithNextMonitor(int index){}
void sendToNextMonitor(int index){}

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
void mouseResize(){
}
void mouseDragWindow(){
}


int floatActiveWindow(){
    return floatWindow(getValue(getActiveWindowStack()));
}
int floatWindow(WindowInfo* winInfo){
    if(NO_TILE_MASK & winInfo->id)
        return NO_ACTION_REQUIRED;
    //setMaskForWindow(winInfo, NO_TILE_MASK);

    return NORMAL_EXIT;
}
int sinkActiveWindow(){
    return sinkWindow(getFocusedWindow());
}
int sinkWindow(WindowInfo* winInfo){
    if(NO_TILE_MASK & winInfo->id == 0)
            return NO_ACTION_REQUIRED;
    //setMaskForWindow(winInfo, NO_TILE_MASK);

    return NORMAL_EXIT;
}
void toggleStateForActiveWindow(int mask){
    updateState(getFocusedWindow(), mask, TOGGLE);
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
    setLayout(getValue(getActiveWorkspace()->layouts));
}
void toggleLayout(Layout* layout){
    if(layout==getActiveWorkspace()->activeLayout)
        setLayout(layout);
    else cycleLayouts(0);
}
void setLayout(Layout* layout){
    getActiveWorkspace()->activeLayout=layout;
    applyLayout(getActiveWorkspace(), layout);
}



