/**
 * * @file monitors.c
 */
///\cond
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/randr.h>
///\endcond

#include "mywm-util.h"
#include "monitors.h"

#include "globals.h"
#include "logger.h"
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

void setDockArea(WindowInfo* info,int numberofProperties,int properties[WINDOW_STRUCT_ARRAY_SIZE]){
    assert(numberofProperties);
    for(int i=0;i<WINDOW_STRUCT_ARRAY_SIZE;i++)
        info->properties[i]=0;
    if(numberofProperties)
        memcpy(info->properties, properties, numberofProperties * sizeof(int));
}
int loadDockProperties(WindowInfo* info){

    xcb_window_t win=info->id;
    xcb_ewmh_wm_strut_partial_t strut;
    if(xcb_ewmh_get_wm_strut_partial_reply(ewmh,
                xcb_ewmh_get_wm_strut_partial(ewmh, win), &strut, NULL)){
        setDockArea(info,sizeof(xcb_ewmh_wm_strut_partial_t)/sizeof(int),(int*)&strut);
    }
    else if(xcb_ewmh_get_wm_strut_reply(ewmh,
                xcb_ewmh_get_wm_strut(ewmh, win),
                (xcb_ewmh_get_extents_reply_t*) &strut, NULL))
        setDockArea(info,sizeof(xcb_ewmh_get_extents_reply_t)/sizeof(int),(int*)&strut);
    else{
        LOG(LOG_LEVEL_WARN,"could not read struct data\n");
        return 0;
    }
    return 1;
}
int getRootWidth(void){
    return screen->width_in_pixels;
}
int getRootHeight(void){
    return screen->height_in_pixels;
}

static unsigned short* getScreenWidth(void){
    return &screen->width_in_pixels;
}

void resizeMonitorToAvoidAllStructs(Monitor*m){
    resetMonitor(m);
    Node*docks=getAllDocks();
    FOR_EACH(docks,resizeMonitorToAvoidStruct(m,getValue(docks)));
}
void resizeAllMonitorsToAvoidAllStructs(void){
    Node *mon=getAllMonitors();
    if(isNotEmpty(mon))
        FOR_EACH(mon,resizeMonitorToAvoidAllStructs(getValue(mon)))
}

/// called when a dock is added/removed
void (*onDockAddRemove)()=resizeAllMonitorsToAvoidAllStructs;

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
            end=(getScreenWidth())[(offset+1)%2];

        short int values[]={0,0,0,0};
        values[offset]=fromPositiveSide?0:
                (winInfo->onlyOnPrimary?
                    (&m->width)[offset]:(getScreenWidth())[offset])
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
void detectMonitors(void){
    LOG(LOG_LEVEL_DEBUG,"refreshing monitors\n");
    xcb_randr_get_monitors_cookie_t cookie =xcb_randr_get_monitors(dis, root, 1);
    xcb_randr_get_monitors_reply_t *monitors=xcb_randr_get_monitors_reply(dis, cookie, NULL);
    assert(monitors);
    xcb_randr_monitor_info_iterator_t iter=xcb_randr_get_monitors_monitors_iterator(monitors);
    ArrayList monitorNames;
    initArrayList(&monitorNames);
    while(iter.rem){
        xcb_randr_monitor_info_t *monitorInfo = iter.data;
        updateMonitor(monitorInfo->name,monitorInfo->primary,&monitorInfo->x);
        addToList(&monitorNames,(void*)monitorInfo->name);
        xcb_randr_monitor_info_next(&iter);
    }
    free(monitors);
    Node*list=getAllMonitors();
    FOR_EACH(list,
        int i;
        for(i=getSizeOfList(&monitorNames)-1;i>=0;i--)
            if(((Monitor*)getValue(list))->id==(long)getElement(&monitorNames,i))
                break;
        if(i==-1)
            removeMonitor(((Monitor*)getValue(list))->id);
    )
    clearArrayList(&monitorNames);
}

Node* getMonitorNodeByID(long id){
    Node*list=getAllMonitors();
    FOR_EACH_CIRCULAR(list,if(id==(*(long*)list->value))return list;);
    return NULL;
}
int removeMonitor(unsigned long id){
    Node *n=getMonitorNodeByID(id);

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
int updateMonitor(long id,int primary,short geometry[4]){
    Monitor*m=getValue(getMonitorNodeByID(id));
    int newMonitor=!m;
    if(!m){
        m=calloc(1,sizeof(Monitor));
        m->id=id;
        addMonitor(m);
    }
    m->primary=primary?1:0;
    memcpy(&m->x, geometry, sizeof(short)*4);
    resizeMonitorToAvoidAllStructs(m);
    return newMonitor;
}

void resetMonitor(Monitor*m){
    assert(m);
    memcpy(&m->viewX, &m->x, sizeof(short)*4);
}
