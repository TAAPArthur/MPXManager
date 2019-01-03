/**
 * @file mywm-util.c
 */

/// \cond
#include <time.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
/// \endcond

#include "mywm-util.h"

///list of all windows
Node* windows;

///lists of all masters
Node* masterList;
///the active master
Master* master;

///list of all monitors
Node*monitors;
///width of the screen
int screenWidth;
///height of the screen
int screenHeigth;

///list of all workspaces
Workspace*workspaces;
///number of workspaces
int numberOfWorkspaces;
char active=0;


void msleep(int mil){
    int sec=mil/1000;
    mil=mil%1000;
    nanosleep((const struct timespec[]){{sec, mil*1e6}}, NULL);
}

unsigned int getTime () {
   return time(NULL);
}


void destroyContext(){
    if(!active)
        return;
    active=0;
    while(isNotEmpty(getAllWindows()))
        removeWindow(getIntValue(getAllWindows()));
    deleteList(getAllWindows());

    while(isNotEmpty(getAllMasters()))
        removeMaster(getIntValue(getAllMasters()));
    deleteList(getAllMasters());

    while(isNotEmpty(getAllMonitors()))
        removeMonitor(getIntValue(getAllMonitors()));
    deleteList(getAllMonitors());

    for(int i=numberOfWorkspaces-1;i>=0;i--){
       for(int n=0;n<NUMBER_OF_LAYERS;n++)
           destroyList(workspaces[i].windows[n]);
       destroyList(workspaces[i].layouts);
    }
    free(workspaces);

}
void createContext(int numberOfWorkspacesToCreate){
    assert(!active);
    active=1;
    windows=createEmptyHead();
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
Master *createMaster(int id,int partnerId){
    Master*master= calloc(1,sizeof(Master));
    master->id=id;
    master->pointerId=partnerId;
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
        for(int n=0;n<NUMBER_OF_LAYERS;n++)
            workspaces[i].windows[n]=createEmptyHead();
    }
    return workspaces;
}
void clearLayoutsOfWorkspace(int workspaceIndex){
    clearList(workspaces[workspaceIndex].layouts);
    workspaces[workspaceIndex].activeLayout=NULL;
}
int setActiveLayout(Layout*layout){
    if(getActiveLayout()==layout)
        return 0;
    getActiveWorkspace()->activeLayout=layout;
    return 1;
}

Layout* getActiveLayout(){
    return getActiveLayoutOfWorkspace(getActiveWorkspaceIndex());
}

char* getNameOfLayout(Layout*layout){
    if(layout)
        return layout->name;
    else return NULL;
}
Layout* getActiveLayoutOfWorkspace(int workspaceIndex){
    return workspaces[workspaceIndex].activeLayout;
}
void addLayoutsToWorkspace(int workspaceIndex,Layout*layout,int num){
    assert(workspaceIndex>=0);
    assert(workspaceIndex<numberOfWorkspaces);
    if(num && !isNotEmpty(getWorkspaceByIndex(workspaceIndex)->layouts))
        getWorkspaceByIndex(workspaceIndex)->activeLayout=layout;
    for(int n=0;n<num;n++)
        insertTail(getWorkspaceByIndex(workspaceIndex)->layouts, &layout[n]);
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
    for(int i=0;i<NUMBER_OF_LAYERS;i++)
        if(isNotEmpty(workspaces[index].windows[i]))
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
        if(workspaces[i].monitor && workspaces[i].monitor->name==monitor->name)
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
    return getWindowStackAtLayer(workspace,NORMAL_LAYER);
}
Node* getWindowStackAtLayer(Workspace*workspace,int layer){
    return workspace->windows[layer];
}
int getWorkspaceIndexOfWindow(WindowInfo*winInfo){
    return winInfo->workspaceIndex;
}
Workspace* getWorkspaceOfWindow(WindowInfo*winInfo){
    return getWorkspaceByIndex(getWorkspaceIndexOfWindow(winInfo));
}
Node* getWindowStackOfWindow(WindowInfo*winInfo){
   return getWindowStackAtLayer(getWorkspaceOfWindow(winInfo),getLayer(winInfo));
}
int getLayer(WindowInfo*winInfo){
    return winInfo->layer;
}

//Workspace methods

Workspace* getWorkspaceByIndex(int index){
    assert(index>=0);
    assert(index<numberOfWorkspaces);
    return &workspaces[index];
}





//all and active methods



int addMaster(unsigned int keyboardMasterId,unsigned int masterPointerId){

    assert(keyboardMasterId);

    if(isInList(masterList, keyboardMasterId))
        return 0;

    Master *keyboardMaster = createMaster(keyboardMasterId,masterPointerId);
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


int removeMonitor(unsigned int id){
    Node *n=isInList(monitors, id);

    if(!n)
        return 0;

    Workspace*w=getWorkspaceFromMonitor(getValue(n));
    if(w)
        w->monitor=NULL;
    if(n)
        deleteNode(n);
    return n?1:0;
}

int isPrimary(Monitor*monitor){
    return monitor->primary;
}
int addMonitor(int id,int primary,short geometry[4]){
    if(isInList(monitors, id))
        return 0;

    Workspace*workspace;
    if(!isWorkspaceVisible(getActiveWorkspaceIndex()))
        workspace=getActiveWorkspace();
    else {

        workspace=getNextWorkspace(1, HIDDEN|NON_EMPTY);
        if(!workspace){
            workspace=getNextWorkspace(1, HIDDEN);
        }
    }
    Monitor*m=calloc(1,sizeof(Monitor));
    *m=(Monitor){id,primary?1:0};
    memcpy(&m->x, geometry, sizeof(short)*4);
    insertHead(monitors, m);
    resetMonitor(m);
    if(workspace)
        workspace->monitor=m;
    return 1;
}

void resetMonitor(Monitor*m){
    assert(m);
    memcpy(&m->viewX, &m->x, sizeof(short)*4);
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
    Node*node;
    for(int i=0;i<NUMBER_OF_LAYERS;i++){
        node=isInList(workspace->windows[i], winInfo->id);
        if(node)
            return node;
    }
    return NULL;
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
int addWindowToWorkspace(WindowInfo*info,int workspaceIndex){
    return addWindowToWorkspaceAtLayer(info, workspaceIndex, NORMAL_LAYER);
}
int addWindowToWorkspaceAtLayer(WindowInfo*winInfo,int workspaceIndex,int layer){
    assert(workspaceIndex>=0);
    assert(layer>=0);
    assert(workspaceIndex<numberOfWorkspaces);
    Workspace*workspace=getWorkspaceByIndex(workspaceIndex);
    assert(winInfo!=NULL);
    if(addUnique(workspace->windows[layer], winInfo)){
        winInfo->workspaceIndex=workspaceIndex;
        winInfo->layer=layer;
        return 1;
    }
    return 0;
}

int addWindowInfo(WindowInfo* wInfo){
    return addUnique(getAllWindows(), wInfo);
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
    Node*clones=getClonesOfWindow(winInfo);
    FOR_EACH(clones,removeWindow(getIntValue(clones)));
    deleteList(getClonesOfWindow(winInfo));
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

    Node *winNode =isInList(windows, winToRemove);
    if(!winNode)
        return 0;

    Node*list=getAllMasters();
    FOR_EACH(list,removeWindowFromMaster(getValue(list),winToRemove));
    removeWindowFromAllWorkspaces(getValue(winNode));

    deleteWindowNode(winNode);
    return 1;
}

Node*getAllWindows(void){
    return windows;
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
