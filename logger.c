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
#include "logger.h"
#include "wmfunctions.h"


int LOG_LEVEL=LOG_LEVEL_INFO;

extern xcb_connection_t *dis;

int getLogLevel(){
    return LOG_LEVEL;
}
void setLogLevel(int level){
    LOG(LOG_LEVEL_NONE,"Changing log level from %d to %d \n",LOG_LEVEL,level);
    LOG_LEVEL=level;
}

void dumpWindowInfo(WindowInfo* win){
    if(!win)return;
    const int level=LOG_LEVEL_DEBUG;
    LOG(level,"Dumping window info %d(%#x) group: %d(%#x)\n",win->id,win->id,win->groupId,win->groupId);
    LOG(level,"Type is %d (%s) implicit: %d\n",win->type,win->typeName,win->implicitType);
    LOG(level,"Lables class %s (%s)\n",win->className,win->instanceName);
    LOG(level,"Title class %s\n",win->title);

    LOG(level,"Transient for %d\n",win->transientFor);
    LOG(level,"Mask %d; Input %d Override redirect %d\n",win->mask,win->input,win->overrideRedirect);

    LOG(level,"mapped %d state: %d visible status: %d\n",win->mapped,win->state,isVisible(win));
    LOG(level,"last active workspace %d\n",win->workspaceIndex);
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
