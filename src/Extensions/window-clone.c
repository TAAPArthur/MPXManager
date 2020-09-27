#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "../boundfunction.h"
#include "../util/logger.h"
#include "../user-events.h"
#include "../xutil/window-properties.h"
#include "../windows.h"
#include "../wmfunctions.h"
#include "../xutil/xsession.h"
#include "window-clone.h"

xcb_gcontext_t graphics_context;

static void create_graphics_context(void) {
    uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_GRAPHICS_EXPOSURES;
    uint32_t values[3] = { screen->black_pixel, screen->white_pixel, 0};
    graphics_context = xcb_generate_id(dis);
    xcb_void_cookie_t cookie = xcb_create_gc_checked(dis,
            graphics_context, root,
            mask, values);
    catchError(cookie);
}


/// Holds meta data mapping a clone to its parent
typedef struct CloneInfo {
    /// ID of the clone window
    WindowID cloneID;
    /// ID of the original window
    WindowID originalID;
    ///x,y offset for cloned windows; they will clone the original starting at this offset
    int offset[2];
} CloneInfo ;
static ArrayList clones;


CloneInfo* getCloneInfo(WindowInfo* winInfo) {
    FOR_EACH(CloneInfo*, clone, &clones) {
        if(clone->cloneID == winInfo->id)
            return clone;
    }
    return NULL;
}
bool isCloneOf(WindowInfo* clone, WindowInfo* parent) {
    CloneInfo* info = getCloneInfo(clone);
    return info && info->originalID == parent->id;
}

WindowInfo* getParent(WindowInfo* clone) {
    CloneInfo* info = getCloneInfo(clone);
    return info ? getWindowInfo(info->originalID) : NULL;
}

/**
 * Updates the displayed image for the cloned window
 *
 * @param cloneInfo
 */
static void updateClone(WindowInfo* clone, WindowInfo* parent) {
    assert(clone);
    CloneInfo* info = getCloneInfo(clone);
    const Rect dims = clone->geometry;
    setWindowTitle(clone->id, parent->title);
    xcb_copy_area(dis, parent->id, clone->id, graphics_context,
        info->offset[0], info->offset[1], 0, 0, dims.width, dims.height);
}

uint32_t CLONE_REFRESH_RATE = 15;

static void syncPropertiesWithParent(WindowID clone, WindowInfo* parent) {
    setWindowTitle(clone, parent->title);
    setWindowClass(clone, parent->className, parent->instanceName);
    setWindowType(clone, parent->type);
}
WindowInfo* cloneWindow(WindowInfo* winInfo) {
    assert(winInfo);
    DEBUG("Cloning window %d", winInfo->id);
    uint32_t values[1] = {winInfo->overrideRedirect};
    WindowID win = createWindow(winInfo->parent, XCB_WINDOW_CLASS_INPUT_OUTPUT, XCB_CW_OVERRIDE_REDIRECT, values,
            winInfo->geometry);
    xcb_icccm_wm_hints_t hints = {.input = hasMask(winInfo, INPUT_MASK)};
    catchError(xcb_icccm_set_wm_hints_checked(dis, win, &hints));
    WindowInfo* clone = newWindowInfo(win, winInfo->parent);
    syncPropertiesWithParent(clone->id, winInfo);
    addMask(clone, winInfo->mask & EXTERNAL_MASKS);
    if(getWorkspaceOfWindow(winInfo))
        moveToWorkspace(clone, getWorkspaceIndexOfWindow(winInfo));
    if(hasMask(winInfo, MAPPED_MASK))
        mapWindow(clone->id);
    clone->eventMasks |= NON_ROOT_EVENT_MASKS | XCB_EVENT_MASK_EXPOSURE;
    clone->deviceEventMasks |= NON_ROOT_DEVICE_EVENT_MASKS | XCB_INPUT_XI_EVENT_MASK_ENTER;
    CloneInfo* cloneInfo = calloc(sizeof(CloneInfo), 1);
    cloneInfo->originalID = winInfo->id;
    cloneInfo->cloneID = clone->id;
    addElement(&clones, cloneInfo);
    updateClone(clone, winInfo);
    return registerWindowInfo(clone, NULL) ? clone : NULL;
}
void killAllClones(WindowInfo* parent) {
    FOR_EACH(WindowInfo*, winInfo, getAllWindows()) {
        if(isCloneOf(winInfo, parent))
            destroyWindow(winInfo->id);
    }
}
void updateAllClonesOfWindow(WindowInfo* parent) {
    FOR_EACH(WindowInfo*, winInfo, getAllWindows()) {
        if(isCloneOf(winInfo, parent))
            updateClone(winInfo, parent);
    }
}

uint32_t updateAllClones(void) {
    int count = 0;
    FOR_EACH(CloneInfo*, info, &clones) {
        WindowInfo* clone = getWindowInfo(info->cloneID);
        WindowInfo* parent = getWindowInfo(info->originalID);
        updateClone(clone, parent);
        count++;
    }
    return count;
}

void swapWithOriginalOnEnter(xcb_input_enter_event_t* event) {
    WindowInfo* clone = getWindowInfo(event->event);
    if(clone) {
        WindowInfo* parent = getParent(clone);
        if(parent && isNotInInvisibleWorkspace(parent) && isNotInInvisibleWorkspace(clone)) {
            DEBUG("swapping clone %d with %d", clone->id, parent ? parent->id : 0);
            swapWindows(clone, parent);
        }
    }
}
void onExpose(xcb_expose_event_t* event) {
    if(event->count == 0) {
        WindowInfo* winInfo = getWindowInfo(event->window);
        if(winInfo) {
            WindowInfo* parent = getParent(winInfo);
            if(parent && isNotInInvisibleWorkspace(parent))
                updateClone(winInfo, parent);
            updateAllClonesOfWindow(winInfo);
        }
    }
}
void focusParent(xcb_input_focus_in_event_t* event) {
    WindowInfo* clone = getWindowInfo(event->event);
    if(clone) {
        WindowInfo* parent = getParent(clone);
        if(parent && hasMask(parent, MAPPED_MASK))
            focusWindowInfo(parent);
    }
}
void swapOnMapEvent(xcb_map_notify_event_t* event) {
    WindowInfo* clone = getWindowInfo(event->window);
    if(clone) {
        WindowInfo* parent = getParent(clone);
        DEBUG("parent %p", parent);
        if(parent && !isNotInInvisibleWorkspace(parent)) {
            DEBUG("swapping mapped clone with unmapped parent %d with %d", clone->id, parent->id);
            swapWindows(clone, parent);
            mapWindow(parent->id);
            unmapWindow(clone->id);
        }
    }
}
void swapOnUnmapEvent(xcb_unmap_notify_event_t* event) {
    WindowInfo* parent = getWindowInfo(event->window);
    if(parent) {
        FOR_EACH(WindowInfo*, clone, getAllWindows()) {
            if(isCloneOf(clone, parent) && isNotInInvisibleWorkspace(clone)) {
                swapWindows(clone, parent);
                mapWindow(parent->id);
                unmapWindow(clone->id);
            }
        }
    }
}
void onCloneUnregister(WindowInfo* winInfo) {
    CloneInfo* info = removeElement(&clones, winInfo, sizeof(WindowID));
    if(info) {
        free(info);
    }
}

void addCloneRules(void) {
    //getEventRules(XCB_INPUT_MOTION + GENERIC_EVENT_OFFSET).add( &swapWithOriginalRule);
    addEvent(UNREGISTER_WINDOW, DEFAULT_EVENT(killAllClones, HIGH_PRIORITY));
    addEvent(UNREGISTER_WINDOW, DEFAULT_EVENT(onCloneUnregister));
    addEvent(XCB_EXPOSE, DEFAULT_EVENT(onExpose));
    addEvent(XCB_INPUT_ENTER + GENERIC_EVENT_OFFSET, DEFAULT_EVENT(swapWithOriginalOnEnter));
    addEvent(XCB_INPUT_FOCUS_IN + GENERIC_EVENT_OFFSET, DEFAULT_EVENT(focusParent));
    addEvent(XCB_MAP_NOTIFY, DEFAULT_EVENT(swapOnMapEvent));
    addEvent(XCB_UNMAP_NOTIFY, DEFAULT_EVENT(swapOnUnmapEvent));
    addEvent(X_CONNECTION, DEFAULT_EVENT(create_graphics_context));
}



void swapWindows(WindowInfo* winInfo1, WindowInfo* winInfo2) {
    TRACE("swapping windows %d %d", winInfo1->id, winInfo2->id);
    Workspace* w1 = getWorkspaceOfWindow(winInfo1);
    Workspace* w2 = getWorkspaceOfWindow(winInfo2);
    if(w1 && w2) {
        swapElements(
            getWorkspaceWindowStack(w1),
            getIndex(getWorkspaceWindowStack(w1), winInfo1, sizeof(WindowID)),
            getWorkspaceWindowStack(w2),
            getIndex(getWorkspaceWindowStack(w2), winInfo2, sizeof(WindowID))
        );
    }
    Rect geo = getRealGeometry(winInfo2->id);
    setWindowPosition(winInfo2->id, getRealGeometry(winInfo1->id));
    setWindowPosition(winInfo1->id, geo);
}
