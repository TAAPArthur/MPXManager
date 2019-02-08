

#include <assert.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI.h>


#include "mywm-util.h"
#include "logger.h"
#include "globals.h"
#include "windows.h"
#include "monitors.h"

extern xcb_connection_t *dis;

extern Display *dpy;

#ifndef INIT_LOG_LEVEL
#define INIT_LOG_LEVEL LOG_LEVEL_INFO
#endif

int LOG_LEVEL=INIT_LOG_LEVEL;

int getLogLevel(){
    return LOG_LEVEL;
}
void setLogLevel(int level){
    LOG_LEVEL=level;
}
void printNodeList(Node*n){
    printf("Printing list: \n");
    FOR_EACH(n,printf("%d ",getIntValue(n)));
    printf("\n");
}
#define PRINT_MASK(str,mask) if( str & mask)LOG(LOG_LEVEL_DEBUG,#str " ");
void printMask(int mask){
    PRINT_MASK(NO_MASK,mask);
    PRINT_MASK(X_MAXIMIZED_MASK,mask);
    PRINT_MASK(Y_MAXIMIZED_MASK,mask);
    PRINT_MASK(FULLSCREEN_MASK,mask);
    PRINT_MASK(ROOT_FULLSCREEN_MASK,mask);
    PRINT_MASK(BELOW_MASK,mask);
    PRINT_MASK(ABOVE_MASK,mask);
    PRINT_MASK(NO_TILE_MASK,mask);

    PRINT_MASK(STICKY_MASK,mask);
    PRINT_MASK(HIDDEN_MASK,mask);
    PRINT_MASK(EXTERNAL_RESIZE_MASK,mask);
    PRINT_MASK(EXTERNAL_MOVE_MASK,mask);
    PRINT_MASK(EXTERNAL_BORDER_MASK,mask);
    PRINT_MASK(EXTERNAL_RAISE_MASK,mask);
    PRINT_MASK(FLOATING_MASK,mask);
    PRINT_MASK(SRC_INDICATION_OTHER,mask);
    PRINT_MASK(SRC_INDICATION_APP,mask);
    PRINT_MASK(SRC_INDICATION_PAGER,mask);
    PRINT_MASK(SRC_ANY,mask);
    PRINT_MASK(INPUT_MASK,mask);
    PRINT_MASK(WM_TAKE_FOCUS_MASK,mask);
    PRINT_MASK(WM_DELETE_WINDOW_MASK,mask);
    PRINT_MASK(WM_PING_MASK,mask);
    PRINT_MASK(PARTIALLY_VISIBLE,mask);
    PRINT_MASK(FULLY_VISIBLE,mask);
    PRINT_MASK(MAPABLE_MASK,mask);
    PRINT_MASK(MAPPED_MASK,mask);
    PRINT_MASK(URGENT_MASK,mask);
    LOG(LOG_LEVEL_DEBUG,"\n");
}
void printMasterSummary(void){
    LOG(LOG_LEVEL_DEBUG,"Active Master %d\n",getActiveMaster()->id);
    Node *n=getAllMasters();
    FOR_EACH(n,
        Master*master=getValue(n);
        LOG(LOG_LEVEL_DEBUG,"(%d, %d: %s %06x) ",master->id,master->pointerId,master->name, master->focusColor);
    );
    LOG(LOG_LEVEL_DEBUG,"\n");
}
void printMonitorSummary(void){
    Node *n=getAllMonitors();
    FOR_EACH(n,
        Monitor *m=getValue(n);
        LOG(LOG_LEVEL_DEBUG,"Monitor %ld primary %d",m->id,m->primary);
        PRINT_ARR(&m->x,4);
        PRINT_ARR(&m->viewX,4);
    );
    LOG(LOG_LEVEL_DEBUG,"\n");
}
void printWindowStackSummary(Node*n){
    FOR_EACH(n,
        WindowInfo*winInfo=getValue(n);
        LOG(LOG_LEVEL_DEBUG,"(%d: %s) ",winInfo->id,winInfo->title);
    );
    LOG(LOG_LEVEL_DEBUG,"\n");
}
void printSummary(void){
    LOG(LOG_LEVEL_DEBUG,"Summary:\n");
    printMasterSummary();
    printMonitorSummary();
    Node*n=getActiveMasterWindowStack();
    if(isNotEmpty(n)){
        LOG(LOG_LEVEL_DEBUG,"Active master window stack:\n");
        FOR_EACH(n,
            WindowInfo*winInfo=getValue(n);
            LOG(LOG_LEVEL_DEBUG,"(%d: %s) ",winInfo->id,winInfo->title);
        );
    }
    LOG(LOG_LEVEL_DEBUG,"\n");
    for(int i=0;i<getNumberOfWorkspaces();i++){
        Node*n=getWindowStack(getWorkspaceByIndex(i));
        if(!isNotEmpty(n))continue;
        LOG(LOG_LEVEL_DEBUG,"%d \n",i);
        printWindowStackSummary(n);
    }
    if(isNotEmpty(getAllDocks())){
        LOG(LOG_LEVEL_DEBUG,"%d Docks: \n",getSize(getAllDocks()));
        printWindowStackSummary(getAllDocks());
    }
}
void dumpAllWindowInfo(void){
    Node*n=getAllWindows();
    
    FOR_EACH(n,dumpWindowInfo(getValue(n));LOG(LOG_LEVEL_DEBUG,"\n"););
}

void dumpWindowInfo(WindowInfo* win){
    if(!win)return;

    LOG(LOG_LEVEL_DEBUG,"Dumping window info %d(%#x) group: %d(%#x) Transient for %d(%#x)\n",win->id,win->id,win->groupId,win->groupId,win->transientFor,win->transientFor);
    LOG(LOG_LEVEL_DEBUG,"Labels class %s (%s)\n",win->className,win->instanceName);
    LOG(LOG_LEVEL_DEBUG,"Type is %d (%s) implicit: %d Dock %d\n",win->type,win->typeName,win->implicitType,win->dock);

    LOG(LOG_LEVEL_DEBUG,"Title class %s\n",win->title);

    LOG(LOG_LEVEL_DEBUG,"Mask %d\n",win->mask);
    printMask(win->mask);
    LOG(LOG_LEVEL_DEBUG,"Workspace %d\n",win->workspaceIndex);
}

void dumpAtoms(xcb_atom_t*atoms,int numberOfAtoms){
    xcb_get_atom_name_reply_t *reply;
    for(int i=0;i<numberOfAtoms;i++){
        reply=xcb_get_atom_name_reply(dis, xcb_get_atom_name(dis, atoms[i]), NULL);
        if(!reply)continue;
        LOG(LOG_LEVEL_DEBUG,"atom: %.*s\n",reply->name_len,xcb_get_atom_name_name(reply));
        free(reply);
    }
}

#define ADD_TYPE_CASE(TYPE) case TYPE: return  #TYPE
char *genericEventTypeToString(int type){
    switch(type) {
        ADD_TYPE_CASE(XCB_INPUT_KEY_PRESS);
        ADD_TYPE_CASE(XCB_INPUT_KEY_RELEASE);
        ADD_TYPE_CASE(XCB_INPUT_BUTTON_PRESS);
        ADD_TYPE_CASE(XCB_INPUT_BUTTON_RELEASE);
        ADD_TYPE_CASE(XCB_INPUT_MOTION);
        ADD_TYPE_CASE(XCB_INPUT_ENTER);
        ADD_TYPE_CASE(XCB_INPUT_LEAVE);
        ADD_TYPE_CASE(XCB_INPUT_FOCUS_IN);
        ADD_TYPE_CASE(XCB_INPUT_FOCUS_OUT);
        ADD_TYPE_CASE(XCB_INPUT_HIERARCHY);

        default:
             return "unkown XI event";
     }
}

char *eventTypeToString(int type){
    switch(type) {
        case 0: return "Error";

        ADD_TYPE_CASE(XCB_EXPOSE);
        ADD_TYPE_CASE(XCB_VISIBILITY_NOTIFY);
        ADD_TYPE_CASE(XCB_CREATE_NOTIFY);
        ADD_TYPE_CASE(XCB_DESTROY_NOTIFY);
        ADD_TYPE_CASE(XCB_UNMAP_NOTIFY);
        ADD_TYPE_CASE(XCB_MAP_NOTIFY);
        ADD_TYPE_CASE(XCB_MAP_REQUEST);
        ADD_TYPE_CASE(XCB_CONFIGURE_NOTIFY);
        ADD_TYPE_CASE(XCB_CONFIGURE_REQUEST);
        ADD_TYPE_CASE(XCB_PROPERTY_NOTIFY);
        ADD_TYPE_CASE(XCB_CLIENT_MESSAGE);
        ADD_TYPE_CASE(XCB_GE_GENERIC);
        default: return "unknown event";

    }
}
int catchError(xcb_void_cookie_t cookie){
    xcb_generic_error_t* e=xcb_request_check(dis,cookie);
    if(e){
        logError(e);
        //TODO finish
        switch(e->major_code){
            case BadAccess:
                exit(1);
                break;
            case BadWindow:
                break;
        }
        free(e);
    }
    return e?1:0;
}
void logError(xcb_generic_error_t* e){
    LOG(LOG_LEVEL_ERROR,"error occured with resource %d %d %d\n",e->resource_id,e->major_code,e->minor_code);

    int size=256;
    char buff[size];
    XGetErrorText(dpy,e->error_code,buff,size);
    dumpWindowInfo(getWindowInfo(e->resource_id));
    LOG(LOG_LEVEL_ERROR,"Error code %d %s \n",
               e->error_code,buff) ;

    if(CRASH_ON_ERRORS)
        assert(0 && "Unexpected error");


}
