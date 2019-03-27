/**
 * @file workspaces.c
 * @copybrief workspaces.h
 *
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "globals.h"
#include "masters.h"
#include "mywm-util.h"
#include "workspaces.h"
#include "workspaces.h"
///list of all workspaces
static ArrayList workspaces;

/**
 * @return newly created workspace
 */
static Workspace* createWorkspace(void){
    Workspace* workspaces = calloc(1, sizeof(Workspace));
    workspaces->id = getNumberOfWorkspaces();
    sprintf(workspaces->name, "%d", workspaces->id + 1);
    return workspaces;
}
static void freeWorkspace(Workspace* w){
    clearList(getWindowStack(w));
    clearList(getLayouts(w));
    clearList(&w->layouts);
    free(w);
}
void resetWorkspaces(void){
    for(int i = -getOffset(&workspaces); i < getNumberOfWorkspaces(); i++)
        freeWorkspace(getWorkspaceByIndex(i));
    clearList(&workspaces);
    setOffset(&workspaces, DEFAULT_NUMBER_OF_HIDDEN_WORKSPACES);
    for(int i = -1; i >= -DEFAULT_NUMBER_OF_HIDDEN_WORKSPACES; i--)
        setElement(&workspaces, i, createWorkspace());
}

void addWorkspaces(int num){
    for(int i = 0; i < num; i++)
        addToList(&workspaces, createWorkspace());
}
void removeWorkspaces(int num){
    for(int i = 0; i < num && getNumberOfWorkspaces() > 1; i++)
        freeWorkspace(pop(&workspaces));
}

Workspace* getWorkspaceByIndex(int index){
    return getElement(&workspaces, index);
}
ArrayList* getWindowStack(Workspace* workspace){
    return &workspace->windows;
}
int getNumberOfWorkspaces(){
    return getSize(&workspaces);
}
int getWorkspaceIndexOfWindow(WindowInfo* winInfo){
    return winInfo->workspaceIndex;
}
Workspace* getWorkspaceOfWindow(WindowInfo* winInfo){
    int index = getWorkspaceIndexOfWindow(winInfo);
    if(index == NO_WORKSPACE)
        return NULL;
    return getWorkspaceByIndex(index);
}
ArrayList* getWindowStackOfWindow(WindowInfo* winInfo){
    return getWindowStack(getWorkspaceOfWindow(winInfo));
}
void setActiveLayout(Layout* layout){
    getActiveWorkspace()->activeLayout = layout;
}

Layout* getActiveLayout(){
    return getActiveLayoutOfWorkspace(getActiveWorkspaceIndex());
}
Layout* getActiveLayoutOfWorkspace(int workspaceIndex){
    return getWorkspaceByIndex(workspaceIndex)->activeLayout;
}
ArrayList* getLayouts(Workspace* workspace){
    return &workspace->layouts;
}
//workspace methods
int isWorkspaceVisible(int index){
    return getMonitorFromWorkspace(getWorkspaceByIndex(index)) ? 1 : 0;
}
int isWorkspaceNotEmpty(int index){
    return isNotEmpty(getWindowStack(getWorkspaceByIndex(index)));
}

Workspace* getNextWorkspace(int dir, int mask){
    int empty = ((mask >> 2) & 3) - 1;
    int hidden = (mask & 3) - 1;
    int index = getActiveWorkspaceIndex();
    for(int i = 0; i < getNumberOfWorkspaces(); i++){
        index = (index + dir) % getNumberOfWorkspaces();
        if(index < 0)
            index += getNumberOfWorkspaces();
        if((hidden == -1 || hidden == !isWorkspaceVisible(index)) &&
                (empty == -1 || empty == !isWorkspaceNotEmpty(index))
          )
            return getWorkspaceByIndex(index);
    }
    return NULL;
}

Workspace* getWorkspaceFromMonitor(Monitor* monitor){
    assert(monitor);
    for(int i = 0; i < getNumberOfWorkspaces(); i++)
        if(getMonitorFromWorkspace(getWorkspaceByIndex(i)) == monitor)
            return getWorkspaceByIndex(i);
    return NULL;
}
Monitor* getMonitorFromWorkspace(Workspace* workspace){
    assert(workspace);
    return workspace->monitor;
}
void swapMonitors(int index1, int index2){
    Monitor* temp = getWorkspaceByIndex(index1)->monitor;
    getWorkspaceByIndex(index1)->monitor = getWorkspaceByIndex(index2)->monitor;
    getWorkspaceByIndex(index2)->monitor = temp;
}
//Workspace methods

int isWindowNotInInvisibleWorkspace(WindowInfo* winInfo){
    if(getWorkspaceIndexOfWindow(winInfo) == NO_WORKSPACE)
        return 1;
    return isWorkspaceVisible(getWorkspaceIndexOfWindow(winInfo));
}
int removeWindowFromWorkspace(WindowInfo* winInfo){
    assert(winInfo);
    if(getWorkspaceIndexOfWindow(winInfo) == NO_WORKSPACE)
        return 0;
    ArrayList* list = getWindowStackOfWindow(winInfo);
    int index = indexOf(list, winInfo, sizeof(int));
    if(index != -1)
        removeFromList(list, index);
    return index != -1;
}
int addWindowToWorkspace(WindowInfo* winInfo, int workspaceIndex){
    Workspace* workspace = getWorkspaceByIndex(workspaceIndex);
    assert(winInfo != NULL);
    assert(workspace);
    if(find(getWindowStack(workspace), winInfo, sizeof(int)) == NULL){
        addToList(getWindowStack(workspace), winInfo);
        winInfo->workspaceIndex = workspaceIndex;
        return 1;
    }
    return 0;
}

void setActiveWorkspaceIndex(int index){
    getActiveMaster()->activeWorkspaceIndex = index;
}
int getActiveWorkspaceIndex(void){
    return getActiveMaster()->activeWorkspaceIndex;
}
Workspace* getActiveWorkspace(void){
    return getWorkspaceByIndex(getActiveWorkspaceIndex());
}

ArrayList* getActiveWindowStack(void){
    return getWindowStack(getActiveWorkspace());
}
