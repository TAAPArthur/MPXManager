/**
 * @file window-clone.c
 * @copybrief window-clone.h
 *
 */
#include <assert.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xtest.h>

#include "bindings.h"
#include "default-rules.h"
#include "devices.h"
#include "events.h"
#include "globals.h"
#include "logger.h"
#include "window-clone.h"
#include "windows.h"
#include "wmfunctions.h"
#include "xsession.h"

WindowInfo* cloneWindowInfo(WindowID id, WindowInfo* winInfo){
    WindowInfo* clone = malloc(sizeof(WindowInfo));
    memcpy(clone, winInfo, sizeof(WindowInfo));
    clearList(getClonesOfWindow(clone));
    clone->id = id;
    clone->cloneOrigin = winInfo->id;
    addToList(getClonesOfWindow(winInfo), clone);
    char** fields = &winInfo->typeName;
    for(int i = 0; i < 4; i++)
        if(fields[i])
            fields[i] = strcpy(calloc(1 + (strlen(fields[i])), sizeof(char)), fields[i]);
    return clone;
}

WindowInfo* cloneWindow(WindowInfo* winInfo){
    assert(winInfo);
    LOG(LOG_LEVEL_DEBUG, "Cloning window %d\n", winInfo->id);
    uint32_t mask = XCB_CW_EVENT_MASK;
    uint32_t values[1] = {XCB_EVENT_MASK_EXPOSURE};
    WindowID window = xcb_generate_id(dis);
    xcb_void_cookie_t cookie = xcb_create_window_checked(dis,
                               XCB_COPY_FROM_PARENT, window, screen->root,
                               0, 0, 10, 10, 0,
                               XCB_WINDOW_CLASS_INPUT_OUTPUT,
                               screen->root_visual,
                               mask, values);
    catchError(cookie);
    catchError(xcb_map_window_checked(dis, window));
    WindowInfo* clone = cloneWindowInfo(window, winInfo);
    processNewWindow(clone);
    addToList(getClonesOfWindow(winInfo), clone);
    int masks = NON_ROOT_DEVICE_EVENT_MASKS;
    passiveGrab(winInfo->id, masks | XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS | XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE |
                XCB_INPUT_XI_EVENT_MASK_KEY_PRESS | XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE);
    passiveGrab(clone->id, masks | XCB_INPUT_XI_EVENT_MASK_ENTER);
    XSelectInput(dpy, winInfo->id, XCB_EVENT_MASK_EXPOSURE);
    return clone;
}
int getCloneOrigin(WindowInfo* winInfo){
    return winInfo->cloneOrigin;
}

void updateClone(WindowInfo* winInfo, WindowInfo* dest){
    assert(winInfo);
    assert(dest);
    short* dims = getGeometry(dest);
    assert(isNotEmpty(getAllWindows()));
    LOG(LOG_LEVEL_DEBUG, "copying %d to %d\n", winInfo->id, dest->id);
    xcb_void_cookie_t cookie = xcb_copy_area_checked(dis, winInfo->id, dest->id, graphics_context,
                               dest->cloneOffset[0], dest->cloneOffset[1], 0, 0, dims[2], dims[3]);
    catchError(cookie);
}

void updateAllClones(WindowInfo* winInfo){
    if(winInfo){
        FOR_EACH(WindowInfo*, clone, getClonesOfWindow(winInfo)) updateClone(winInfo, clone);
    }
}
void swapWithOriginal(void){
    xcb_input_enter_event_t* event = getLastEvent();
    WindowInfo* winInfo = getWindowInfo(event->event);
    if(winInfo && winInfo->cloneOrigin){
        WindowInfo* origin = getWindowInfo(winInfo->cloneOrigin);
        if(origin){
            LOG(LOG_LEVEL_TRACE, "swaping clone %d with %d\n", winInfo->id, origin ? origin->id : 0);
            swapWindows(origin, winInfo);
        }
    }
}
void onExpose(void){
    xcb_expose_event_t* event = getLastEvent();
    LOG(LOG_LEVEL_DEBUG, "expose event received %d\n", event->window);
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo){
        LOG(LOG_LEVEL_DEBUG, "updating %d clones \n", getSize(getClonesOfWindow(winInfo)));
        if(winInfo->cloneOrigin)
            updateClone(getWindowInfo(winInfo->cloneOrigin), winInfo);
        FOR_EACH(WindowInfo*, clone, getClonesOfWindow(winInfo)) updateClone(winInfo, clone);
    }
}
void* autoUpdateClones(void){
    assert(CLONE_REFRESH_RATE);
    while(!isShuttingDown()){
        lock();
        FOR_EACH_REVERSED(WindowInfo*, winInfo, getAllWindows()){
            updateAllClones(winInfo);
            int origin = getCloneOrigin(winInfo);
            if(origin && !getWindowInfo(origin))removeWindow(winInfo->id);
        }
        unlock();
        msleep(CLONE_REFRESH_RATE);
    }
    return NULL;
}
void addCloneRules(void){
    static Rule onExposeRule = CREATE_DEFAULT_EVENT_RULE(onExpose);
    static Rule swapWithOriginalRule = CREATE_DEFAULT_EVENT_RULE(swapWithOriginal);
    addToList(getEventRules(XCB_EXPOSE), &onExposeRule);
    //addToList(getEventRules(XCB_INPUT_MOTION + GENERIC_EVENT_OFFSET), &swapWithOriginalRule);
    addToList(getEventRules(XCB_INPUT_ENTER + GENERIC_EVENT_OFFSET), &swapWithOriginalRule);
}
