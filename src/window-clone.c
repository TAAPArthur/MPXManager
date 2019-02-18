#include <assert.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xtest.h>

#include "globals.h"
#include "devices.h"
#include "bindings.h"
#include "window-clone.h"
#include "default-rules.h"
#include "test-functions.h"
#include "logger.h"

xcb_gcontext_t graphics_context;

WindowInfo* createCloneWindow(WindowInfo*winInfo){
    assert(winInfo);
    LOG(LOG_LEVEL_ERROR,"Cloning window %d\n",winInfo->id);
    uint32_t mask=XCB_CW_EVENT_MASK;
    uint32_t values[1]={XCB_EVENT_MASK_EXPOSURE};
    int window = xcb_generate_id(dis);
    xcb_void_cookie_t cookie = xcb_create_window_checked(dis,
                 XCB_COPY_FROM_PARENT, window, screen->root,
                 0, 0, 10, 10,0,
                 XCB_WINDOW_CLASS_INPUT_OUTPUT,
                 screen->root_visual,
    mask, values);

    catchError(cookie);
    catchError(xcb_map_window_checked(dis, window));

    WindowInfo *clone =calloc(1,sizeof(WindowInfo));
    memcpy(clone, winInfo, sizeof(WindowInfo));
    clearList(clone->cloneList=createEmptyHead());
    clone->id=window;
    clone->cloneOrigin=winInfo->id;
    processNewWindow(clone);
    insertHead(winInfo->cloneList,clone);


    int masks=0;//XCB_INPUT_XI_EVENT_MASK_ENTER|XCB_INPUT_XI_EVENT_MASK_LEAVE|;
    passiveGrab(winInfo->id, masks|XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS|XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE|
            XCB_INPUT_XI_EVENT_MASK_KEY_PRESS|XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE);
    passiveGrab(clone->id, masks|XCB_INPUT_XI_EVENT_MASK_ENTER|XCB_INPUT_XI_EVENT_MASK_MOTION);
    //passiveGrab(root, masks);

    XSelectInput(dpy, winInfo->id, XCB_EVENT_MASK_EXPOSURE);
    //updateClone(winInfo, clone);

    dumpWindowInfo(winInfo);
    dumpWindowInfo(clone);
    assert(isTileable(clone));

    return clone;

    //passiveGrab(clone->id, XCB_EVENT_MASK_KEY_PRESS|XCB_EVENT_MASK_BUTTON_PRESS);

}
//TODO move to wmfuncctions
xcb_gcontext_t create_graphics_context() {
    uint32_t mask;
    uint32_t values[3];

    mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_GRAPHICS_EXPOSURES;
    values[0] = screen->black_pixel;
    values[1] = screen->white_pixel;
    values[2] = 0;

    graphics_context = xcb_generate_id(dis);
    xcb_void_cookie_t cookie=xcb_create_gc_checked(dis,
        graphics_context, root,
        mask, values);
    catchError(cookie);
    return graphics_context;
}
void updateClone(WindowInfo*winInfo,WindowInfo* dest){
    assert(winInfo);
    assert(dest);
    short* dims=getGeometry(dest);
    assert(isNotEmpty(getAllWindows()));
    LOG(LOG_LEVEL_ERROR,"copying %d to %d\n",winInfo->id, dest->id);
    xcb_void_cookie_t cookie=xcb_copy_area_checked(dis, winInfo->id, dest->id, graphics_context,
            dest->cloneOffset[0], dest->cloneOffset[1], 0, 0, dims[2],dims[3]);
    catchError(cookie);
}

void updateAllClones(WindowInfo*winInfo){
      if(winInfo){
          LOG(LOG_LEVEL_ERROR,"updating %d clones \n",getSize(winInfo->cloneList));
          Node* list=winInfo->cloneList;
          FOR_EACH(list,updateClone(winInfo, getValue(list)));
      }
}


void swapWithOriginal(void){
    xcb_input_enter_event_t* event=getLastEvent();

    WindowInfo*winInfo=getWindowInfo(event->event);

    if(winInfo->cloneOrigin){
        swap(isInList(getActiveWindowStack(), winInfo->id), isInList(getActiveWindowStack(), winInfo->cloneOrigin));
        //TODO fix tileWindows();
    }
}
int getCloneOrigin(WindowInfo*winInfo){
    return winInfo->cloneOrigin;
}
Rule onExposeRule=CREATE_DEFAULT_EVENT_RULE(onExpose);
Rule swapWithOriginalRule=CREATE_DEFAULT_EVENT_RULE(swapWithOriginal);
void addCloneRules(void){
    insertHead(getEventRules(XCB_EXPOSE), &onExposeRule);
    insertHead(getEventRules(XCB_INPUT_MOTION + GENERIC_EVENT_OFFSET), &swapWithOriginalRule);
    insertHead(getEventRules(XCB_INPUT_ENTER + GENERIC_EVENT_OFFSET), &swapWithOriginalRule);
}
void onExpose(void){
    xcb_expose_event_t *event=getLastEvent();
    LOG(LOG_LEVEL_ERROR,"expose event received %d\n",event->window);
    WindowInfo*winInfo=getWindowInfo(event->window);
    if(winInfo){
        LOG(LOG_LEVEL_ERROR,"updating %d clones \n",getSize(winInfo->cloneList));
        Node* list=winInfo->cloneList;
        if(winInfo->cloneOrigin)
            updateClone(getWindowInfo(winInfo->cloneOrigin), winInfo);
        FOR_EACH(list,updateClone(winInfo, getValue(list)));

    }
}
void *autoUpdateClone(void*pointerToOriginalWindowInfo){
    WindowInfo*original=pointerToOriginalWindowInfo;
    if(CLONE_REFRESH_RATE)
        while(1){
            msleep(CLONE_REFRESH_RATE);
            Node* list=original->cloneList;;
            FOR_EACH(list,updateClone(original, getValue(list)));
        }
    return NULL;
}
