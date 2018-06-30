#include <time.h>
#include <assert.h>

#include "mywm-util.h"
#include "mywm-util-private.h"


typedef struct context_t{
    void*lastEvent;
    int dirty;


    Node* windows;
    Node* docks;
    Node*urgentWindows;
    Node*minimized;

    Node* masterList;
    Node* master;

    Node*monitors;
    Workspace*workspaces;
    int numberOfWorkspaces;
} Context;

static Context *context=NULL;

unsigned int getTime () {
   return time(NULL);
}
void destroyContext(){
    Node*node;

    node=context->docks;
    while(isNotEmpty(node))
        removeWindow(getIntValue(node));
    destroyList(node);
    node=context->windows;
    while(isNotEmpty(node))
        removeWindow(getIntValue(node));
    destroyList(node);
    node=context->masterList;
    while(isNotEmpty(node))
        removeMaster(getIntValue(node));
    destroyList(node);

    node=context->monitors;
    while(isNotEmpty(node))
        removeMonitor(getIntValue(node));
    destroyList(node);


    for(int i=context->numberOfWorkspaces-1;i>=0;i--){
       for(int n=0;n<NUMBER_OF_LAYERS;n++)
           destroyList(context->workspaces[i].windows[n]);
       destroyList(context->workspaces[i].layouts);
    }
    free(context->workspaces);

    free(context);
    context=NULL;


}
void createContext(int numberOfWorkspaces){

    if(context)
        destroyContext();

    context=calloc(1, sizeof(Context));
    context->windows=createEmptyHead();
    context->docks=createEmptyHead();
    context->masterList=createEmptyHead();
    context->master=context->masterList;
    context->numberOfWorkspaces=numberOfWorkspaces;
    context->workspaces=createWorkSpaces(numberOfWorkspaces);
    context->monitors=createEmptyHead();
}
WindowInfo* createWindowInfo(Window id){
    WindowInfo *wInfo =calloc(1,sizeof(WindowInfo));
    wInfo->id=id;
    return wInfo;
}
Master *createMaster(int id){
    Master*master= calloc(1,sizeof(Master));
    master->id=id;
    master->windowStack=createEmptyHead(NULL);
    master->focusedWindow=master->windowStack;
    master->windowsToIgnore=createEmptyHead();
    return master;
}
Workspace*createWorkSpaces(int size){

    Workspace *workspaces=calloc(size,sizeof(Workspace));
    for(int i=0;i<size;i++){
        workspaces[i].id=i;
        workspaces[i].name=NULL;
        workspaces[i].monitor=NULL;
        workspaces[i].activeLayout=DEFAULT_LAYOUTS;
        workspaces[i].layouts=createCircularHead(NULL);
        for(int n=0;n<NUMBER_OF_DEFAULT_LAYOUTS;n++){
            insertHead(workspaces[i].layouts, &DEFAULT_LAYOUTS[n]);
        }
        for(int n=0;n<NUMBER_OF_LAYERS;n++)
            workspaces[i].windows[n]=createEmptyHead();
    }
    return workspaces;
}


int getNumberOfWorkspaces(){
    return context->numberOfWorkspaces;
}

void setClean(){
    context->dirty=0;
}
void setDirty(){
    context->dirty=1;
}
int isDirty(){
    return context->dirty;
}

void setLastEvent(void* event){
    context->lastEvent=event;
}
void* getLastEvent(){
    return context->lastEvent;
}

//master methods

int isFocusStackFrozen(){
    return getActiveMaster()->freezeFocusStack;
}
void setFocusStackFrozen(int value){
    Master*master=getActiveMaster();
    if(master->freezeFocusStack!=value){
        master->freezeFocusStack=value;
        if(!value)
            master->focusedWindow=master->windowStack;
    }
}
int getLastWindowClicked(){
    return getActiveMaster()->lastWindowClicked;
}
void setLastWindowClicked(int value){
    getActiveMaster()->lastWindowClicked=value;
}


Node* getFocusedWindow(){
    return getActiveMaster()->focusedWindow;
}
Node* getFocusedWindowByMaster(Master*master){
    return master->focusedWindow;
}
Node* getNextMasterNodeFocusedOnWindow(int win){
    Node*list=getAllMasters();
    GET_MAX(list,
            getIntValue(getFocusedWindowByMaster(getValue(list)))==win,
            getFocusedTime(getValue(list))
            );
    return list;
}
int getFocusedTime(Master*m){
    return m->focusedTimeStamp;
}
void onWindowFocus(int win){
    Node *node=isInList(context->windows, win);
    if(! isFocusStackFrozen())
        addUnique(getMasterWindowStack(), getValue(node));
    getActiveMaster()->focusedTimeStamp = getTime();
    assert(node);
    shiftToHead(context->windows, node);
    shiftToHead(context->masterList, context->master);

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
    return context->workspaces[index].monitor?1:0;
}
int isWorkspaceNotEmpty(int index){
    for(int i=0;i<NUMBER_OF_LAYERS;i++)
        if(isNotEmpty(context->workspaces[index].windows[i]))
            return 1;
    return 0;

}
Workspace*getNextWorkspace(int dir,int empty,int hidden){
    int index = getActiveWorkspaceIndex(context);
    for(int i=0;i<context->numberOfWorkspaces;i++){
        if( (hidden ==-1 || hidden == !isWorkspaceVisible(index)) &&
            (empty ==-1 || empty == !isWorkspaceNotEmpty(index))
            )
            return &context->workspaces[index];
        index =(index+dir) %context->numberOfWorkspaces;
    }
    return NULL;
}
Workspace*getNextHiddenNonEmptyWorkspace(){
    return getNextWorkspace(1,0,1);
}
Workspace*getNextHiddenWorkspace(){
    return getNextWorkspace(1,-1,1);
}
Workspace*getNextEmptyWorkspace(){
    return getNextWorkspace(1,1,-1);
}
Workspace*getWorkspaceFromMonitor(Monitor*monitor){
    assert(monitor);
    assert(monitor->workspaceIndex>=0);
    assert(monitor->workspaceIndex<context->numberOfWorkspaces);
    return &context->workspaces[monitor->workspaceIndex];
}
Monitor*getMonitorFromWorkspace(Workspace*workspace){
    assert(workspace);
    Node*node=context->monitors;
    UNTIL_FIRST(node,((Monitor*)getValue(node))->workspaceIndex==workspace->id)
    return getValue(node);
}


//end workspace methods






//stack methods

Node* getMasterWindowStack(){
    return getActiveMaster()->windowStack;
}

Node* getNextWindowInFocusHistory(int dir){
    Node*focusedNode=getFocusedWindow();
    if(dir>0)
        return focusedNode->next?focusedNode->next:getMasterWindowStack();
    else
       return focusedNode->prev?focusedNode->prev:getMasterWindowStack();
}
Node*getWindowStack(Workspace*workspace){
    return workspace->windows[DEFAULT_LAYER];
}
Node* getNextWindowInStack(int dir){
    Node*activeWindows=getMasterWindowStack(context);
    int focusedWindowValue=getIntValue(getFocusedWindow(context));
    Node*node=isInList(activeWindows, focusedWindowValue);
    if(dir>0)
        return node->next?node->next:activeWindows;
    else
       return node->prev?node->prev:activeWindows;
}






//Workspace methods

int isInNormalWorkspace(int workspaceIndex){
    return workspaceIndex!=STICK_WORKSPACE;
}



Workspace* getWorkspaceByIndex(int index){
    return &context->workspaces[index];
}

//get monitors


void resizeAllMonitorsToAvoidAllStructs(){

    Node *mon=context->monitors;
    Node* docks;
    if(isNotEmpty(mon))
        FOR_EACH(mon,
            resetMonitor(getValue(mon));
            docks=context->docks;
            FOR_EACH(docks,
                    resizeMonitorToAvoidStruct(mon,getValue(docks))));
}


void resizeMonitorToAvoidStruct(Node*n,WindowInfo*winInfo){

    assert(n);
    assert(winInfo);
    Monitor*m=(Monitor*)getValue(n);
    for(int i=0;i<4;i++){

        int pos=i;
        int dim=winInfo->properties[i];
        if(dim==0)
            continue;
        if(pos==DOCK_LEFT || pos==DOCK_RIGHT){
            int width=m->viewWidth-dim;
            if(pos==DOCK_LEFT)
                resizeMonitor(m,dim, m->viewY, width, m->viewHeight);
            else
                resizeMonitor(m,0, m->viewY, width, m->viewHeight);
        }
        else {
            int height=m->viewHeight-dim;
            if(pos==DOCK_TOP)
                resizeMonitor(m,m->x, dim, m->viewWidth, height);
            else
                resizeMonitor(m,m->x, 0, m->viewWidth, height);
        }
    }
}
void resetMonitor(Monitor*m){
    assert(m);
    m->viewX=m->x;
    m->viewY=m->y;
    m->viewWidth=m->width;
    m->viewHeight=m->height;
}
void resizeMonitor(Monitor*monitor,int x,int y,int width, int height){
    monitor->viewX=x;
    monitor->viewY=y;
    monitor->viewWidth=width;
    monitor->viewHeight=height;
}


static int intersects1D(int P1,int D1,int P2,int D2){
    return P1 < P2 +D2 && P1 +D1 > P2;
}

int intersects(short int arg1[4],short int arg2[4]){

    if( intersects1D(arg1[0],arg1[2],arg2[0],arg2[2]) &&
            intersects1D(arg1[1],arg1[3],arg2[1],arg2[3]))
            return 1;
    return 0;
}


//all and active methods



int addMaster(unsigned int keyboardMasterId){
    Node*masterList=context->masterList;
    if(!keyboardMasterId)
        return 0;

    if(isInList(masterList, keyboardMasterId))
        return 0;
    Master *keyboardMaster = createMaster(keyboardMasterId);
    insertHead(masterList, keyboardMaster);
    return 1;
}
int removeMaster(unsigned int id){
    Node*masterList=context->masterList;
    Node*node=isInList(masterList, id);
    //if id is not in node
    if(!node)
        return 0;
    assert(node->value);
    Master *master=(Master*)node->value;
    destroyList(master->windowStack);
    destroyList(master->windowsToIgnore);
    deleteNode(node);
    return 1;
}


int removeMonitor(unsigned int id){
    Node *n=isInList(context->monitors, id);

    if(!n)
        return 0;

    Workspace*w=getWorkspaceFromMonitor(getValue(n));
    if(w)
        w->monitor=NULL;
    if(n)
        deleteNode(n);
    return n?1:0;
}
int addMonitor(int id,int primary,int x,int y,int width,int height){
    if(isInList(context->monitors, id))
        return 0;
    Workspace*workspace=getNextHiddenNonEmptyWorkspace();
    if(!workspace){
        workspace=getNextHiddenWorkspace();
        if(!workspace){
            assert(0);
            return 0;
        }
    }

    Monitor*m=calloc(1,sizeof(Monitor));
    *m=(Monitor){id,0,primary,x,y,width,height};
    insertHead(context->monitors, m);

    resetMonitor(m);

    setMonitorForWorkspace(workspace,m);
     return 1;
}
void setMonitorForWorkspace(Workspace*w,Monitor*m){
    w->monitor=m;
    m->workspaceIndex=w->id;
}

void removeWindowFromAllWorkspaces(WindowInfo* winInfo){
    for(int i=0;i<context->numberOfWorkspaces;i++)
        removeWindowFromWorkspace(winInfo, i);
}
int isWindowInVisibleWorkspace(WindowInfo* winInfo){
    for(int i=0;i<getNumberOfWorkspaces();i++)
        if(isWorkspaceVisible(i) && isWindowInWorkspace(winInfo,i))
            return 1;
    return 0;
}
Node* isWindowInWorkspace(WindowInfo* winInfo,int workspaceIndex){
    if(workspaceIndex==NO_WORKSPACE)
        return NULL;
    Workspace*workspace=getWorkspaceByIndex(workspaceIndex);
    Node*node;
    for(int i=0;i<NUMBER_OF_LAYERS;i++){
        node=isInList(workspace->windows[i], winInfo->id);
        if(node)
            return node;
    }
    return NULL;
}
int removeWindowFromWorkspace(WindowInfo* winInfo,int workspaceIndex){
    if(workspaceIndex==NO_WORKSPACE)
        return 0;
    Workspace*workspace=getWorkspaceByIndex(workspaceIndex);
    Node*node;
    for(int i=0;i<NUMBER_OF_LAYERS;i++){
        node=isWindowInWorkspace(winInfo,workspaceIndex);
        if(node){
            if(isWorkspaceVisible(workspace->id))
                context->dirty=1;
            softDeleteNode(node);
            return 1;
        }
    }
    return 0;
}

void moveWindowToLayerForAllWorksppaces(WindowInfo* winInfo,int layer){
    for(int i=0;i<context->numberOfWorkspaces;i++)
        moveWindowToLayer(winInfo, i, layer);
}
void moveWindowToLayer(WindowInfo*winInfo,int workspaceIndex,int layer){
    if(removeWindowFromWorkspace(winInfo, workspaceIndex))
        addWindowToWorkspaceAtLayer(winInfo, workspaceIndex, layer);
}

void addWindowToWorkspace(WindowInfo*info,int workspaceIndex){
    addWindowToWorkspaceAtLayer(info, workspaceIndex, DEFAULT_LAYER);
}
void addWindowToWorkspaceAtLayer(WindowInfo*info,int workspaceIndex,int layer){
    assert(workspaceIndex>=0);
    assert(layer>=0);
    assert(workspaceIndex<context->numberOfWorkspaces);
    Workspace*workspace=getWorkspaceByIndex(workspaceIndex);
    assert(info!=NULL);
    addUnique(workspace->windows[layer], info);
    if(isWorkspaceVisible(workspace->id))
       context->dirty=1;
}

int addWindowInfo(WindowInfo* wInfo){
    return addUnique(getAllWindows(), wInfo);
}


int addDock(WindowInfo* info){
    if(addUnique(context->docks, info)){
        resizeAllMonitorsToAvoidAllStructs(context);
        return 1;
    }
    return 0;
}

int _removeWindowFromMaster(Node*masterNode,int winToRemove){
    Master*master=(Master*)masterNode->value;
    removeByValue(master->windowStack, winToRemove);
    if(getIntValue(master->focusedWindow)==winToRemove)
        master->focusedWindow=master->focusedWindow->next;
    if(master->focusedWindow==NULL)
        master->focusedWindow=master->windowStack;
    return 0;
}

int removeWindow(unsigned int winToRemove){
    assert(winToRemove!=0);

    Node *winNode =isInList(context->windows, winToRemove);
    int dock=0;
    if(!winNode){
        winNode=isInList(context->docks, winToRemove);
        if(!winNode)
            return 0;
        else
            dock=1;
    }

    Node*list=context->masterList;
    FOR_EACH(list,_removeWindowFromMaster(list,winToRemove));
    removeWindowFromAllWorkspaces(getValue(winNode));

    WindowInfo* winInfo=getValue(winNode);
    char**fields=&winInfo->typeName;
    for(int i=0;i<4;i++)
        if(fields[i])
            free(fields[i]);
    deleteNode(winNode);
    if(dock)
        resizeAllMonitorsToAvoidAllStructs(context);
    return 1;
}


Node*getAllWindows(){
    return context->windows;
}
Node*getAllDocks(){
    return context->docks;
}
Node*getAllMonitors(){
    return context->monitors;
}
Node* getAllMasters(){
    return context->masterList;
}


Master*getActiveMaster(){
    return getValue(context->master);
}


void setActiveWorkspaceIndex(int index){
    getActiveMaster()->activeWorkspaceIndex=index;
}
int getActiveWorkspaceIndex(){
    return getActiveMaster()->activeWorkspaceIndex;
}
Workspace* getActiveWorkspace(){
    return &context->workspaces[getActiveWorkspaceIndex(context)];
}

Node*getActiveWindowStack(){
    return getWindowStack(getActiveWorkspace(context));
}

WindowInfo* getWindowInfo(unsigned int win){
    return getValue(isInList(context->windows, win));
}

Node*getMasterNodeById(int keyboardID){
    return isInList(context->masterList, keyboardID);
}
void setActiveMasterNodeById(Node*node){
    assert(node!=NULL);
    assert(node->value);
    context->master=node;
}

