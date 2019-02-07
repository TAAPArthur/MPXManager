/**
 * @file mywm-util.c
 */

/// \cond
#include <assert.h>
#include <string.h>
#include <stdlib.h>
/// \endcond

#include "mywm-util.h"

///list of all windows
Node* windows;

/**
 * List of docks.
 * Docks are a special kind of window that isn't managed by us
 */
Node*docks;

///lists of all masters
Node* masterList;
///the active master
Master* master;

///list of all monitors
Node*monitors;

///list of all workspaces
Workspace*workspaces;
///number of workspaces
int numberOfWorkspaces;
char active=0;

/// called when a dock is added/removed
extern void (*onDockAddRemove)();



void destroyContext(){
    if(!active)
        return;
    active=0;
    while(isNotEmpty(getAllWindows()))
        deleteWindowNode(getAllWindows());
    deleteList(getAllWindows());
    destroyList(getAllDocks());

    while(isNotEmpty(getAllMasters()))
        removeMaster(getIntValue(getAllMasters()));
    deleteList(getAllMasters());

    deleteList(getAllMonitors());

    for(int i=numberOfWorkspaces-1;i>=0;i--){
       destroyList(workspaces[i].windows);
       destroyList(workspaces[i].layouts);
    }
    free(workspaces);

}
void createContext(int numberOfWorkspacesToCreate){
    assert(!active);
    active=1;
    windows=createEmptyHead();
    docks=createEmptyHead();
    masterList=createEmptyHead();
    master=NULL;
    numberOfWorkspaces=numberOfWorkspacesToCreate;
    workspaces=createWorkspaces(numberOfWorkspaces);
    monitors=createEmptyHead();

}
WindowInfo* createWindowInfo(unsigned int id){
    WindowInfo *wInfo =calloc(1,sizeof(WindowInfo));
    wInfo->id=id;
    wInfo->workspaceIndex=0;
    wInfo->cloneList=createEmptyHead();
    return wInfo;
}
WindowInfo*cloneWindowInfo(unsigned int id,WindowInfo*winInfo){
    WindowInfo *clone =malloc(sizeof(WindowInfo));
    memcpy(clone, winInfo, sizeof(WindowInfo));
    clone->cloneList=createEmptyHead();
    clone->id=id;
    clone->cloneOrigin=winInfo->id;
    insertHead(winInfo->cloneList,clone);
    char**fields=&winInfo->typeName;
    for(int i=0;i<4;i++)
        if(fields[i])
            fields[i]=strcpy(calloc(1+(strlen(fields[i])),sizeof(char)),fields[i]);
    return clone;
}
Master *createMaster(int id,int partnerId,char* name,int focusColor){
    Master*master= calloc(1,sizeof(Master));
    master->id=id;
    master->pointerId=partnerId;
    master->focusColor=focusColor;
    strncpy(master->name,name,sizeof(master->name));
    master->windowStack=createEmptyHead(NULL);
    master->focusedWindow=master->windowStack;
    master->windowsToIgnore=createEmptyHead();
    master->activeChains=createEmptyHead();
    master->workspaceHistory=createEmptyHead();
    return master;
}
Workspace*createWorkspaces(int size){

    Workspace *workspaces=calloc(size,sizeof(Workspace));
    for(int i=0;i<size;i++){
        workspaces[i].id=i;
        workspaces[i].name=NULL;
        workspaces[i].monitor=NULL;
        workspaces[i].activeLayout=NULL;
        workspaces[i].layouts=createCircularHead(NULL);
        workspaces[i].windows=createEmptyHead();
    }
    return workspaces;
}
void setActiveLayout(Layout*layout){
    getActiveWorkspace()->activeLayout=layout;
}

Layout* getActiveLayout(){
    return getActiveLayoutOfWorkspace(getActiveWorkspaceIndex());
}
Layout* getActiveLayoutOfWorkspace(int workspaceIndex){
    return workspaces[workspaceIndex].activeLayout;
}


int getNumberOfWorkspaces(){
    return numberOfWorkspaces;
}

//master methods
int getActiveMasterKeyboardID(){
    return getActiveMaster()->id;
}
int getActiveMasterPointerID(){
    return getActiveMaster()->pointerId;
}
int isFocusStackFrozen(){
    return getActiveMaster()->freezeFocusStack;
}
void setFocusStackFrozen(int value){
    Master*master=getActiveMaster();
    if(master->freezeFocusStack!=value){
        master->freezeFocusStack=value;
        if(!value)
            master->focusedWindow=getActiveMasterWindowStack();
    }
}

Node*getFocusedWindowByMaster(Master*master){
    return master->focusedWindow;
}
Node* getFocusedWindowNode(){
    return getFocusedWindowByMaster(getActiveMaster());
}
WindowInfo* getFocusedWindow(){
    return getValue(getFocusedWindowNode());
}
Master* getLastMasterToFocusWindow(int win){
    Node*list=getAllMasters();
    GET_MAX(list,
            getIntValue(getFocusedWindowByMaster(getValue(list)))==win,
            getFocusedTime(getValue(list))
            );
    return getValue(list);
}
unsigned int getFocusedTime(Master*m){
    return m->focusedTimeStamp;
}
void onWindowFocus(unsigned int win){

    Node *node=isInList(getAllWindows(), win);

    if(!node)return;
    if(! isFocusStackFrozen())
        addUnique(getActiveMasterWindowStack(), getValue(node));
    getActiveMaster()->focusedTimeStamp = getTime();
    Node*focusedWindow = isInList(getActiveMasterWindowStack(), win);
    if(focusedWindow){
        getActiveMaster()->focusedWindow = focusedWindow;
        assert(getFocusedWindow());
        assert(getFocusedWindow()->id);
    }


    shiftToHead(getAllWindows(), node);
    Node*masterNode = isInList(getAllMasters(), getActiveMaster()->id);
    if(masterNode)
        shiftToHead(getAllMasters(),masterNode);
}

Node*getWindowCache(){
    return getActiveMaster()->windowsToIgnore;
}
void clearWindowCache(){
    clearList(getActiveMaster()->windowsToIgnore);
    assert(getWindowCache());
    assert(getSize(getWindowCache())==0);
}
int updateWindowCache(WindowInfo* targetWindow){
    assert(targetWindow);
    return addUnique(getWindowCache(), targetWindow);
}

//end master methods


//workspace methods
int isWorkspaceVisible(int index){
    return workspaces[index].monitor?1:0;
}
int isWorkspaceNotEmpty(int index){
    if(isNotEmpty(workspaces[index].windows))
        return 1;
    return 0;

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
            return &workspaces[index];

    }
    return NULL;
}

Workspace*getWorkspaceFromMonitor(Monitor*monitor){
    assert(monitor);
    for(int i=0;i<numberOfWorkspaces;i++)
        if(workspaces[i].monitor==monitor)
            return &workspaces[i];
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

//end workspace methods

//stack methods

Node* getActiveMasterWindowStack(){
    return getWindowStackByMaster(getActiveMaster());
}
Node* getWindowStackByMaster(Master*master){
    return master->windowStack;
}

Node* getNextWindowInFocusStack(int dir){
    Node*focusedNode=getFocusedWindowNode();
    if(dir==0)return focusedNode;
    if(dir>0)
        return focusedNode->next?focusedNode->next:getActiveMasterWindowStack();
    else
       return focusedNode->prev?focusedNode->prev:getActiveMasterWindowStack();
}
Node*getWindowStack(Workspace*workspace){
    return workspace->windows;
}
int getWorkspaceIndexOfWindow(WindowInfo*winInfo){
    return winInfo->workspaceIndex;
}
Workspace* getWorkspaceOfWindow(WindowInfo*winInfo){
    return getWorkspaceByIndex(getWorkspaceIndexOfWindow(winInfo));
}
Node* getWindowStackOfWindow(WindowInfo*winInfo){
   return getWindowStack(getWorkspaceOfWindow(winInfo));
}

//Workspace methods

Workspace* getWorkspaceByIndex(int index){
    assert(index>=0);
    assert(index<numberOfWorkspaces);
    return &workspaces[index];
}





//all and active methods



int addMaster(unsigned int keyboardMasterId,unsigned int masterPointerId,char*name,int focusColor){

    assert(keyboardMasterId);

    if(isInList(masterList, keyboardMasterId))
        return 0;

    Master *keyboardMaster = createMaster(keyboardMasterId,masterPointerId,name,focusColor);
    if(!isNotEmpty(masterList))
        setActiveMaster(keyboardMaster);
    insertHead(masterList, keyboardMaster);
    return 1;
}
int removeMaster(unsigned int id){
    Node*masterList=getAllMasters();
    Node*node=isInList(masterList, id);
    //if id is not in node
    if(!node)
        return 0;
    assert(node->value);
    Master *master=(Master*)node->value;
    int isActiveMaster=getActiveMaster()->id==master->id;
    destroyList(master->windowStack);
    destroyList(master->windowsToIgnore);
    destroyList(master->activeChains);
    destroyList(master->workspaceHistory);
    deleteNode(node);
    if(isActiveMaster)
        setActiveMaster(getValue(getAllMasters()));
    return 1;
}




void removeWindowFromAllWorkspaces(WindowInfo* winInfo){
    for(int i=0;i<getNumberOfWorkspaces();i++)
        removeWindowFromWorkspace(winInfo, i);
}
int isWindowInVisibleWorkspace(WindowInfo* winInfo){
    for(int i=0;i<getNumberOfWorkspaces();i++)
        if(isWorkspaceVisible(i) && isWindowInWorkspace(winInfo,i))
            return 1;
    return 0;
}
Node* isWindowInWorkspace(WindowInfo* winInfo,int workspaceIndex){
    if(!winInfo)return NULL;
    Workspace*workspace=getWorkspaceByIndex(workspaceIndex);
    return isInList(workspace->windows,winInfo->id);
}
int removeWindowFromActiveWorkspace(WindowInfo* winInfo){
    return removeWindowFromWorkspace(winInfo,getActiveWorkspaceIndex());
}
int removeWindowFromWorkspace(WindowInfo* winInfo,int workspaceIndex){
    Node*node=isWindowInWorkspace(winInfo,workspaceIndex);
    if(node){
        softDeleteNode(node);
        return 1;
    }
    return 0;
}



int addWindowToActiveWorkspace(WindowInfo*winInfo){
    return addWindowToWorkspace(winInfo,getActiveWorkspaceIndex());
}
int addWindowToWorkspace(WindowInfo*winInfo,int workspaceIndex){
    assert(workspaceIndex>=0);
    assert(workspaceIndex<numberOfWorkspaces);
    Workspace*workspace=getWorkspaceByIndex(workspaceIndex);
    assert(winInfo!=NULL);
    if(addUnique(workspace->windows, winInfo)){
        winInfo->workspaceIndex=workspaceIndex;
        return 1;
    }
    return 0;
}

void markAsDock(WindowInfo*winInfo){winInfo->dock=1;}
int addWindowInfo(WindowInfo* winInfo){
    if(winInfo->dock){
        addUnique(getAllDocks(),winInfo);
        if(onDockAddRemove)
            onDockAddRemove();
     }
    return addUnique(getAllWindows(), winInfo);
}



int removeWindowFromMaster(Master*master,int winToRemove){
    Node*node=isInList(master->windowStack, winToRemove);
    if(node){
        if(master->focusedWindow==node && master->windowStack != node){
            master->focusedWindow =
                getNextWindowInFocusStack(
                    winToRemove==getIntValue(getFocusedWindowByMaster(master)));
        }
        softDeleteNode(node);
    }
    return node?1:0;
}
Node* getClonesOfWindow(WindowInfo*winInfo){
    return winInfo->cloneList;
}
void deleteWindowInfo(WindowInfo*winInfo){
    destroyList(getClonesOfWindow(winInfo));
    char**fields=&winInfo->typeName;
    for(int i=0;i<4;i++)
        if(fields[i])
            free(fields[i]);
    free(winInfo);
}
void deleteWindowNode(Node*winNode){
    WindowInfo* winInfo=getValue(winNode);
    deleteWindowInfo(winInfo);
    softDeleteNode(winNode);
}
int removeWindow(unsigned int winToRemove){
    assert(winToRemove!=0);

    Node *winNode =isInList(getAllWindows(), winToRemove);
    if(!winNode)
        return 0;

    Node*list=getAllMasters();
    FOR_EACH(list,removeWindowFromMaster(getValue(list),winToRemove));
    removeWindowFromAllWorkspaces(getValue(winNode));

    if(((WindowInfo*)getValue(winNode))->dock){
        softDeleteNode(isInList(getAllDocks(), winToRemove));
        if(onDockAddRemove)
            onDockAddRemove();
    }
    deleteWindowNode(winNode);
    return 1;
}

Node*getAllWindows(void){
    return windows;
}

Node*getAllDocks(){
    return docks;
}

void addMonitor(Monitor*m){
    Workspace*workspace;
    if(!isWorkspaceVisible(getActiveWorkspaceIndex()))
        workspace=getActiveWorkspace();
    else {

        workspace=getNextWorkspace(1, HIDDEN|NON_EMPTY);
        if(!workspace){
            workspace=getNextWorkspace(1, HIDDEN);
        }
    }
    if(workspace)
        workspace->monitor=m;
    insertHead(getAllMonitors(), m);
}
Node*getAllMonitors(void){
    return monitors;
}
Node* getAllMasters(void){
    return masterList;
}


Master*getActiveMaster(void){
    return master;
}
Master*getMasterById(int keyboardID){
    return getValue(isInList(getAllMasters(), keyboardID));
}
void setActiveMaster(Master*newMaster){

    assert(!isNotEmpty(getAllMasters()) || newMaster);
    master=newMaster;
}


void setActiveWorkspaceIndex(int index){
    getActiveMaster()->activeWorkspaceIndex=index;
}
int getActiveWorkspaceIndex(void){
    return getActiveMaster()->activeWorkspaceIndex;
}
Workspace* getActiveWorkspace(void){
    return &workspaces[getActiveWorkspaceIndex()];
}

Node*getActiveWindowStack(void){
    return getWindowStack(getActiveWorkspace());
}

WindowInfo* getWindowInfo(unsigned int win){
    return getValue(isInList(windows, win));
}
