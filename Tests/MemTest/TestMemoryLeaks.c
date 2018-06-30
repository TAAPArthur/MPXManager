#include "../../mywm-util.h"
#include "../../mywm-util-private.h"
#include "../../wmfunctions.h"
#include "../../defaults.h"
#include "../../event-loop.h"

#include "../TestHelper.c"

static int size=10;

void testDeleteFunctions(){
    Node *main=createEmptyHead();
    Node *clone1=createEmptyHead();
    Node *clone2=createEmptyHead();
    Node *clone3=createEmptyHead();
    for(int i=0;i<size;i++){
        int *n=malloc(sizeof(int));
        insertHead(main, n);
        insertHead(clone1, n);
        insertHead(clone2, n);
        insertHead(clone3, n);
    }
    clearList(clone1);
    free(clone1);
    for(int i=0;i<size;i++)
        softDeleteNode(clone2);
    free(clone2);
    destroyList(clone3);
    for(int i=0;i<size;i++){
        deleteNode(main);
    }
    free(main);
}
void testContextCreation(){
    createContext(size);
    for(int i=1;i<=size;i++){
        addWindowInfo(createWindowInfo(i));
        addDock(createWindowInfo(i+size));
        addMaster(i);
        addMonitor(i, 1, 0, 0, 0, 0);
    }
    for(int i=1;i<=size/2;i++){
        removeWindow(i);
        removeWindow(i+size);
        removeMaster(i);
        removeMonitor(i);
    }
    destroyContext();
}


void testLoad(){
    INIT
    createContext(size);
    onStartup();
    for(int i=0;i<size;i++){
        int win=createArbitraryWindow(dc);
        if(i%2==0)
            setArbitraryProperties(dc,win);
        if(i%3==0)
            mapWindow(dc, win);
    }
    connectToXserver();
    destroyContext();
    END
}
void testEventLoop(){
    INIT
    xcb_window_t win[size*2];
    createContext(size);
    onStartup();
    connectToXserver();
    START_MY_WM
    for(int i=0;i<size;i++){
        int win=createArbitraryWindow(dc);
        if(i%2==0)
            setArbitraryProperties(dc,win);
        if(i%3==0)
            mapWindow(dc, win);
    }
    WAIT_FOR(getSize(getAllWindows())!=size*2)
    for(int i=0;i<size/2;i++){
        distroyWindow(dc, win[i]);
    }
    quit();
    END
}
int main(){
    testDeleteFunctions();
    testContextCreation();
    testLoad();
    testEventLoop();
}
