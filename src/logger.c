/**
 * @file logger.c
 *
 * @copybrief logger.h
 */


#include <execinfo.h>
#include <unistd.h>

#include <xcb/xinput.h>

#include "bindings.h"
#include "events.h"
#include "globals.h"
#include "logger.h"
#include "devices.h"
#include "masters.h"
#include "monitors.h"
#include "mywm-util.h"
#include "windows.h"
#include "workspaces.h"
#include "xsession.h"


#ifndef INIT_LOG_LEVEL
    #define INIT_LOG_LEVEL LOG_LEVEL_INFO
#endif

static int LOG_LEVEL = INIT_LOG_LEVEL;

int getLogLevel(){
    return LOG_LEVEL;
}
void setLogLevel(int level){
    LOG_LEVEL = level;
}

#define _PRINT_MASK(str,mask) if( (str & mask)||(str==0 &&mask==0))LOG(LOG_LEVEL_INFO,#str " ");
void printMask(WindowMask mask){
    _PRINT_MASK(NO_MASK, mask);
    _PRINT_MASK(X_MAXIMIZED_MASK, mask);
    _PRINT_MASK(Y_MAXIMIZED_MASK, mask);
    _PRINT_MASK(FULLSCREEN_MASK, mask);
    _PRINT_MASK(ROOT_FULLSCREEN_MASK, mask);
    _PRINT_MASK(BELOW_MASK, mask);
    _PRINT_MASK(ABOVE_MASK, mask);
    _PRINT_MASK(NO_TILE_MASK, mask);
    _PRINT_MASK(STICKY_MASK, mask);
    _PRINT_MASK(HIDDEN_MASK, mask);
    _PRINT_MASK(EXTERNAL_RESIZE_MASK, mask);
    _PRINT_MASK(EXTERNAL_MOVE_MASK, mask);
    _PRINT_MASK(EXTERNAL_BORDER_MASK, mask);
    _PRINT_MASK(EXTERNAL_RAISE_MASK, mask);
    _PRINT_MASK(FLOATING_MASK, mask);
    _PRINT_MASK(SRC_INDICATION_OTHER, mask);
    _PRINT_MASK(SRC_INDICATION_APP, mask);
    _PRINT_MASK(SRC_INDICATION_PAGER, mask);
    _PRINT_MASK(SRC_ANY, mask);
    _PRINT_MASK(INPUT_ONLY_MASK, mask);
    _PRINT_MASK(INPUT_MASK, mask);
    _PRINT_MASK(WM_TAKE_FOCUS_MASK, mask);
    _PRINT_MASK(WM_DELETE_WINDOW_MASK, mask);
    _PRINT_MASK(WM_PING_MASK, mask);
    _PRINT_MASK(IMPLICIT_TYPE, mask);
    _PRINT_MASK(PARTIALLY_VISIBLE, mask);
    _PRINT_MASK(FULLY_VISIBLE, mask);
    _PRINT_MASK(MAPPABLE_MASK, mask);
    _PRINT_MASK(MAPPED_MASK, mask);
    _PRINT_MASK(URGENT_MASK, mask);
    LOG(LOG_LEVEL_INFO, "\n");
}
void printMasterSummary(void){
    LOG(LOG_LEVEL_INFO, "Active Master %d\n", getActiveMaster()->id);
    FOR_EACH(Master*, master, getAllMasters()){
        LOG(LOG_LEVEL_INFO, "Master %s (%d, %d); Active Focus: %d; Recorded Focus: %d; Focus Color: %06x\n) Focus",
            master->name, master->id, master->pointerId,
            getActiveFocus(master->id), getFocusedWindow() ? getFocusedWindow()->id : 0, master->focusColor);
    }
    LOG(LOG_LEVEL_INFO, "\n");
}
void dumpMonitorInfo(Monitor* m){
    LOG(LOG_LEVEL_INFO, "Monitor %d(%s) primary %d ", m->id, getNameOfMonitor(m), m->primary);
    Workspace* w = getWorkspaceFromMonitor(m);
    if(w)
        LOG(LOG_LEVEL_INFO, "Workspace %d", w->id);
    PRINT_ARR("\nBase", &m->base.x, 4, " ");
    PRINT_ARR("View", &m->view.x, 4, "\n");
}
void printMonitorSummary(void){
    FOR_EACH(Monitor*, m, getAllMonitors()){
        dumpMonitorInfo(m);
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
        printWindowStackSummary(list);
    }
    LOG(LOG_LEVEL_INFO, "\n");
    for(int i = 0; i < getNumberOfWorkspaces(); i++){
        ArrayList* list = getWindowStack(getWorkspaceByIndex(i));
        if(!isNotEmpty(list))continue;
        LOG(LOG_LEVEL_INFO, "Workspace: %d%s\n", i, isWorkspaceVisible(i) ? "*" : "");
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
void dumpMatch(char* match){
    Rule rule = {match, MATCH_ANY_REGEX, NULL};
    FOR_EACH(WindowInfo*, winInfo, getAllWindows()){
        if(doesWindowMatchRule(&rule, winInfo))
            dumpWindowInfo(winInfo);
    }
}

void dumpWindowInfo(WindowInfo* winInfo){
    if(!winInfo)return;
    LOG(LOG_LEVEL_INFO, "Dumping window info %d(%#x) group: %d(%#x) Transient for %d(%#x)\n", winInfo->id, winInfo->id,
        winInfo->groupId, winInfo->groupId, winInfo->transientFor, winInfo->transientFor);
    LOG(LOG_LEVEL_INFO, "Class %s (%s)\n", winInfo->className, winInfo->instanceName);
    LOG(LOG_LEVEL_INFO, "Parent %d; Type is %d (%s); Dock: %d; OverrideRedirect: %d\n", winInfo->parent, winInfo->type,
        winInfo->typeName, isDock(winInfo), isOverrideRedirectWindow(winInfo));
    LOG(LOG_LEVEL_INFO, "Title %s\n", winInfo->title);
    PRINT_ARR("Recorded Geometry", getGeometry(winInfo), 5, "\n");
    LOG(LOG_LEVEL_INFO, "Tiling override %d", winInfo->tilingOverrideEnabled);
    PRINT_ARR(" ", getTilingOverride(winInfo), 5, "\n");
    if(winInfo->dock)
        PRINT_ARR("Properties", winInfo->properties, LEN(winInfo->properties), "\n");
    LOG(LOG_LEVEL_INFO, "Mask %u\n", winInfo->mask);
    printMask(winInfo->mask);
    if(winInfo->workspaceIndex == NO_WORKSPACE)
        LOG(LOG_LEVEL_INFO, "NO WORKPACE\n");
    else
        LOG(LOG_LEVEL_INFO, "Workspace %d\n", winInfo->workspaceIndex);
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

void dumpBoundFunction(BoundFunction* boundFunction){
    LOG(LOG_LEVEL_INFO, "calling function %d\n", boundFunction->type);
    LOG(LOG_LEVEL_INFO, "Arg %d (%p)\n", boundFunction->arg.intArg, boundFunction->arg.voidArg);
    void*    funptr = &boundFunction->func.func;
    backtrace_symbols_fd(funptr, 1, STDERR_FILENO);
}

#define _ADD_EVENT_TYPE_CASE(TYPE) case TYPE: return  #TYPE
#define _ADD_GE_EVENT_TYPE_CASE(TYPE) case GENERIC_EVENT_OFFSET+TYPE: return  #TYPE

char* eventTypeToString(int type){
    switch(type){
            _ADD_EVENT_TYPE_CASE(XCB_EXPOSE);
            _ADD_EVENT_TYPE_CASE(XCB_VISIBILITY_NOTIFY);
            _ADD_EVENT_TYPE_CASE(XCB_CREATE_NOTIFY);
            _ADD_EVENT_TYPE_CASE(XCB_DESTROY_NOTIFY);
            _ADD_EVENT_TYPE_CASE(XCB_UNMAP_NOTIFY);
            _ADD_EVENT_TYPE_CASE(XCB_MAP_NOTIFY);
            _ADD_EVENT_TYPE_CASE(XCB_MAP_REQUEST);
            _ADD_EVENT_TYPE_CASE(XCB_REPARENT_NOTIFY);
            _ADD_EVENT_TYPE_CASE(XCB_CONFIGURE_NOTIFY);
            _ADD_EVENT_TYPE_CASE(XCB_CONFIGURE_REQUEST);
            _ADD_EVENT_TYPE_CASE(XCB_PROPERTY_NOTIFY);
            _ADD_EVENT_TYPE_CASE(XCB_SELECTION_CLEAR);
            _ADD_EVENT_TYPE_CASE(XCB_CLIENT_MESSAGE);
            _ADD_EVENT_TYPE_CASE(XCB_GE_GENERIC);
            _ADD_GE_EVENT_TYPE_CASE(XCB_INPUT_KEY_PRESS);
            _ADD_GE_EVENT_TYPE_CASE(XCB_INPUT_KEY_RELEASE);
            _ADD_GE_EVENT_TYPE_CASE(XCB_INPUT_BUTTON_PRESS);
            _ADD_GE_EVENT_TYPE_CASE(XCB_INPUT_BUTTON_RELEASE);
            _ADD_GE_EVENT_TYPE_CASE(XCB_INPUT_MOTION);
            _ADD_GE_EVENT_TYPE_CASE(XCB_INPUT_ENTER);
            _ADD_GE_EVENT_TYPE_CASE(XCB_INPUT_LEAVE);
            _ADD_GE_EVENT_TYPE_CASE(XCB_INPUT_FOCUS_IN);
            _ADD_GE_EVENT_TYPE_CASE(XCB_INPUT_FOCUS_OUT);
            _ADD_GE_EVENT_TYPE_CASE(XCB_INPUT_HIERARCHY);
            _ADD_EVENT_TYPE_CASE(ExtraEvent);
            _ADD_EVENT_TYPE_CASE(onXConnection);
            _ADD_EVENT_TYPE_CASE(TypeChange);
            _ADD_EVENT_TYPE_CASE(PropertyLoad);
            _ADD_EVENT_TYPE_CASE(RegisteringWindow);
            _ADD_EVENT_TYPE_CASE(onScreenChange);
            _ADD_EVENT_TYPE_CASE(TileWorkspace);
            _ADD_EVENT_TYPE_CASE(onWindowMove);
            _ADD_EVENT_TYPE_CASE(Periodic);
            _ADD_EVENT_TYPE_CASE(Idle);
        case 0:
            return "Error";
        default:
            return "unknown event";
    }
}
char* opcodeToString(int opcode){
    switch(opcode){
            _ADD_EVENT_TYPE_CASE(XCB_CREATE_WINDOW);
            _ADD_EVENT_TYPE_CASE(XCB_CHANGE_WINDOW_ATTRIBUTES);
            _ADD_EVENT_TYPE_CASE(XCB_GET_WINDOW_ATTRIBUTES);
            _ADD_EVENT_TYPE_CASE(XCB_DESTROY_WINDOW);
            _ADD_EVENT_TYPE_CASE(XCB_MAP_WINDOW);
            _ADD_EVENT_TYPE_CASE(XCB_CONFIGURE_WINDOW);
            _ADD_EVENT_TYPE_CASE(XCB_CHANGE_PROPERTY);
        default:
            return "unknown code";
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
    LOG(LOG_LEVEL_ERROR, "error occurred with resource %d. Error code: %d %s (%d %d)\n", e->resource_id, e->error_code,
        opcodeToString(e->major_code), e->major_code, e->minor_code);
    int size = 256;
    char buff[size];
    XGetErrorText(dpy, e->error_code, buff, size);
    if(e->error_code != BadWindow)
        dumpWindowInfo(getWindowInfo(e->resource_id));
    LOG(LOG_LEVEL_ERROR, "Error code %d %s \n", e->error_code, buff) ;
    LOG_RUN(LOG_LEVEL_DEBUG, printStackTrace());
    if((1 << e->error_code) & CRASH_ON_ERRORS){
        LOG(LOG_LEVEL_ERROR, "Crashing on error\n");
        quit(1);
    }
}
void printStackTrace(void){
    void* array[32];
    size_t size;
    // get void*'s for all entries on the stack
    size = backtrace(array, LEN(array));
    // print out all the frames to stderr
    backtrace_symbols_fd(array, size, STDERR_FILENO);
}
