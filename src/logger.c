

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
#include "masters.h"
#include "workspaces.h"
#include "monitors.h"

extern xcb_connection_t* dis;

extern Display* dpy;

#ifndef INIT_LOG_LEVEL
    #define INIT_LOG_LEVEL LOG_LEVEL_INFO
#endif

int LOG_LEVEL = INIT_LOG_LEVEL;

int getLogLevel(){
    return LOG_LEVEL;
}
void setLogLevel(int level){
    LOG_LEVEL = level;
}
void printArrayList(ArrayList* n){
    LOG(LOG_LEVEL_INFO, "Printing list: \n");
    FOR_EACH(void*, v, n) LOG(LOG_LEVEL_INFO, "%d ", *(int*)v);
    LOG(LOG_LEVEL_INFO, "\n");
}
#define PRINT_MASK(str,mask) if( (str & mask)||(str==0 &&mask==0))LOG(LOG_LEVEL_INFO,#str " ");
void printMask(int mask){
    PRINT_MASK(NO_MASK, mask);
    PRINT_MASK(X_MAXIMIZED_MASK, mask);
    PRINT_MASK(Y_MAXIMIZED_MASK, mask);
    PRINT_MASK(FULLSCREEN_MASK, mask);
    PRINT_MASK(ROOT_FULLSCREEN_MASK, mask);
    PRINT_MASK(BELOW_MASK, mask);
    PRINT_MASK(ABOVE_MASK, mask);
    PRINT_MASK(NO_TILE_MASK, mask);
    PRINT_MASK(STICKY_MASK, mask);
    PRINT_MASK(HIDDEN_MASK, mask);
    PRINT_MASK(EXTERNAL_RESIZE_MASK, mask);
    PRINT_MASK(EXTERNAL_MOVE_MASK, mask);
    PRINT_MASK(EXTERNAL_BORDER_MASK, mask);
    PRINT_MASK(EXTERNAL_RAISE_MASK, mask);
    PRINT_MASK(FLOATING_MASK, mask);
    PRINT_MASK(SRC_INDICATION_OTHER, mask);
    PRINT_MASK(SRC_INDICATION_APP, mask);
    PRINT_MASK(SRC_INDICATION_PAGER, mask);
    PRINT_MASK(SRC_ANY, mask);
    PRINT_MASK(INPUT_MASK, mask);
    PRINT_MASK(WM_TAKE_FOCUS_MASK, mask);
    PRINT_MASK(WM_DELETE_WINDOW_MASK, mask);
    PRINT_MASK(WM_PING_MASK, mask);
    PRINT_MASK(PARTIALLY_VISIBLE, mask);
    PRINT_MASK(FULLY_VISIBLE, mask);
    PRINT_MASK(MAPPABLE_MASK, mask);
    PRINT_MASK(MAPPED_MASK, mask);
    PRINT_MASK(URGENT_MASK, mask);
    LOG(LOG_LEVEL_INFO, "\n");
}
void printMasterSummary(void){
    LOG(LOG_LEVEL_INFO, "Active Master %d\n", getActiveMaster()->id);
    FOR_EACH(Master*, master, getAllMasters()){
        LOG(LOG_LEVEL_INFO, "(%d, %d: %s %06x) ", master->id, master->pointerId,
            master->name, master->focusColor);
    }
    LOG(LOG_LEVEL_INFO, "\n");
}
void printMonitorSummary(void){
    FOR_EACH(Monitor*, m, getAllMonitors()){
        LOG(LOG_LEVEL_INFO, "Monitor %ld primary %d ", m->id, m->primary);
        PRINT_ARR("Base", &m->base.x, 4, " ");
        PRINT_ARR("View", &m->view.x, 4, "\n");
    }
    LOG(LOG_LEVEL_INFO, "\n");
}
void printWindowStackSummary(ArrayList* list){
    FOR_EACH(WindowInfo*, winInfo, list){
        LOG(LOG_LEVEL_INFO, "(%d: %s)%s ", winInfo->id, winInfo->title,
            isTileable(winInfo) ? "*" : " ");
    }
    LOG(LOG_LEVEL_INFO, "\n");
}
void printSummary(void){
    LOG(LOG_LEVEL_INFO, "Summary:\n");
    printMasterSummary();
    printMonitorSummary();
    ArrayList* list = getActiveMasterWindowStack();
    if(isNotEmpty(list)){
        LOG(LOG_LEVEL_INFO, "Active master window stack:\n");
        FOR_EACH(WindowInfo*, winInfo, list){
            LOG(LOG_LEVEL_INFO, "(%d: %s) ", winInfo->id, winInfo->title);
        }
    }
    LOG(LOG_LEVEL_INFO, "\n");
    for(int i = 0; i < getNumberOfWorkspaces(); i++){
        ArrayList* list = getWindowStack(getWorkspaceByIndex(i));
        if(!isNotEmpty(list))continue;
        LOG(LOG_LEVEL_INFO, "Workspace: %d \n", i);
        printWindowStackSummary(list);
    }
    if(isNotEmpty(getAllDocks())){
        LOG(LOG_LEVEL_INFO, "%d Docks: \n", getSize(getAllDocks()));
        printWindowStackSummary(getAllDocks());
    }
}
void dumpAllWindowInfo(int filterMask){
    FOR_EACH(WindowInfo*, winInfo, getAllWindows()){
        if(!filterMask ||  hasMask(winInfo, filterMask))
            dumpWindowInfo(winInfo);
    }
}

void dumpWindowInfo(WindowInfo* win){
    if(!win)return;
    LOG(LOG_LEVEL_INFO, "Dumping window info %d(%#x) group: %d(%#x) Transient for %d(%#x)\n", win->id, win->id,
        win->groupId, win->groupId, win->transientFor, win->transientFor);
    LOG(LOG_LEVEL_INFO, "Labels class %s (%s)\n", win->className, win->instanceName);
    LOG(LOG_LEVEL_INFO, "Parent %d,Type is %d (%s) implicit: %d Dock %d\n", win->parent, win->type, win->typeName,
        win->implicitType, win->dock);
    LOG(LOG_LEVEL_INFO, "Title class %s\n", win->title);
    LOG(LOG_LEVEL_INFO, "Mask %d\n", win->mask);
    printMask(win->mask);
    LOG(LOG_LEVEL_INFO, "Workspace %d\n", win->workspaceIndex);
    LOG(LOG_LEVEL_INFO, "\n");
}

void dumpAtoms(xcb_atom_t* atoms, int numberOfAtoms){
    xcb_get_atom_name_reply_t* reply;
    for(int i = 0; i < numberOfAtoms; i++){
        reply = xcb_get_atom_name_reply(dis, xcb_get_atom_name(dis, atoms[i]), NULL);
        if(!reply)continue;
        LOG(LOG_LEVEL_INFO, "atom: %.*s\n", reply->name_len, xcb_get_atom_name_name(reply));
        free(reply);
    }
}

#define ADD_TYPE_CASE(TYPE) case TYPE: return  #TYPE
char* genericEventTypeToString(int type){
    switch(type){
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

char* eventTypeToString(int type){
    switch(type){
        case 0:
            return "Error";
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
        default:
            return "unknown event";
    }
}
int catchError(xcb_void_cookie_t cookie){
    xcb_generic_error_t* e = xcb_request_check(dis, cookie);
    int errorCode = 0;
    if(e){
        errorCode = e->error_code;
        logError(e);
        free(e);
    }
    return errorCode;
}
int catchErrorSilent(xcb_void_cookie_t cookie){
    xcb_generic_error_t* e = xcb_request_check(dis, cookie);
    int errorCode = 0;
    if(e){
        errorCode = e->error_code;
        free(e);
    }
    return errorCode;
}
void logError(xcb_generic_error_t* e){
    LOG(LOG_LEVEL_ERROR, "error occured with resource %d. Error code: %d %d (%d)\n", e->resource_id, e->error_code,
        e->major_code, e->minor_code);
    int size = 256;
    char buff[size];
    XGetErrorText(dpy, e->error_code, buff, size);
    if(e->error_code != BadWindow)
        dumpWindowInfo(getWindowInfo(e->resource_id));
    LOG(LOG_LEVEL_ERROR, "Error code %d %s \n", e->error_code, buff) ;
    if((1 << e->error_code) & CRASH_ON_ERRORS)
        assert(0 && "Unexpected error");
}
