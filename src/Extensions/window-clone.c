/**
 * @file window-clone.c
 * @copybrief window-clone.h
 *
 */
#include <assert.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xtest.h>

#include "../bindings.h"
#include "../default-rules.h"
#include "../devices.h"
#include "../events.h"
#include "../globals.h"
#include "../logger.h"
#include "../mywm-util.h"
#include "../state.h"
#include "../windows.h"
#include "../wmfunctions.h"
#include "../xsession.h"
#include "window-clone.h"

/// Holds meta data mapping a clone to its parent
typedef struct CloneInfo {
    /// ID of the source window
    WindowID parent;
    /// ID of the clone window
    WindowID clone;
    ///x,y offset for cloned windows; they will clone the original starting at this offset
    int cloneOffset[2];
} CloneInfo;
static ArrayList clones;

static void syncTitleWithParent(WindowInfo* clone, WindowInfo* parent){
    if(parent->title)
        xcb_ewmh_set_wm_name(ewmh, clone->id, strlen(parent->title), parent->title);
}
static void syncPropertiesWithParent(WindowInfo* clone, WindowInfo* parent){
    assert(clone != parent);
    syncTitleWithParent(clone, parent);
    if(parent->className && parent->instanceName){
        int classLen = strlen(parent->className) + 1;
        int instanceLen = strlen(parent->instanceName) + 1;
        int size = classLen + instanceLen;
        char* buffer = malloc(size);
        strcpy(buffer, parent->instanceName);
        strcpy(buffer + strlen(buffer) + 1, parent->className);
        xcb_icccm_set_wm_class(dis, clone->id, size, buffer);
        free(buffer);
    }
    if(parent->type)
        xcb_ewmh_set_wm_window_type(ewmh, clone->id, 1, &parent->type);
}
/**
 * Clones a WindowInfo object and sets its id to id
 *
 * @param id    the id of the WindowInfo object
 * @param winInfo    the window to clone
 * @return  pointer to the newly created object
 */
static WindowInfo* cloneWindowInfo(WindowID id, WindowInfo* winInfo){
    WindowInfo* clone = malloc(sizeof(WindowInfo));
    memcpy(clone, winInfo, sizeof(WindowInfo));
    clone->id = id;
    char** fields = &clone->typeName;
    for(int i = 0; i < 4; i++)
        fields[i] = NULL;
    clone->type = 0;
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
    xcb_icccm_wm_hints_t hints = {.input = hasMask(winInfo, INPUT_MASK), .initial_state = XCB_ICCCM_WM_STATE_NORMAL };
    catchError(xcb_icccm_set_wm_hints_checked(dis, window, &hints));
    WindowInfo* clone = cloneWindowInfo(window, winInfo);
    assert(clone->title == NULL);
    syncPropertiesWithParent(clone, winInfo);
    clone->eventMasks |= XCB_EVENT_MASK_EXPOSURE;
    registerWindow(clone);
    passiveGrab(clone->id, NON_ROOT_DEVICE_EVENT_MASKS | XCB_INPUT_XI_EVENT_MASK_ENTER);
    CloneInfo* cloneInfo = calloc(1, sizeof(CloneInfo));
    cloneInfo->parent = winInfo->id;
    cloneInfo->clone = clone->id;
    addToList(&clones, cloneInfo);
    return clone;
}
void killAllClones(WindowInfo* winInfo){
    FOR_EACH_REVERSED(CloneInfo*, cloneInfo, &clones){
        if(cloneInfo->parent == winInfo->id){
            LOG(LOG_LEVEL_TRACE, "Killing clone %d of %d\n", cloneInfo->clone, winInfo->id);
            catchError(xcb_destroy_window_checked(dis, cloneInfo->clone));
        }
    }
}

/**
 * Updates the displayed image for the cloned window
 *
 * @param cloneInfo
 */
static void updateClone(CloneInfo* cloneInfo){
    assert(cloneInfo);
    WindowInfo* dest = getWindowInfo(cloneInfo->clone);
    if(dest){
        short* dims = getGeometry(dest);
        if(getWindowInfo(cloneInfo->clone))
            syncTitleWithParent(dest, getWindowInfo(cloneInfo->parent));
        xcb_copy_area(dis, cloneInfo->parent, dest->id, graphics_context,
                      cloneInfo->cloneOffset[0], cloneInfo->cloneOffset[1], 0, 0, dims[2], dims[3]);
    }
}
static int getParent(WindowID win){
    FOR_EACH_REVERSED(CloneInfo*, cloneInfo, &clones){
        if(win == cloneInfo->clone)
            return cloneInfo->parent;
    }
    return 0;
}
void updateAllClonesOfWindow(WindowID win){
    FOR_EACH_REVERSED(CloneInfo*, cloneInfo, &clones){
        if(win == cloneInfo->parent)
            updateClone(cloneInfo);
    }
}
void updateAllClones(void){
    FOR_EACH_REVERSED(CloneInfo*, cloneInfo, &clones){
        if(!getWindowInfo(cloneInfo->parent) || !getWindowInfo(cloneInfo->clone))
            free(removeElementFromList(&clones, cloneInfo, sizeof(cloneInfo)));
        else
            updateClone(cloneInfo);
    }
}
void* autoUpdateClones(void* arg __attribute__((unused))){
    assert(CLONE_REFRESH_RATE);
    while(!isShuttingDown()){
        ATOMIC(updateAllClones());
        msleep(CLONE_REFRESH_RATE);
    }
    return NULL;
}
void startAutoUpdatingClones(void){
    runInNewThread(autoUpdateClones, NULL, "Auto update window clone");
}

void swapWithOriginalOnEnter(void){
    xcb_input_enter_event_t* event = getLastEvent();
    WindowInfo* winInfo = getWindowInfo(event->event);
    if(winInfo && getParent(winInfo->id)){
        WindowInfo* origin = getWindowInfo(getParent(winInfo->id));
        if(origin && hasMask(origin, MAPPED_MASK) && hasMask(winInfo, MAPPED_MASK)){
            LOG(LOG_LEVEL_DEBUG, "swapping clone %d with %d\n", winInfo->id, origin ? origin->id : 0);
            swapWindows(origin, winInfo);
            markState();
        }
    }
}
void onExpose(void){
    xcb_expose_event_t* event = getLastEvent();
    if(event->count == 0){
        WindowInfo* winInfo = getWindowInfo(event->window);
        if(winInfo){
            WindowID parent = getParent(winInfo->id);
            if(parent)
                updateAllClonesOfWindow(parent);
            updateAllClonesOfWindow(winInfo->id);
        }
    }
}
void focusParent(void){
    xcb_input_focus_in_event_t* event = getLastEvent();
    WindowInfo* winInfo = getWindowInfo(event->event);
    if(winInfo){
        WindowInfo* parent = getWindowInfo(getParent(winInfo->id));
        if(parent)
            focusWindowInfo(parent);
    }
}
void swapOnMapEvent(void){
    xcb_map_notify_event_t* event = getLastEvent();
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo){
        WindowInfo* parent = getWindowInfo(getParent(winInfo->id));
        if(parent && !hasMask(parent, MAPPED_MASK)){
            LOG(LOG_LEVEL_DEBUG, "swapping mapped clone with unmapped parent %d with %d\n", winInfo->id, parent->id);
            swapWindows(parent, winInfo);
        }
    }
}
void swapOnUnmapEvent(void){
    xcb_unmap_notify_event_t* event = getLastEvent();
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo && getParent(winInfo->id) == 0){
        CloneInfo* cloneInfo;
        UNTIL_FIRST(cloneInfo, &clones, cloneInfo->parent == winInfo->id && getWindowInfo(cloneInfo->clone) &&
                    hasMask(getWindowInfo(cloneInfo->clone), MAPPED_MASK));
        if(cloneInfo){
            LOG(LOG_LEVEL_DEBUG, "swapping parent with mapped clone %d with %d\n", winInfo->id, cloneInfo->clone);
            swapWindows(winInfo, getWindowInfo(cloneInfo->clone));
        }
    }
}
void addCloneRules(void){
    static Rule onExposeRule = CREATE_DEFAULT_EVENT_RULE(onExpose);
    static Rule swapWithOriginalRule = CREATE_DEFAULT_EVENT_RULE(swapWithOriginalOnEnter);
    static Rule onMapRule = CREATE_DEFAULT_EVENT_RULE(swapOnMapEvent);
    static Rule onUnmapRule = CREATE_DEFAULT_EVENT_RULE(swapOnUnmapEvent);
    static Rule transferFocusToParent = CREATE_DEFAULT_EVENT_RULE(focusParent);
    addToList(getEventRules(XCB_EXPOSE), &onExposeRule);
    //addToList(getEventRules(XCB_INPUT_MOTION + GENERIC_EVENT_OFFSET), &swapWithOriginalRule);
    addToList(getEventRules(XCB_MAP_NOTIFY), &onMapRule);
    addToList(getEventRules(XCB_UNMAP_NOTIFY), &onUnmapRule);
    addToList(getEventRules(XCB_INPUT_ENTER + GENERIC_EVENT_OFFSET), &swapWithOriginalRule);
    addToList(getEventRules(XCB_INPUT_FOCUS_IN + GENERIC_EVENT_OFFSET), &transferFocusToParent);
}
