
#include <string.h>
#include "layouts.h"
#include "wmfunctions.h"
#include "assert.h"

#define CONFIG_STACK    5
#define CONFIG_BORDER   4
#define CONFIG_HEIGHT   3
#define CONFIG_WIDTH    2

#define TILE_MASK   XCB_CONFIG_WINDOW_WIDTH|XCB_CONFIG_WINDOW_HEIGHT|XCB_CONFIG_WINDOW_X|XCB_CONFIG_WINDOW_Y
Layout (DEFAULT_LAYOUTS[])={
        {.name="Full",.layoutFunction=full},
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
void full(Monitor*m,Node*windowStack,int*args){
    if(!isNotEmpty(windowStack))
        return;
    short int values[CONFIG_LEN];
    memcpy(&values, &m->viewX, sizeof(short int)*4);
    values[4]=0;
    values[5]=XCB_STACK_MODE_ABOVE;
    FOR_EACH_TILE_AT_MOST(windowStack,1,configureWindow(m,getValue(windowStack),values));

    values[5]=XCB_STACK_MODE_BELOW;
    if(windowStack)
        FOR_EACH_TILE_REVERSE(windowStack,configureWindow(m,getValue(windowStack),values));
}

Node*splitEven(Monitor*m,Node*stack,short values[CONFIG_LEN],int dim,int num){
    assert(stack);
    values[dim]/=num;
    LOG(LOG_LEVEL_INFO,"tiling at most %d windows \n",num);
    FOR_EACH_TILE_AT_MOST(stack,num,
            configureWindow(m,getValue(stack),values);
            values[dim-2]+=values[dim];//convert width/height index to x/y
     )
    return stack;
}

void grid(Monitor*m,Node*windowStack,int*args){

    int size = getNumberOfTileableWindows(windowStack);
    if(size==0)
        return;
    if(size==1){
        full(m, windowStack, args);
        return;
    }

    LOG(LOG_LEVEL_INFO,"using grid layout: num win %d\n\n\n\n",size);
    int isEven =size%2;

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

    splitEven(m,rem, values, delteDim,size/2+isEven);
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
