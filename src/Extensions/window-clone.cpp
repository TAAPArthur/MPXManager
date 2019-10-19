#include <assert.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xtest.h>

#include "../ext.h"
#include "../bindings.h"
#include "../xsession.h"
#include "../wm-rules.h"
#include "../devices.h"
#include "../globals.h"
#include "../logger.h"
#include "../state.h"
#include "../system.h"
#include "../windows.h"
#include "../wmfunctions.h"
#include "../xsession.h"
#include "../user-events.h"
#include "../globals.h"
#include "../time.h"
#include "../window-properties.h"
#include "window-clone.h"


CloneInfo* getCloneInfo(WindowInfo* winInfo, bool createNew) {
    static Index<CloneInfo> index;
    auto* list = get(index, winInfo, createNew, NULL);
    return list;
}
ArrayList<WindowID>* getClonesOfWindow(WindowInfo* winInfo, bool createNew) {
    static Index<ArrayList<WindowID>> index;
    auto* list = get(index, winInfo, createNew, NULL);
    return list;
}

static void syncPropertiesWithParent(WindowInfo* clone, WindowInfo* parent) {
    assert(clone != parent);
    setWindowTitle(clone->getID(), parent->getTitle());
    setWindowClass(clone->getID(), parent->getClassName(), parent->getInstanceName());
    setWindowType(clone->getID(), parent->getType());
}
WindowInfo* cloneWindow(WindowInfo* winInfo, WindowID parent) {
    assert(winInfo);
    LOG(LOG_LEVEL_DEBUG, "Cloning window %d\n", winInfo->getID());
    uint32_t values[1] = {winInfo->isOverrideRedirectWindow()};
    WindowID window = xcb_generate_id(dis);
    parent = parent ? parent : winInfo->getParent();
    xcb_void_cookie_t cookie = xcb_create_window_checked(dis,
            XCB_COPY_FROM_PARENT, window, parent,
            0, 0, 10, 10, 0,
            winInfo->isInputOnly() ? XCB_WINDOW_CLASS_INPUT_ONLY : XCB_WINDOW_CLASS_INPUT_OUTPUT,
            screen->root_visual,
            XCB_CW_OVERRIDE_REDIRECT, values);
    catchError(cookie);
    xcb_icccm_wm_hints_t hints = {.input = winInfo->hasMask(INPUT_MASK), .initial_state = XCB_ICCCM_WM_STATE_NORMAL };
    catchError(xcb_icccm_set_wm_hints_checked(dis, window, &hints));
    WindowInfo* clone = new WindowInfo(window, parent, winInfo->getID());
    syncPropertiesWithParent(clone, winInfo);
    clone->addMask(winInfo->getMask());
    if(winInfo->isInputOnly())
        clone->markAsInputOnly();
    if(winInfo->isOverrideRedirectWindow())
        clone->markAsOverrideRedirect();
    attemptToMapWindow(clone->getID());
    clone->addEventMasks(NON_ROOT_EVENT_MASKS | XCB_EVENT_MASK_EXPOSURE);
    if(!applyEventRules(PreRegisterWindow, clone)) {
        delete clone;
        return NULL;
    }
    getAllWindows().add(clone);
    postRegisterWindow(clone, 0);
    passiveGrab(clone->getID(), NON_ROOT_DEVICE_EVENT_MASKS | XCB_INPUT_XI_EVENT_MASK_ENTER);
    getClonesOfWindow(winInfo, 1)->add(clone->getID());
    CloneInfo* cloneInfo = getCloneInfo(clone);
    cloneInfo->original = winInfo->getID();
    cloneInfo->clone = clone->getID();
    return clone;
}
void killAllClones(WindowInfo* winInfo) {
    auto* list = getClonesOfWindow(winInfo);
    if(list)
        for(WindowID clone : *list)
            catchError(xcb_destroy_window_checked(dis, clone));
}

/**
 * Updates the displayed image for the cloned window
 *
 * @param cloneInfo
 */
static void updateClone(WindowInfo* clone) {
    assert(clone);
    CloneInfo* info = getCloneInfo(clone);
    const RectWithBorder& dims = clone->getGeometry();
    setWindowTitle(clone->getID(), getWindowInfo(clone->getEffectiveID())->getTitle());
    xcb_copy_area(dis, clone->getEffectiveID(), clone->getID(), graphics_context,
        info->offset[0], info->offset[1], 0, 0, dims.width, dims.height);
}
void updateAllClonesOfWindow(WindowInfo* winInfo) {
    auto* list = getClonesOfWindow(winInfo);
    if(list)
        for(WindowID win : *list)
            if(getWindowInfo(win))
                updateClone(getWindowInfo(win));
}

void updateAllClones(void) {
    for(WindowInfo* winInfo : getAllWindows())
        if(winInfo->getID() != winInfo->getEffectiveID())
            updateClone(winInfo);
}
void* autoUpdateClones(void* arg __attribute__((unused))) {
    assert(CLONE_REFRESH_RATE);
    while(!isShuttingDown()) {
        ATOMIC(updateAllClones());
        msleep(CLONE_REFRESH_RATE);
    }
    return NULL;
}
void startAutoUpdatingClones(void) {
    runInNewThread(autoUpdateClones, NULL, "Auto update window clone");
}

void swapWithOriginalOnEnter(void) {
    xcb_input_enter_event_t* event = (xcb_input_enter_event_t*) getLastEvent();
    WindowInfo* winInfo = getWindowInfo(event->event);
    if(winInfo) {
        WindowInfo* origin = getWindowInfo(winInfo->getEffectiveID());
        if(origin && origin->hasMask(MAPPED_MASK) && winInfo->hasMask(MAPPED_MASK)) {
            LOG(LOG_LEVEL_DEBUG, "swapping clone %d with %d\n", winInfo->getID(), origin ? origin->getID() : 0);
            swapWindows(origin, winInfo);
            markState();
        }
    }
}
void onExpose(void) {
    xcb_expose_event_t* event = (xcb_expose_event_t*) getLastEvent();
    if(event->count == 0) {
        WindowInfo* winInfo = getWindowInfo(event->window);
        if(winInfo) {
            WindowInfo* origin = getWindowInfo(winInfo->getEffectiveID());
            if(origin && origin != winInfo && origin->hasMask(MAPPED_MASK))
                updateClone(winInfo);
            updateAllClonesOfWindow(winInfo);
        }
    }
}
void focusParent(void) {
    xcb_input_focus_in_event_t* event = (xcb_input_focus_in_event_t*) getLastEvent();
    WindowInfo* winInfo = getWindowInfo(event->event);
    if(winInfo) {
        WindowInfo* origin = getWindowInfo(winInfo->getEffectiveID());
        if(origin && origin != winInfo && origin->isInteractable())
            focusWindow(origin);
    }
}
void swapOnMapEvent(void) {
    xcb_map_notify_event_t* event = (xcb_map_notify_event_t*)getLastEvent();
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo) {
        WindowInfo* origin = getWindowInfo(winInfo->getEffectiveID());
        if(origin && origin != winInfo && !origin->hasMask(MAPPED_MASK)) {
            LOG(LOG_LEVEL_DEBUG, "swapping mapped clone with unmapped parent %d with %d\n", winInfo->getID(), origin->getID());
            swapWindows(origin, winInfo);
        }
    }
}
void swapOnUnmapEvent(void) {
    xcb_unmap_notify_event_t* event = (xcb_unmap_notify_event_t*)getLastEvent();
    WindowInfo* winInfo = getWindowInfo(event->window);
    if(winInfo) {
        auto list = getClonesOfWindow(winInfo);
        if(list)
            for(WindowID cloneID : *list) {
                WindowInfo* clone = getWindowInfo(cloneID);
                if(clone && clone->hasMask(MAPPED_MASK))
                    swapWindows(winInfo, clone);
            }
    }
}
void addCloneRules(void) {
    getEventRules(XCB_EXPOSE).add(DEFAULT_EVENT(onExpose));
    //getEventRules(XCB_INPUT_MOTION + GENERIC_EVENT_OFFSET).add( &swapWithOriginalRule);
    getEventRules(XCB_MAP_NOTIFY).add(DEFAULT_EVENT(swapOnMapEvent));
    getEventRules(XCB_UNMAP_NOTIFY).add(DEFAULT_EVENT(swapOnUnmapEvent));
    getEventRules(XCB_INPUT_ENTER + GENERIC_EVENT_OFFSET).add(DEFAULT_EVENT(swapWithOriginalOnEnter));
    getEventRules(XCB_INPUT_FOCUS_IN + GENERIC_EVENT_OFFSET).add(DEFAULT_EVENT(focusParent));
}
