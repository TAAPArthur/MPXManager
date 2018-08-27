

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

extern xcb_connection_t *dis;

extern Display *dpy;
int LOG_LEVEL=LOG_LEVEL_ERROR;

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
void dumpAllWindowInfo(void){
    Node*n=getAllWindows();
    FOR_EACH(n,dumpWindowInfo(getValue(n)));
}

void dumpWindowInfo(WindowInfo* win){
    if(!win)return;

    LOG(LOG_LEVEL_DEBUG,"Dumping window info %d(%#x) group: %d(%#x) Transient for %d\n",win->id,win->id,win->groupId,win->groupId,win->transientFor);
    LOG(LOG_LEVEL_DEBUG,"Lables class %s (%s)\n",win->className,win->instanceName);
    LOG(LOG_LEVEL_DEBUG,"Type is %d (%s) implicit: %d\n",win->type,win->typeName,win->implicitType);

    LOG(LOG_LEVEL_DEBUG,"Title class %s\n",win->title);

    LOG(LOG_LEVEL_DEBUG,"Mask %d\n",win->mask);

    //LOG(level,"mapped %d state: %d visible status: %d\n",win->mapped,win->state,isWindowVisible(win));
    LOG(LOG_LEVEL_DEBUG,"last active workspace %d\n",win->workspaceIndex);
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
            case BadWindow:
                break;
        }
        free(e);
    }
    return e?1:0;
}
void logError(xcb_generic_error_t* e){
    LOG(LOG_LEVEL_ERROR,"error occured with resource %d %d %d\n\n",e->resource_id,e->major_code,e->minor_code);


    int size=256;
    char buff[size];
    XGetErrorText(dpy,e->error_code,buff,size);
    LOG(LOG_LEVEL_ERROR,"Error code %d %s \n",
               e->error_code,buff) ;

    if(CRASH_ON_ERRORS)
        assert(0 && "Unexpected error");


}
