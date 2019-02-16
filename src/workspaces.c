#include <assert.h>
#include <stdlib.h>
#include "mywm-util.h"
#include "workspaces.h"
#include "masters.h"
///list of all workspaces
static ArrayList workspaces;

ArrayList*getListOfWorkspaces(void){return &workspaces;}

Workspace* getWorkspaceByIndex(int index){
    assert(index>=0);
    assert(index<getNumberOfWorkspaces());
    return getElement(getListOfWorkspaces(),index);
}
ArrayList*getWindowStack(Workspace*workspace){
    return &workspace->windows;
}
Workspace*createWorkspace(){
    Workspace *workspaces=calloc(1,sizeof(Workspace));
    workspaces->id=getNumberOfWorkspaces();
    return workspaces;
}
int getNumberOfWorkspaces(){
    return getSize(&workspaces);
}
int getWorkspaceIndexOfWindow(WindowInfo*winInfo){
    return winInfo->workspaceIndex;
}
Workspace* getWorkspaceOfWindow(WindowInfo*winInfo){
    return getWorkspaceByIndex(getWorkspaceIndexOfWindow(winInfo));
}
ArrayList* getWindowStackOfWindow(WindowInfo*winInfo){
   return getWindowStack(getWorkspaceOfWindow(winInfo));
}
void setActiveLayout(Layout*layout){
    getActiveWorkspace()->activeLayout=layout;
}

Layout* getActiveLayout(){
    return getActiveLayoutOfWorkspace(getActiveWorkspaceIndex());
}
Layout* getActiveLayoutOfWorkspace(int workspaceIndex){
    return getWorkspaceByIndex(workspaceIndex)->activeLayout;
}
ArrayList* getLayouts(Workspace*workspace){
    return &workspace->layouts;
}
//workspace methods
int isWorkspaceVisible(int index){
    return getWorkspaceByIndex(index)->monitor?1:0;
}
int isWorkspaceNotEmpty(int index){
    return isNotEmpty(getWindowStack(getWorkspaceByIndex(index)));
}

Workspace*getNextWorkspace(int dir,int mask){
    int empty = ((mask>>2) & 3) -1;
    int hidden = (mask & 3) -1;
    int index = getActiveWorkspaceIndex();

    for(int i=0;i<getNumberOfWorkspaces();i++){
        index =(index+dir) %getNumberOfWorkspaces();
        if(index<0)
            index+=getNumberOfWorkspaces();
        if( (hidden ==-1 || hidden == !isWorkspaceVisible(index)) &&
            (empty ==-1 || empty == !isWorkspaceNotEmpty(index))
            )
            return getWorkspaceByIndex(index);

    }
    return NULL;
}

Workspace*getWorkspaceFromMonitor(Monitor*monitor){
    assert(monitor);
    for(int i=0;i<getNumberOfWorkspaces();i++)
        if(getWorkspaceByIndex(i)->monitor==monitor)
            return getWorkspaceByIndex(i);
    return NULL;
}
Monitor*getMonitorFromWorkspace(Workspace*workspace){
    assert(workspace);
    return workspace->monitor;
}
void swapMonitors(int index1,int index2){
    Monitor*temp=getWorkspaceByIndex(index1)->monitor;
    getWorkspaceByIndex(index1)->monitor=getWorkspaceByIndex(index2)->monitor;
    getWorkspaceByIndex(index2)->monitor=temp;
}
//Workspace methods

int isWindowInVisibleWorkspace(WindowInfo* winInfo){
    return isWorkspaceVisible(getWorkspaceIndexOfWindow(winInfo));
}
int removeWindowFromWorkspace(WindowInfo* winInfo){
    assert(winInfo);
    ArrayList*list=getWindowStackOfWindow(winInfo);
    int index=indexOf(list,winInfo,sizeof(int));
    if(index!=-1)
        removeFromList(list,index);
    return index!=-1;
}
int addWindowToWorkspace(WindowInfo*winInfo,int workspaceIndex){
    assert(workspaceIndex>=0);
    assert(workspaceIndex<getNumberOfWorkspaces());
    Workspace*workspace=getWorkspaceByIndex(workspaceIndex);
    assert(winInfo!=NULL);
    
    if(find(getWindowStack(workspace),winInfo,sizeof(int))==NULL){
        addToList(getWindowStack(workspace), winInfo);
        winInfo->workspaceIndex=workspaceIndex;
        return 1;
    }
    return 0;
}

void setActiveWorkspaceIndex(int index){
    getActiveMaster()->activeWorkspaceIndex=index;
}
int getActiveWorkspaceIndex(void){
    return getActiveMaster()->activeWorkspaceIndex;
}
Workspace* getActiveWorkspace(void){
    return getWorkspaceByIndex(getActiveWorkspaceIndex());
}

ArrayList*getActiveWindowStack(void){
    return getWindowStack(getActiveWorkspace());
}
