#include <time.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "mywm-util.h"
#include "mywm-util-private.h"


typedef struct context_t{
    void*lastEvent;

    Node* windows;
    Node* docks;
    Node*urgentWindows;
    Node*minimized;

    Node* masterList;
    Master* master;

    Node*monitors;
    int screenWidth;
    int screenHeigth;
    Workspace*workspaces;
    int numberOfWorkspaces;
} Context;

static Context *context=NULL;

void msleep(int mil){
    int sec=mil/1000;
    mil=mil-sec;
    nanosleep((const struct timespec[]){{sec, mil*1e6}}, NULL);
}

unsigned int getTime () {
   return time(NULL);
}
/**
 * Checks to if two lines intersect
 * (either both vertical or both horizontal
 * @param P1 starting point 1
 * @param D1 displacement 1
 * @param P2 staring point 2
 * @param D2 displacement 2
 * @return 1 iff the line interect
 */
static int intersects1D(int P1,int D1,int P2,int D2){
    return P1 < P2 +D2 && P1 +D1 > P2;
}

int intersects(short int arg1[4],short int arg2[4]){

    if( intersects1D(arg1[0],arg1[2],arg2[0],arg2[2]) &&
            intersects1D(arg1[1],arg1[3],arg2[1],arg2[3]))
            return 1;
    return 0;
}

void destroyContext(){
    if(!context)return;

    while(isNotEmpty(getAllDocks()))
        removeWindow(getIntValue(getAllDocks()));
    deleteList(getAllDocks());

    while(isNotEmpty(getAllWindows()))
        removeWindow(getIntValue(getAllWindows()));
    deleteList(getAllWindows());

    while(isNotEmpty(getAllMasters()))
        removeMaster(getIntValue(getAllMasters()));
    deleteList(getAllMasters());

    while(isNotEmpty(getAllMonitors()))
        removeMonitor(getIntValue(getAllMonitors()));
    deleteList(getAllMonitors());

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


    assert(!context);

    context=calloc(1, sizeof(Context));
    context->windows=createEmptyHead();
    context->docks=createEmptyHead();
    context->masterList=createEmptyHead();
    context->master=NULL;
    context->numberOfWorkspaces=numberOfWorkspaces;
    context->workspaces=createWorkSpaces(numberOfWorkspaces);
    context->monitors=createEmptyHead();
}
WindowInfo* createWindowInfo(unsigned int id){
    WindowInfo *wInfo =calloc(1,sizeof(WindowInfo));
    wInfo->id=id;
    wInfo->workspaceIndex=NO_WORKSPACE;
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
Workspace*createWorkSpaces(int size){

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
}/*
void clearLayoutsOfWorkspace(int workspaceIndex){
    clearList(context->workspaces[workspaceIndex].layouts);
    context->workspaces[workspaceIndex].activeLayout=NULL;
}*/
void setActiveLayout(Layout*layout){
    getActiveWorkspace()->activeLayout=layout;
}
Layout* switchToNthLayout(int delta){
    Node*head=getActiveWorkspace()->layouts;
    if(delta>=0)
        FOR_AT_MOST(head,delta,;)
    else
        FOR_AT_MOST_REVERSED(head,abs(delta),;)
    getActiveWorkspace()->layouts=head;
    return getValue(head);
}
Layout* getActiveLayout(){
    return getActiveLayoutOfWorkspace(getActiveWorkspaceIndex());
}
Layout* getActiveLayoutOfWorkspace(int workspaceIndex){
    return context->workspaces[workspaceIndex].activeLayout;
}
void addLayoutsToWorkspace(int workspaceIndex,Layout*layout,int num){
    assert(workspaceIndex>=0);
    assert(workspaceIndex<context->numberOfWorkspaces);
    if(isNotEmpty(context->workspaces[workspaceIndex].layouts))
        for(int n=0;n<num;n++)
            insertBefore(context->workspaces[workspaceIndex].layouts, createHead(&layout[n]));
    else{
        context->workspaces[workspaceIndex].activeLayout=layout;
        insertHead(context->workspaces[workspaceIndex].layouts, layout);
    }

}


int getNumberOfWorkspaces(){
    return context->numberOfWorkspaces;
}



void setLastEvent(void* event){
    context->lastEvent=event;
}
void* getLastEvent(){
    return context->lastEvent;
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
            master->focusedWindow=getMasterWindowStack();
    }
}
int getLastWindowClicked(){
    return getActiveMaster()->lastWindowClicked;
}
void setLastWindowClicked(int value){
    getActiveMaster()->lastWindowClicked=value;
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
int getFocusedTime(Master*m){
    return m->focusedTimeStamp;
}
void onWindowFocus(unsigned int win){

    Node *node=isInList(getAllWindows(), win);

    if(!node)return;
    if(! isFocusStackFrozen())
        addUnique(getMasterWindowStack(), getValue(node));
    getActiveMaster()->focusedTimeStamp = getTime();
    Node*focusedWindow = isInList(getMasterWindowStack(), win);
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

    for(int i=0;i<getNumberOfWorkspaces();i++){
        index =(index+dir) %getNumberOfWorkspaces();
        if(index<0)
            index+=getNumberOfWorkspaces();
        if( (hidden ==-1 || hidden == !isWorkspaceVisible(index)) &&
            (empty ==-1 || empty == !isWorkspaceNotEmpty(index))
            )
            return &context->workspaces[index];

    }
    return NULL;
}
Workspace*getNextHiddenNonEmptyWorkspace(){
    return getNextWorkspace(1,0,1);
}
Workspace*getNextHiddenWorkspace(){
    return getNextWorkspace(1,-1,1);
}

Workspace*getWorkspaceFromMonitor(Monitor*monitor){
    assert(monitor);
    for(int i=0;i<context->numberOfWorkspaces;i++)
        if(context->workspaces[i].monitor && context->workspaces[i].monitor->name==monitor->name)
            return &context->workspaces[i];
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

Node* getMasterWindowStack(){
    return getActiveMaster()->windowStack;
}

Node* getNextWindowInFocusStack(int dir){
    Node*focusedNode=getFocusedWindowNode();
    if(dir==0)return focusedNode;
    if(dir>0)
        return focusedNode->next?focusedNode->next:getMasterWindowStack();
    else
       return focusedNode->prev?focusedNode->prev:getMasterWindowStack();
}
Node*getWindowStack(Workspace*workspace){
    return getWindowStackAtLayer(workspace,DEFAULT_LAYER);
}
Node* getWindowStackAtLayer(Workspace*workspace,int layer){
    return workspace->windows[layer];
}
Node* getNextWindowInStack(int dir){
    Node*activeWindows=getActiveWindowStack();
    int focusedWindowValue=getFocusedWindow()->id;
    Node*node=isInList(activeWindows, focusedWindowValue);
    assert(node);
    if(dir>0)
        return node->next?node->next:activeWindows;
    else
       return node->prev?node->prev:getLast(activeWindows);
}

//Workspace methods





Workspace* getWorkspaceByIndex(int index){
    assert(index>=0);
    assert(index<context->numberOfWorkspaces);
    return &context->workspaces[index];
}

//get monitors

/**
 * Resets the viewport to be the size of the rectangle
 * @param m
 */
static void resetMonitor(Monitor*m){
    assert(m);
    memcpy(&m->viewX, &m->x, sizeof(short)*4);
}

void resizeAllMonitorsToAvoidAllStructs(){
    Node *mon=context->monitors;
    Node* docks;
    if(isNotEmpty(mon))
        FOR_EACH(mon,
            resetMonitor(getValue(mon));
            docks=getAllDocks();
            FOR_EACH(docks,
                    resizeMonitorToAvoidStruct(getValue(mon),getValue(docks))));
}


int resizeMonitorToAvoidStruct(Monitor*m,WindowInfo*winInfo){
    assert(m);

    assert(winInfo);
    int changed=0;
    for(int i=0;i<4;i++){
        int dim=winInfo->properties[i];
        if(dim==0)
            continue;
        int start=winInfo->properties[i*2+4];
        int end=winInfo->properties[i*2+4+1];
        assert(start<=end);

        int fromPositiveSide = i==DOCK_LEFT || i==DOCK_TOP;
        int offset=i==DOCK_LEFT || i==DOCK_RIGHT?0:1;

        if(winInfo->onlyOnPrimary){
            if(end==0)
                end=(&m->x)[offset];
            if(!m->primary)
                continue;
        }
        if(end==0)
            end=(&context->screenWidth)[(offset+1)%2];

        short int values[]={0,0,0,0};
        values[offset]=fromPositiveSide?0:
                (winInfo->onlyOnPrimary?
                    (&m->width)[offset]:(&context->screenWidth)[offset])
                -dim;
        values[(offset+1)%2]=start;
        values[offset+2]=dim;
        values[(offset+1)%2+2]=end-start;


        if(!intersects(&m->viewX, values))
            continue;

        int intersectionWidth=fromPositiveSide?
                dim-(&m->viewX)[offset]:
                (&m->viewX)[offset]+(&m->viewWidth)[offset]-values[offset];

        assert(intersectionWidth>0);
        (&m->viewWidth)[offset]-=intersectionWidth;
        if(fromPositiveSide)
            (&m->viewX)[offset]=dim;
        changed++;
    }
    return changed;
}



//all and active methods



int addMaster(unsigned int keyboardMasterId,unsigned int masterPointerId){
    Node*masterList=context->masterList;
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
    destroyList(master->windowStack);
    destroyList(master->windowsToIgnore);
    destroyList(master->activeChains);
    destroyList(master->workspaceHistory);
    deleteNode(node);
    if(master==getActiveMaster())
        setActiveMaster(getValue(getAllMasters()));
    return 1;
}

void recalculateScreenDiminsions(){
    Node*head=getAllMonitors();
    if(!isNotEmpty(head))return;
    GET_MAX(head,1,((Monitor*)getValue(head))->x+((Monitor*)getValue(head))->width)

    assert(head);
    context->screenWidth=((Monitor*)getValue(head))->x+((Monitor*)getValue(head))->width;
    head=getAllMonitors();
    GET_MAX(head,1,((Monitor*)getValue(head))->y+((Monitor*)getValue(head))->height);
    assert(head);
    context->screenHeigth=((Monitor*)getValue(head))->y+((Monitor*)getValue(head))->height;
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
    recalculateScreenDiminsions();
    return n?1:0;
}
int isPrimary(Monitor*monitor){
    return monitor->primary;
}
int addMonitor(int id,int primary,short geometry[4]){
    if(isInList(context->monitors, id))
        return 0;

    Workspace*workspace;
    if(!isWorkspaceVisible(getActiveWorkspaceIndex()))
        workspace=getActiveWorkspace();
    else {
        workspace=getNextHiddenNonEmptyWorkspace();
        if(!workspace){
            workspace=getNextHiddenWorkspace();
        }
    }
    Monitor*m=calloc(1,sizeof(Monitor));
    *m=(Monitor){id,0,primary?1:0};
    memcpy(&m->x, geometry, sizeof(short)*4);
    insertHead(context->monitors, m);
    resetMonitor(m);
    if(workspace)
        workspace->monitor=m;
    recalculateScreenDiminsions();
    return 1;
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
    Node*node;
    for(int i=0;i<NUMBER_OF_LAYERS;i++){
        node=isWindowInWorkspace(winInfo,workspaceIndex);
        if(node){
            softDeleteNode(node);
            return 1;
        }
    }
    return 0;
}



int addWindowToWorkspace(WindowInfo*info,int workspaceIndex){
    return addWindowToWorkspaceAtLayer(info, workspaceIndex, DEFAULT_LAYER);
}
int addWindowToWorkspaceAtLayer(WindowInfo*winInfo,int workspaceIndex,int layer){
    assert(workspaceIndex>=0);
    assert(layer>=0);
    assert(workspaceIndex<context->numberOfWorkspaces);
    Workspace*workspace=getWorkspaceByIndex(workspaceIndex);
    assert(winInfo!=NULL);
    if(addUnique(workspace->windows[layer], winInfo)){
        winInfo->workspaceIndex=workspaceIndex;
        return 1;
    }
    return 0;
}

int addWindowInfo(WindowInfo* wInfo){
    return addUnique(getAllWindows(), wInfo);
}

void setDockArea(WindowInfo* info,int numberofProperties,int properties[WINDOW_STRUCT_ARRAY_SIZE]){
    assert(numberofProperties);
    for(int i=0;i<WINDOW_STRUCT_ARRAY_SIZE;i++)
        info->properties[i]=0;
    if(numberofProperties)
        memcpy(info->properties, properties, numberofProperties * sizeof(int));
}
int addDock(WindowInfo* info){
    if(addUnique(context->docks, info)){
        resizeAllMonitorsToAvoidAllStructs(context);
        return 1;
    }
    return 0;
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


    Node*list=getAllMasters();
    FOR_EACH(list,removeWindowFromMaster(getValue(list),winToRemove));
    removeWindowFromAllWorkspaces(getValue(winNode));

    WindowInfo* winInfo=getValue(winNode);
    Node*clones=getClonesOfWindow(winInfo);
    FOR_EACH(clones,removeWindow(getIntValue(clones)));
    deleteList(getClonesOfWindow(winInfo));
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
    return context->master;
}
Master*getMasterById(int keyboardID){
    return getValue(isInList(getAllMasters(), keyboardID));
}
void setActiveMaster(Master*master){

    assert(!isNotEmpty(getAllMasters()) || master);
    context->master=master;
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
