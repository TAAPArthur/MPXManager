/**
 *@file layouts.c
 */
#include <assert.h>
#include <string.h>

#include "wmfunctions.h"
#include "layouts.h"

#include "globals.h"
#include "logger.h"


#define CONFIG_STACK    5
#define CONFIG_BORDER   4
#define CONFIG_HEIGHT   3
#define CONFIG_WIDTH    2

#define TILE_MASK   XCB_CONFIG_WINDOW_WIDTH|XCB_CONFIG_WINDOW_HEIGHT|XCB_CONFIG_WINDOW_X|XCB_CONFIG_WINDOW_Y
Layout (DEFAULT_LAYOUTS[])={
    {.name="Full",.layoutFunction=full,.args=(int*)(int[]){0,0}},
    {.name="Grid",.layoutFunction=grid,.args=(int*)(int[]){0,0,CONFIG_WIDTH}},

//        {.name="Master",.layoutFunction=masterPane},
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
#define FOR_EACH_TILE_AT_MOST_REVERSE(stack,N,...){\
        Node*__end__=stack;\
        stack=getLast(stack);\
        _FOR_EACH_TILE_AT_MOST(prev,stack,-1,__VA_ARGS__;if(__end__==stack)break;)\
    }

#define FOR_EACH_TILE_REVERSE(stack,...)\
    FOR_EACH_TILE_AT_MOST_REVERSE(stack,-1,__VA_ARGS__)


int getNumberOfWindowsToTile(Node*windowStack,int limit){
    int size = 0;
    FOR_EACH(windowStack,
                if(isTileable(getValue(windowStack)))size++)

    if(limit)
        return size>limit?limit:size;
    else return size;
}

#define COPY_VALUES(I)  actualWindowValues[I]=winInfo->config[I]?winInfo->config[I]:values[I];
static void configureWindow(Monitor*m,WindowInfo* winInfo,short values[CONFIG_LEN]){
    assert(winInfo);
    assert(m);
    int actualWindowValues[CONFIG_LEN+1];
    int mask= XCB_CONFIG_WINDOW_X|XCB_CONFIG_WINDOW_Y|
        XCB_CONFIG_WINDOW_WIDTH|XCB_CONFIG_WINDOW_HEIGHT |
        XCB_CONFIG_WINDOW_BORDER_WIDTH|XCB_CONFIG_WINDOW_STACK_MODE;
    if(winInfo){
        COPY_VALUES(0);//x
        COPY_VALUES(1);//y

        int maxWidth=getMaxDimensionForWindow(m, winInfo,0);
        if(maxWidth)
            actualWindowValues[2]=maxWidth;
        else    COPY_VALUES(2);

        int maxHeight=getMaxDimensionForWindow(m, winInfo,1);
        if(maxHeight){
            assert(maxHeight>=values[3]);
            actualWindowValues[3]=maxHeight;
        }
        else  COPY_VALUES(3);
        COPY_VALUES(4);//border
        actualWindowValues[5]=values[5];
    }
    assert(winInfo->id);
    xcb_configure_window_checked(dis, winInfo->id, mask, actualWindowValues);
    LOG(LOG_LEVEL_ALL,"Window dims %d %d %d %d %d %d\n",values[0],values[1],values[2],values[3],values[4],values[5]);
    LOG(LOG_LEVEL_ALL,"Window dims %d %d %d %d \n",m->width,m->height,m->viewWidth,m->viewHeight);
    //if(checkError(c, winInfo->id, "could not configure window"))assert(0);
}






int getLimit(int*args){
    return args[0];
}
void full(Monitor*m,Node*windowStack,int*args){
    if(!isNotEmpty(windowStack))
        return;
    int size=getNumberOfWindowsToTile(windowStack,getLimit(args));
    short int values[CONFIG_LEN];
    memcpy(&values, &m->viewX, sizeof(short int)*4);
    values[4]=0;
    values[5]=XCB_STACK_MODE_ABOVE;

    FOR_EACH_TILE_AT_MOST(windowStack,1,configureWindow(m,getValue(windowStack),values));

    values[5]=XCB_STACK_MODE_BELOW;
    if(size>1)
        FOR_EACH_TILE_AT_MOST(windowStack,size-1,configureWindow(m,getValue(windowStack),values));
}

Node*splitEven(Monitor*m,Node*stack,short values[CONFIG_LEN],int dim,int num){
    assert(stack);
    int rem=values[dim]%num;
    int size =values[dim]/num;
    LOG(LOG_LEVEL_TRACE,"tiling at most %d windows size %d rem: %d\n",num,size, rem);
    FOR_EACH_TILE_AT_MOST(stack,num,
            values[dim] =size+(rem-->0?1:0);
            configureWindow(m,getValue(stack),values);
            values[dim-2]+=values[dim];//convert width/height index to x/y
     )
    return stack;
}

void grid(Monitor*m,Node*windowStack,int*args){

    int size = getNumberOfWindowsToTile(windowStack,getLimit(args));
    if(size==0)
        return;
    if(size==1){
        full(m, windowStack, args);
        return;
    }

    LOG(LOG_LEVEL_TRACE,"using grid layout: num win %d\n",size);
    int isOdd =size%2==1;

    int fixedDim=args[2];
    int delteDim=fixedDim==CONFIG_WIDTH?CONFIG_HEIGHT:CONFIG_WIDTH;
    short values[CONFIG_LEN];
    values[CONFIG_BORDER]=1;
    values[CONFIG_STACK]=XCB_STACK_MODE_ABOVE;
    memcpy(&values, &m->viewX, sizeof(short int)*4);
    values[fixedDim]/=2;
    Node*rem=splitEven(m,windowStack, values, delteDim,size/2);

    memcpy(&values, &m->viewX, sizeof(short int)*4);
    values[fixedDim]/=2;
    values[fixedDim-2]+=values[fixedDim];

    splitEven(m,rem, values, delteDim,size/2+isOdd);
}
/*
void masterPane(Monitor*m,Node*windowStack,int*args){
    int size = getNumberOfWindowsInActiveWorkspace();
    if(size==0)
        return;
    if(size==1){
        full(m,windowStack,args);
        return;
    }
    //TODO fix master ratio
    int masterWidth=m->width*(float)args[2];
    int dim=args[3];
    int values[CONFIG_LEN];
    memcpy(&values, &m->x, sizeof(int)*4);

    Node*rem=splitEven(m,windowStack, values, dim,size/2);
    values[dim-2]+=values[dim];
    splitEven(m,rem, values, dim,size/2+isEven);
    tileWindow(->dpy,getIntValue(->windows),m->x,m->y,masterWidth,m->height,0);
    splitEven(->windows, m->x+masterWidth, m->y, m->width-masterWidth, m->height, size-1);
}

*/
