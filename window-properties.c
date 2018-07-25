#include <assert.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>

#include "window-properties.h"
#include "wmfunctions.h"
#include "defaults.h"




void loadClassInfo(WindowInfo*info){
    int win=info->id;
    xcb_icccm_get_wm_class_reply_t prop;
    xcb_get_property_cookie_t cookie = xcb_icccm_get_wm_class(dis, win);
    if(!xcb_icccm_get_wm_class_reply(dis,cookie, &prop, NULL))
        return;
    if(info->className)
        free(info->className);
    if(info->instanceName)
        free(info->instanceName);

    info->className=calloc(1+(strlen(prop.class_name)),sizeof(char));
    info->instanceName=calloc(1+(strlen(prop.instance_name)),sizeof(char));

    strcpy(info->className, prop.class_name);
    strcpy(info->instanceName, prop.instance_name);
    xcb_icccm_get_wm_class_reply_wipe(&prop);
    LOG(LOG_LEVEL_DEBUG,"class name %s instance name: %s \n",info->className,info->instanceName);

}
void loadTitleInfo(WindowInfo*winInfo){
    xcb_ewmh_get_utf8_strings_reply_t wtitle;
    xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_name(ewmh, winInfo->id);
    if(winInfo->title)
        free(winInfo->title);
    if (xcb_ewmh_get_wm_name_reply(ewmh, cookie, &wtitle, NULL)) {
        winInfo->title=calloc(1+wtitle.strings_len,sizeof(char));
        memcpy(winInfo->title, wtitle.strings, wtitle.strings_len*sizeof(char));
        xcb_ewmh_get_utf8_strings_reply_wipe(&wtitle);
    }
    else {
        xcb_icccm_get_text_property_reply_t icccmName;
        cookie =xcb_icccm_get_wm_name(dis, winInfo->id);
        if (xcb_icccm_get_wm_name_reply(dis, cookie, &icccmName, NULL)) {
            winInfo->title=calloc(1+icccmName.name_len,sizeof(char));
            memcpy(winInfo->title, icccmName.name, icccmName.name_len*sizeof(char));
            xcb_icccm_get_text_property_reply_wipe(&icccmName);
        }
    }

    if(winInfo->title)
        LOG(LOG_LEVEL_DEBUG,"window title %s\n",winInfo->title);
}
void loadWindowType(WindowInfo *winInfo){
    LOG(LOG_LEVEL_TRACE,"loading type\n");
    xcb_ewmh_get_atoms_reply_t name;
    if(xcb_ewmh_get_wm_window_type_reply(ewmh,
            xcb_ewmh_get_wm_window_type(ewmh, winInfo->id), &name, NULL)){
        winInfo->type=name.atoms[0];
        xcb_ewmh_get_atoms_reply_wipe(&name);
    }
    else{
        winInfo->implicitType=1;
        winInfo->type=winInfo->transientFor?ewmh->_NET_WM_WINDOW_TYPE_DIALOG:ewmh->_NET_WM_WINDOW_TYPE_NORMAL;
    }

    xcb_get_atom_name_reply_t *reply=xcb_get_atom_name_reply(
            dis, xcb_get_atom_name(dis, winInfo->type), NULL);

    if(!reply)return;
    if(winInfo->typeName)
        free(winInfo->typeName);

    winInfo->typeName=calloc(reply->name_len+1,sizeof(char));

    memcpy(winInfo->typeName, xcb_get_atom_name_name(reply), reply->name_len*sizeof(char));

    LOG(LOG_LEVEL_TRACE,"window type %s\n",winInfo->typeName);
    free(reply);
}
void loadWindowHints(WindowInfo *winInfo){
    assert(winInfo);
    xcb_icccm_wm_hints_t hints;
    if(xcb_icccm_get_wm_hints_reply(dis, xcb_icccm_get_wm_hints(dis, winInfo->id), &hints, NULL)){
        winInfo->groupId=hints.window_group;
        winInfo->input=hints.input;
        winInfo->state=hints.initial_state;
    }
    if(winInfo->mapped && winInfo->state ==XCB_ICCCM_WM_STATE_WITHDRAWN)
        winInfo->state=XCB_ICCCM_WM_STATE_NORMAL;

}
void loadWindowProperties(WindowInfo *winInfo){
    if(!winInfo)
        return;

    loadClassInfo(winInfo);
    loadTitleInfo(winInfo);
    loadWindowHints(winInfo);
    loadWindowType(winInfo);

    xcb_window_t prop;
    if(xcb_icccm_get_wm_transient_for_reply(dis,
            xcb_icccm_get_wm_transient_for(dis, winInfo->id), &prop, NULL)){
        winInfo->transientFor=prop;
    }
    //dumpWindowInfo(winInfo);
}

int getSavedWorkspaceIndex(xcb_window_t win){
    unsigned int workspaceIndex=NO_WORKSPACE;

    if ((xcb_ewmh_get_wm_desktop_reply(ewmh,
              xcb_ewmh_get_wm_desktop(ewmh, win), &workspaceIndex, NULL))) {
        if(workspaceIndex>=getNumberOfWorkspaces())
            workspaceIndex=getNumberOfWorkspaces()-1;
    }
    else workspaceIndex = getActiveWorkspaceIndex();
    return workspaceIndex;
}


int isInNormalWorkspace(int workspaceIndex){
    return workspaceIndex!=ALL_WORKSPACES;
}


int isWindowVisible(WindowInfo* winInfo){
    return winInfo && winInfo->visibile!=VisibilityFullyObscured;
}
void setVisible(WindowInfo* winInfo,int v){
    winInfo->visibile=v;
}
int isActivatable(WindowInfo* winInfo){
    return winInfo->input;
}
int isMappable(WindowInfo* winInfo){
    return winInfo && winInfo->state==XCB_ICCCM_WM_STATE_NORMAL;
}
int isMapped(WindowInfo* winInfo){
    return winInfo && winInfo->mapped;
}
int isTileable(WindowInfo*winInfo){
    return isMapped(winInfo) && winInfo->type==ewmh->_NET_WM_WINDOW_TYPE_NORMAL && !(winInfo->mask & NO_TILE_MASK);
}


int isExternallyResizable(WindowInfo* winInfo){
    return !winInfo || winInfo->mask & EXTERNAL_RESIZE_MASK;
}
int isExternallyMoveable(WindowInfo* winInfo){
    return !winInfo || winInfo->mask & EXTERNAL_RESIZE_MASK;
}

static int hasMask(WindowInfo* win,int mask){
    return win->mask & mask;
}


int getMaxDimensionForWindow(Monitor*m,WindowInfo*winInfo,int height){
    int maxDimMask=height?Y_MAXIMIZED_MASK:X_MAXIMIZED_MASK;
    if(winInfo->mask & (maxDimMask|PRIVILGED_MASK)){
        if(winInfo->mask&PRIVILGED_MASK)
            return winInfo->mask&ROOT_FULLSCREEN_MASK?(&screen->width_in_pixels)[height]:(&m->width)[height];
        else return (&m->viewWidth)[height];
    }
    else return 0;
}
int getWindowMask(WindowInfo*info){
    return info->mask;
}
int getNumberOfTileableWindows(Node*windowStack){

    int size=0;
    FOR_EACH(windowStack,
            if(isTileable(getValue(windowStack)))size++);
    return size;
}

void updateState(WindowInfo*winInfo,int mask,MaskAction action){
    if(action==TOGGLE)
        action=hasMask(winInfo,mask)?REMOVE:ADD;
}
void addState(WindowInfo*winInfo,int mask){
    int layer=NO_LAYER;
    if(mask &STICKY_MASK){
        layer=Above;
    }
    if(mask &MINIMIZED_MASK){
        winInfo->minimizedTimeStamp=getTime();
        xcb_unmap_window(dis,winInfo->id);
    }
    if(mask &X_MAXIMIZED_MASK ){}
    if(mask &FULLSCREEN_MASK){}
    if(mask &EXTERNAL_RESIZE_MASK){}
    //if(layer!=NO_LAYER)moveWindowToLayerForAllWorkspaces(winInfo, layer);

    winInfo->mask=mask;
}
void removeState(WindowInfo*winInfo,int mask){
//TODO implement
}
