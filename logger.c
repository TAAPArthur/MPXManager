/*
 * logger.c
 *
 *  Created on: Jul 8, 2018
 *      Author: arthur
 */

#include <assert.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI.h>

#include "mywm-structs.h"
#include "mywm-util.h"
#include "defaults.h"
#include "logger.h"
#include "window-properties.h"

extern xcb_connection_t *dis;

extern Display *dpy;
int LOG_LEVEL=LOG_LEVEL_WARN;

int getLogLevel(){
    return LOG_LEVEL;
}
void setLogLevel(int level){
    LOG(LOG_LEVEL_NONE,"Changing log level from %d to %d \n",LOG_LEVEL,level);
    LOG_LEVEL=level;
}
void printNodeList(Node*n){
    printf("Printing list: \n");
    FOR_EACH(n,printf("%d ",getIntValue(n)));
    printf("\n");
}
void dumpAllWindowInfo(){
    Node*n=getAllWindows();
    FOR_EACH(n,dumpWindowInfo(getValue(n)));
}

void dumpWindowInfo(WindowInfo* win){
    if(!win)return;
    const int level=LOG_LEVEL_DEBUG;
    LOG(LOG_LEVEL_DEBUG,"Dumping window info %d(%#x) group: %d(%#x)\n",win->id,win->id,win->groupId,win->groupId);
    LOG(LOG_LEVEL_DEBUG,"Lables class %s (%s)\n",win->className,win->instanceName);
    LOG(LOG_LEVEL_DEBUG,"Type is %d (%s) implicit: %d\n",win->type,win->typeName,win->implicitType);

    LOG(LOG_LEVEL_DEBUG,"Title class %s\n",win->title);

    LOG(LOG_LEVEL_TRACE,"Transient for %d\n",win->transientFor);
    LOG(LOG_LEVEL_TRACE,"Mask %d; Input %d Override redirect %d\n",win->mask,win->input,win->overrideRedirect);

    //LOG(level,"mapped %d state: %d visible status: %d\n",win->mapped,win->state,isWindowVisible(win));
    LOG(LOG_LEVEL_TRACE,"last active workspace %d\n",win->workspaceIndex);
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

char *genericEventTypeToString(int type){

    //type+=GENERIC_EVENT_OFFSET;
    switch(type) {

     case XI_KeyPress: return "XI_KeyPress";
     case XI_KeyRelease: return "XI_KeyRelease";
     case XI_ButtonPress: return "XI_ButtonPress";
     case XI_ButtonRelease: return "XI_ButtonRelease";
     case XI_Motion: return "XI_Motion";
     case XI_Enter: return "XI_Enter";
     case XI_Leave: return "XI_Leave";
     case XI_FocusIn: return "XI_FocusIn";
     case XI_FocusOut: return "XI_FocusOut";
     case XI_HierarchyChanged: return "XI_HierarchyChanged";
     case XI_PropertyEvent: return "XI_PropertyEvent";
         default:
             return "undef XI event (%d)";
     }
}
char *eventTypeToString(int type){
    switch(type) {

    case GenericEvent: return "GenericEvent";

    case Expose: return "Expose";

    case VisibilityNotify: return "VisibilityNotify";
    case CreateNotify: return "CreateNotify";
    case DestroyNotify: return "DestroyNotify";
    case UnmapNotify: return "UnmapNotify";
    case MapNotify: return "MapNotify";
    case MapRequest: return "MapRequest";
    case ReparentNotify: return "ReparentNotify";
    case ConfigureNotify: return "ConfigureNotify";
    case ConfigureRequest: return "ConfigureRequest";
    case PropertyNotify: return "PropertyNotify";
    case ClientMessage: return "ClientMessage";
    case MappingNotify: return "MappingNotify";
        default: return "unknown event";
    }
}
int checkError(xcb_void_cookie_t cookie,int cause,char*msg){
    xcb_generic_error_t* e=xcb_request_check(dis,cookie);
    if(e){
        LOG(LOG_LEVEL_ERROR,"error occured with window %d %s\n\n",cause,msg);
        logError(e);
        free(e);
        assert(1==2);
    }
    return e?1:0;
}
void logError(xcb_generic_error_t* e){
    LOG(LOG_LEVEL_ERROR,"error occured with window %d %d\n\n",e->resource_id,e->major_code);
    int size=256;
    char buff[size];
    XGetErrorText(dpy,e->error_code,buff,size);
    LOG(LOG_LEVEL_ERROR,"Error code %d %s \n",
               e->error_code,buff) ;

}
