/**
 *@file layouts.c
 */
#include <assert.h>
#include <string.h>
#include <math.h>

#include "wmfunctions.h"
#include "windows.h"
#include "layouts.h"

#include "monitors.h"
#include "globals.h"
#include "logger.h"

#define LAYOUT_FAMILY(S) {.name=""#S,.layoutFunction=S}

Layout LAYOUT_FAMILIES[]={
    LAYOUT_FAMILY(full),
    LAYOUT_FAMILY(column),
};
int NUMBER_OF_LAYOUT_FAMILIES = LEN(LAYOUT_FAMILIES);
Layout (DEFAULT_LAYOUTS[])={
    {.name="Full",.layoutFunction=full,.args={.noBorder=1,.raiseFocused=1}},
    {.name="Grid",.layoutFunction=column},
    {.name="2 Column",.layoutFunction=column, .args={.dim=0, .arg={2}}},
    {.name="2 Row",.layoutFunction=column, .args={.dim=1, .arg={2}}},
    {.name="2 Pane",.layoutFunction=column, .args={.dim=0, .arg={2},.limit=2}},
    {.name="2 HPane",.layoutFunction=column, .args={.dim=1, .arg={2},.limit=2}},

};
int NUMBER_OF_DEFAULT_LAYOUTS = LEN(DEFAULT_LAYOUTS);


#define _FOR_EACH_TILE_AT_MOST(dir,stack,N,...) {\
    int __size__=0;\
    _FOR_EACH(dir,stack,\
            if(__size__==N)break;\
            if(isTileable(getValue(stack))){__size__++;__VA_ARGS__;})}
#define FOR_EACH_TILE_AT_MOST(stack,N,...) _FOR_EACH_TILE_AT_MOST(next,stack,N,__VA_ARGS__)
#define FOR_EACH_TILE(stack,...)\
        FOR_EACH_TILE_AT_MOST(stack,-1,__VA_ARGS__)
#define FOR_EACH_TILE_AT_MOST_REVERSE(stack,N,...)\
        _FOR_EACH_TILE_AT_MOST(prev,stack,N,__VA_ARGS__;)

#define FOR_EACH_TILE_REVERSE(stack,...)\
    FOR_EACH_TILE_AT_MOST_REVERSE(stack,-1,__VA_ARGS__)

char* getNameOfLayout(Layout*layout){
    return layout?layout->name: NULL;
}
void addLayoutsToWorkspace(int workspaceIndex,Layout*layout,int num){
    if(num && !isNotEmpty(getWorkspaceByIndex(workspaceIndex)->layouts))
        getWorkspaceByIndex(workspaceIndex)->activeLayout=layout;
    for(int n=0;n<num;n++)
        insertTail(getWorkspaceByIndex(workspaceIndex)->layouts, &layout[n]);
}
void clearLayoutsOfWorkspace(int workspaceIndex){
    clearList(getWorkspaceByIndex(workspaceIndex)->layouts);
    getWorkspaceByIndex(workspaceIndex)->activeLayout=NULL;
}

int getNumberOfWindowsToTile(Node*windowStack,LayoutArgs*args,Node**last){
    assert(windowStack!=NULL);
    int size=0;
    FOR_EACH(windowStack,
        if(args&&args->limit&&size==args->limit)break;
        if(isTileable(getValue(windowStack))){
            if(last)
                *last=windowStack;
            size++;
        }
    )
    return size;
}
void transformConfig(LayoutState*state,int config[CONFIG_LEN]){
    Monitor*m=state->monitor;
    if(state->args){
        int midX=m->viewX+m->viewWidth/2;
        int midY=m->viewY+m->viewHeight/2;
        int endX=m->viewX*2+m->viewWidth;
        int endY=m->viewY*2+m->viewHeight;
        const int x=config[CONFIG_X],y=config[CONFIG_Y];
        switch(state->args->transform){
            case NONE:
                break;
            case REFLECT_HOR:
                config[CONFIG_X]=endX-(config[CONFIG_X]+config[CONFIG_WIDTH]);
                break;
            case REFLECT_VERT:
                config[CONFIG_Y]=endY-(config[CONFIG_Y]+config[CONFIG_HEIGHT]);
                break;

            case ROT_180:
                config[CONFIG_X]=endX-(config[CONFIG_X]+config[CONFIG_WIDTH]);
                config[CONFIG_Y]=endY-(config[CONFIG_Y]+config[CONFIG_HEIGHT]);
                break;
            case ROT_90:
                config[CONFIG_X]=MIN(-(y-midY)+midX, midX-(y+config[CONFIG_HEIGHT]-midY));
                config[CONFIG_Y]=MIN(x-midX+midY, midY+x+config[CONFIG_WIDTH]-midX);
                break;
            case ROT_270:
                config[CONFIG_X]=MIN(y-midY+midX, midX+(y+config[CONFIG_HEIGHT]-midY));
                config[CONFIG_Y]=MIN(-(x-midX)+midY, midY-(x+config[CONFIG_WIDTH]-midX));
                break;
        }
        config[CONFIG_BORDER]=state->args->noBorder?0:DEFAULT_BORDER_WIDTH; 
        config[CONFIG_STACK]=state->args->lowerWindows?XCB_STACK_MODE_BELOW:XCB_STACK_MODE_ABOVE; 
        if(!state->args->noAdjustForBorders){
            config[CONFIG_WIDTH]-=config[CONFIG_BORDER]*2;
            config[CONFIG_HEIGHT]-=config[CONFIG_BORDER]*2;
        }
    }
}
static void applyMasksToConfig(Monitor*m,int *values,WindowInfo*winInfo){
    if(hasMask(winInfo,ROOT_FULLSCREEN_MASK)){
        values[CONFIG_X]=0;
        values[CONFIG_Y]=0;
        values[CONFIG_WIDTH]=getRootWidth();
        values[CONFIG_HEIGHT]=getRootWidth();
    }
    else if(hasMask(winInfo,FULLSCREEN_MASK)){
        values[CONFIG_X]=m->x;
        values[CONFIG_Y]=m->y;
        values[CONFIG_WIDTH]=m->width;
        values[CONFIG_HEIGHT]=m->height;
    }
    else if(hasMask(winInfo,Y_MAXIMIZED_MASK))
        values[CONFIG_HEIGHT]=m->viewHeight;
    else if(hasMask(winInfo,X_MAXIMIZED_MASK))
        values[CONFIG_WIDTH]=m->viewWidth;
}

void configureWindow(LayoutState*state,WindowInfo* winInfo,short values[CONFIG_LEN]){
    assert(winInfo);
    int mask= XCB_CONFIG_WINDOW_X|XCB_CONFIG_WINDOW_Y|
        XCB_CONFIG_WINDOW_WIDTH|XCB_CONFIG_WINDOW_HEIGHT |
        XCB_CONFIG_WINDOW_BORDER_WIDTH|XCB_CONFIG_WINDOW_STACK_MODE;
    int config[CONFIG_LEN]={[CONFIG_BORDER]=winInfo->config[CONFIG_BORDER],[CONFIG_STACK]=XCB_STACK_MODE_ABOVE};
    assert(LEN(winInfo->config)==CONFIG_LEN-1);
    for(int i=0;i<=CONFIG_HEIGHT;i++)
        config[i]=winInfo->config[i]?winInfo->config[i]:values[i];
    applyMasksToConfig(state->monitor,config,winInfo);
    transformConfig(state,config);
    if(winInfo->config[CONFIG_BORDER])
        config[CONFIG_BORDER]=winInfo->config[CONFIG_BORDER];
    assert(winInfo->id);
    xcb_configure_window(dis, winInfo->id, mask, config);
    //if(checkError(c, winInfo->id, "could not configure window"))assert(0);
}


int getDimIndex(LayoutArgs*args){return args->dim==0?CONFIG_WIDTH:CONFIG_HEIGHT;}
int getOtherDimIndex(LayoutArgs*args){return args->dim==0?CONFIG_HEIGHT:CONFIG_WIDTH;}

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
    }
    else cycleLayouts(0);
    retile();
}
void retile(void){
    tileWorkspace(getActiveWorkspaceIndex());
}

void tileWorkspace(int index){
    //TODO support if(!applyRules(getEventRules(TilingWindows),&index))return;

    if(!isWorkspaceVisible(index))
        return;
    Workspace*workspace=getWorkspaceByIndex(index);
    applyLayout(workspace);
    Node*n;
    n=getWindowStack(workspace);
    LayoutState dummyState={.monitor=getMonitorFromWorkspace(workspace)};
    FOR_EACH(n,
        if(hasPartOfMask(getValue(n),FULLSCREEN_MASK|ROOT_FULLSCREEN_MASK)){
            short config[CONFIG_LEN]={0};
            configureWindow(&dummyState,getValue(n),config);
        }
    )
    n=getWindowStack(workspace);
    FOR_EACH(n,if(hasMask(getValue(n),BELOW_MASK))raiseLowerWindowInfo(getValue(n),0))
    n=getLast(getWindowStack(workspace));
    FOR_EACH_REVERSED(n,if(hasMask(getValue(n),ABOVE_MASK))raiseLowerWindowInfo(getValue(n),1))
}

void applyLayout(Workspace*workspace){
    Layout*layout=getActiveLayoutOfWorkspace(workspace->id);
    if(!layout){
        LOG(LOG_LEVEL_TRACE,"workspace %d does not have a layout; skipping \n",workspace->id);
        return;
    }
    Monitor*m=getMonitorFromWorkspace(workspace);
    assert(m);

    Node* windowStack=getWindowStack(workspace);
    if(!isNotEmpty(windowStack)){
        LOG(LOG_LEVEL_TRACE,"no windows in workspace\n");
        return;
    }
    int raiseFocusedWindow=layout->args.raiseFocused&&
        getFocusedWindow()&&isInList(windowStack,getFocusedWindow()->id)&&isTileable(getFocusedWindow());
    Node*last=NULL;
    int size = getNumberOfWindowsToTile(windowStack,&layout->args,&last);
    LayoutState state={.args=&layout->args,.monitor=m,.numWindows=size,.stack=windowStack,.last=last};
    if(size)
        if(layout->layoutFunction){
            LOG(LOG_LEVEL_DEBUG,"using '%s' layout: num win %d\n",layout->name,size);
            layout->layoutFunction(&state);
        }
        else
            LOG(LOG_LEVEL_WARN,"WARNING there is not a set layout function\n");
    else
        LOG(LOG_LEVEL_TRACE,"there are no windows to tile\n");
    
    if(raiseFocusedWindow)
        raiseWindowInfo(getFocusedWindow());
}

void full(LayoutState* state){
    short int values[CONFIG_LEN];
    memcpy(&values, &state->monitor->viewX, sizeof(short int)*4);
    Node*windowStack=state->last;
    FOR_EACH_TILE_REVERSE(windowStack,configureWindow(state,getValue(windowStack),values));
}
Node*splitEven(LayoutState*state,Node*stack,short const* baseValues,int dim,int num){
    assert(stack);
    assert(num);
    short values[CONFIG_LEN];
    memcpy(values, baseValues, sizeof(values));
    int sizePerWindow =values[dim]/num;
    int rem=values[dim]%num;
    LOG(LOG_LEVEL_DEBUG,"tiling at most %d windows size %d %d\n",num,sizePerWindow,dim);
    FOR_EACH_TILE_AT_MOST(stack,num,
            values[dim]=sizePerWindow+(rem-->0?1:0);
            configureWindow(state,getValue(stack),values);
            values[dim-2]+=values[dim];//convert width/height index to x/y
     )
    return stack;
}
void column(LayoutState*state){

    int size = state->numWindows;
    if(size==1){
        full(state);
        return;
    }
    int numCol= state->args->arg[0]==0?log2(size+1):state->args->arg[0];
    int rem = size%numCol;
    // split of width(0) or height(1)
    int splitDim=getDimIndex(state->args);
    short values[CONFIG_LEN];
    memcpy(values, &state->monitor->viewX, sizeof(short)*4);
    values[splitDim]/=numCol;
    Node*windowStack=state->stack;
    for(int i=0;i<numCol;i++){
        windowStack=splitEven(state, windowStack,values, 
            getOtherDimIndex(state->args),size/numCol+(rem-->0?1:0));
        values[splitDim-2]+=values[splitDim];
    }
}
