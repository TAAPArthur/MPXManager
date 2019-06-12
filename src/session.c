/**
 * @file session.c
 * @brief Save/load MPXManager state
 */
#include <limits.h>
#include <assert.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>


#include "session.h"
#include "devices.h"
#include "layouts.h"
#include "logger.h"
#include "masters.h"
#include "monitors.h"
#include "windows.h"
#include "wmfunctions.h"
#include "workspaces.h"
#include "xsession.h"

void syncState(){
    WorkspaceID currentWorkspace = DEFAULT_WORKSPACE_INDEX;
    if(!xcb_ewmh_get_current_desktop_reply(ewmh,
                                           xcb_ewmh_get_current_desktop(ewmh, defaultScreenNumber),
                                           &currentWorkspace, NULL)){
        currentWorkspace = DEFAULT_WORKSPACE_INDEX;
    }
    if(currentWorkspace >= getNumberOfWorkspaces())
        currentWorkspace = getNumberOfWorkspaces() - 1;
    xcb_ewmh_set_number_of_desktops(ewmh, defaultScreenNumber, getNumberOfWorkspaces());
    unsigned int value = 0;
    xcb_ewmh_get_showing_desktop_reply(ewmh, xcb_ewmh_get_showing_desktop(ewmh, defaultScreenNumber), &value, NULL);
    setShowingDesktop(value);
    LOG(LOG_LEVEL_INFO, "Current workspace is %d; default %d\n\n", currentWorkspace, DEFAULT_WORKSPACE_INDEX);
    switchToWorkspace(currentWorkspace);
}

void loadCustomState(void){
    xcb_get_property_cookie_t cookie;
    xcb_get_property_reply_t* reply;
    LOG(LOG_LEVEL_TRACE, "Loading active layouts\n");
    cookie = xcb_get_property(dis, 0, root, WM_WORKSPACE_LAYOUT_NAMES, ewmh->UTF8_STRING, 0, UINT_MAX);
    if((reply = xcb_get_property_reply(dis, cookie, NULL))){
        int len = xcb_get_property_value_length(reply);
        char* strings = xcb_get_property_value(reply);
        int index = 0, count = 0;
        while(index < len){
            setActiveLayoutOfWorkspaceByName(&strings[index], getWorkspaceByIndex(count++));
            index += strlen(&strings[index]) + 1;
        }
    }
    free(reply);
    LOG(LOG_LEVEL_TRACE, "Loading Workspace layout offsets\n");
    cookie = xcb_get_property(dis, 0, root, WM_WORKSPACE_LAYOUT_INDEXES, XCB_ATOM_CARDINAL, 0, UINT_MAX);
    if((reply = xcb_get_property_reply(dis, cookie, NULL)))
        for(int i = 0; i < xcb_get_property_value_length(reply) / sizeof(int); i++)
            getWorkspaceByIndex(i)->layoutOffset = ((int*)xcb_get_property_value(reply))[i];
    free(reply);
    LOG(LOG_LEVEL_TRACE, "Loading Workspace monitor mappings\n");
    cookie = xcb_get_property(dis, 0, root, WM_WORKSPACE_MONITORS, XCB_ATOM_CARDINAL, 0, UINT_MAX);
    if((reply = xcb_get_property_reply(dis, cookie, NULL))){
        for(int i = 0; i < xcb_get_property_value_length(reply) / sizeof(int); i++){
            Monitor* m = find(getAllMonitors(), &((MonitorID*)xcb_get_property_value(reply))[i], sizeof(MonitorID));
            if(m){
                Workspace* w = getWorkspaceFromMonitor(m);
                if(w)
                    swapWorkspaces(i, w->id);
            }
        }
    }
    free(reply);
    LOG(LOG_LEVEL_TRACE, "Loading Workspace window stacks\n");
    cookie = xcb_get_property(dis, 0, root, WM_WORKSPACE_WINDOWS, XCB_ATOM_CARDINAL, 0, UINT_MAX);
    if((reply = xcb_get_property_reply(dis, cookie, NULL))){
        int index = 0;
        WindowID* wid = xcb_get_property_value(reply);
        for(int i = 0; i < xcb_get_property_value_length(reply) / sizeof(int); i++)
            if(wid[i] == 0)
                index++;
            else if(getWindowInfo(wid[i])){
                moveWindowToWorkspace(getWindowInfo(wid[i]), index);
                assert(getLast(getWindowStack(getWorkspaceByIndex(index))) == getWindowInfo(wid[i]));
            }
    }
    free(reply);
    LOG(LOG_LEVEL_TRACE, "Loading Master window stacks\n");
    cookie = xcb_get_property(dis, 0, root, WM_MASTER_WINDOWS, XCB_ATOM_CARDINAL, 0, UINT_MAX);
    if((reply = xcb_get_property_reply(dis, cookie, NULL))){
        WindowID* wid = xcb_get_property_value(reply);
        for(int i = 0; i < xcb_get_property_value_length(reply) / sizeof(int); i++)
            if(wid[i] == 0){
                setActiveMasterByDeviceId(wid[++i]);
                shiftToHead(getAllMasters(), indexOf(getAllMasters(), getActiveMaster(), 0));
            }
            else
                onWindowFocus(wid[i]);
    }
    free(reply);
    FOR_EACH(Master*, master, getAllMasters()){
        setActiveMaster(master);
        onWindowFocus(getActiveFocus(master->id));
    }
    LOG(LOG_LEVEL_TRACE, "Loading active Master\n");
    cookie = xcb_get_property(dis, 0, root, WM_ACTIVE_MASTER, XCB_ATOM_CARDINAL, 0, sizeof(MasterID));
    if((reply = xcb_get_property_reply(dis, cookie, NULL)) && xcb_get_property_value_length(reply)){
        setActiveMasterByDeviceId(*(MasterID*)xcb_get_property_value(reply));
    }
    free(reply);
}
void saveCustomState(void){
    int layoutOffsets[getNumberOfWorkspaces()];
    int monitors[getNumberOfWorkspaces()];
    WindowID windows[getSize(getAllWindows()) + 2 * getNumberOfWorkspaces()];
    WindowID masterWindows[(getSize(getAllWindows()) + 2) * getSize(getAllMasters())];
    int numWindows = 0;
    int numMasterWindows = 0;
    int len = 0;
    int bufferSize = 64;
    char* activeLayoutNames = malloc(bufferSize);
    FOR_EACH_REVERSED(Master*, master, getAllMasters()){
        masterWindows[numMasterWindows++] = 0;
        masterWindows[numMasterWindows++] = master->id;
        FOR_EACH_REVERSED(WindowInfo*, winInfo, getWindowStackByMaster(master)){
            masterWindows[numMasterWindows++] = winInfo->id;
        }
    }
    for(int i = 0; i < getNumberOfWorkspaces(); i++){
        FOR_EACH(WindowInfo*, winInfo, getWindowStack(getWorkspaceByIndex(i))){
            windows[numWindows++] = winInfo->id;
        }
        windows[numWindows++] = 0;
        Monitor* m = getMonitorFromWorkspace(getWorkspaceByIndex(i));
        monitors[i] = m ? m->id : 0;
        layoutOffsets[i] = getWorkspaceByIndex(i)->layoutOffset;
        char* name = getNameOfLayout(getActiveLayoutOfWorkspace((i)));
        if(bufferSize < len + (name ? strlen(name) : 0) + 2)
            activeLayoutNames = realloc(activeLayoutNames, bufferSize *= 2);
        if(name){
            strcpy(&activeLayoutNames[len], name);
            len += strlen(name) + 1;
        }
        else
            activeLayoutNames[len++] = 0;
    }
    xcb_atom_t REPLACE = XCB_PROP_MODE_REPLACE;
    xcb_change_property(dis, REPLACE, root, WM_WORKSPACE_LAYOUT_NAMES, ewmh->UTF8_STRING, 8, len, activeLayoutNames);
    xcb_change_property(dis, REPLACE, root, WM_WORKSPACE_LAYOUT_INDEXES, XCB_ATOM_CARDINAL, 32,  LEN(layoutOffsets),
                        layoutOffsets);
    xcb_change_property(dis, REPLACE, root, WM_WORKSPACE_WINDOWS, XCB_ATOM_CARDINAL, 32, numWindows, windows);
    xcb_change_property(dis, REPLACE, root, WM_WORKSPACE_MONITORS, XCB_ATOM_CARDINAL, 32, LEN(monitors), monitors);
    xcb_change_property(dis, REPLACE, root, WM_MASTER_WINDOWS, XCB_ATOM_CARDINAL, 32, numMasterWindows, masterWindows);
    xcb_change_property(dis, REPLACE, root, WM_ACTIVE_MASTER, XCB_ATOM_CARDINAL, 32, 1, getActiveMaster());
    free(activeLayoutNames);
    flush();
}
