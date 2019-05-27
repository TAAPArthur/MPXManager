#include <stdlib.h>

#include <X11/keysym.h>

#include "../bindings.h"
#include "../devices.h"
#include "../events.h"
#include "../globals.h"
#include "../masters.h"
#include "../mywm-util.h"
#include "../test-functions.h"
#include "xmousecontrol.h"

#if XMOUSE_CONTROL_EXT_ENABLED

unsigned int XMOUSE_CONTROL_UPDATER_INTERVAL = 30;
unsigned int BASE_MOUSE_SPEED = 10;
unsigned int BASE_SCROLL_SPEED = 1;

typedef struct {
    /// ID of the backing master device
    MasterID id;
    /// the id of the pointer of said master device
    MasterID pointerId;
    /// The masks that are currently appied
    int mask;
    /// how many times the scroll button is pressed when scrolling
    int scrollScale;
    /// The displacement every updater when the mouse moves
    double vScale;
} MasterInfo;

static ArrayList masterInfo;
static MasterInfo* getMasterInfo(void){
    Master* m = getActiveMaster();
    MasterInfo* info = find(&masterInfo, m, sizeof(MasterID));
    if(info == NULL){
        info = calloc(1, sizeof(MasterInfo));
        info->id = m->id;
        info->pointerId = m->pointerId;
        info->scrollScale = BASE_SCROLL_SPEED;
        info->vScale = BASE_MOUSE_SPEED ;
        addToList(&masterInfo, info);
    }
    return info;
}
void removeXmouseControlMasterInfo(MasterID id){
    removeElementFromList(&masterInfo, &id, sizeof(MasterID));
}
void addXMouseControlMask(int mask){
    MasterInfo* info = getMasterInfo();
    info->mask |= mask;
}
void removeXMouseControlMask(int mask){
    MasterInfo* info = getMasterInfo();
    info->mask &= ~mask;
}

static void clickButtonN(int btn, int N){
    for(int i = 0; i < N; i++)
        clickButton(btn);
}
#define _IS_SET(info,A,B)\
       (info->mask & (A|B)) && (((info->mask & A)?1:0) ^ ((info->mask&B)?1:0))
void xmousecontrolUpdate(void){
    FOR_EACH(MasterInfo*, info, &masterInfo){
        if(_IS_SET(info, SCROLL_RIGHT_MASK, SCROLL_LEFT_MASK))
            if(info->mask & SCROLL_RIGHT_MASK)
                clickButtonN(SCROLL_RIGHT, info->scrollScale);
            else
                clickButtonN(SCROLL_LEFT, info->scrollScale);
        if(_IS_SET(info, SCROLL_UP_MASK, SCROLL_DOWN_MASK))
            if(info->mask & SCROLL_UP_MASK)
                clickButtonN(SCROLL_UP, info->scrollScale);
            else
                clickButtonN(SCROLL_DOWN, info->scrollScale);
        double deltaX = 0, deltaY = 0;
        if(_IS_SET(info, MOVE_RIGHT_MASK, MOVE_LEFT_MASK))
            deltaX = info->mask & MOVE_RIGHT_MASK ? info->vScale : -info->vScale;
        if(_IS_SET(info, MOVE_UP_MASK, MOVE_DOWN_MASK))
            deltaY = info->mask & MOVE_DOWN_MASK ? info->vScale : -info->vScale;
        if(deltaX || deltaY){
            movePointer(info->pointerId, None, deltaX, deltaY);
        }
    }
}
void* runXMouseControl(void* c __attribute__((unused))){
    while(!isShuttingDown()){
        xmousecontrolUpdate();
        msleep(XMOUSE_CONTROL_UPDATER_INTERVAL);
    }
    return NULL;
}

void adjustScrollSpeed(double multiplier){
    MasterInfo* info = getMasterInfo();
    info->scrollScale *= multiplier;
    if(info->scrollScale < 1)
        info->scrollScale = 1;
}

void adjustSpeed(double multiplier){
    MasterInfo* info = getMasterInfo();
    info->vScale *= multiplier;
    if(info->vScale < 1)
        info->vScale = 1;
}
#define PAIR(MASK,KEY,KP,KR)\
    {MASK,KEY,KP,.noKeyRepeat=1,.mask=XCB_INPUT_XI_EVENT_MASK_KEY_PRESS},\
    {MASK,KEY,KR,.mask=XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE,.noKeyRepeat=1}

static Binding bindings[] = {
// modifier  key	opts	press func	 press arg	release func	release arg
// Enable/disable
//
// Directional control with WASD.
    PAIR(0,	XK_w, BIND(addXMouseControlMask, SCROLL_UP_MASK), BIND(removeXMouseControlMask, SCROLL_UP_MASK)),
    PAIR(0,	XK_a, BIND(addXMouseControlMask, SCROLL_LEFT_MASK), BIND(removeXMouseControlMask, SCROLL_LEFT_MASK)),
    PAIR(0,	XK_s, BIND(addXMouseControlMask, SCROLL_DOWN_MASK), BIND(removeXMouseControlMask, SCROLL_DOWN_MASK)),
    PAIR(0,	XK_d, BIND(addXMouseControlMask, SCROLL_RIGHT_MASK), BIND(removeXMouseControlMask, SCROLL_RIGHT_MASK)),

    PAIR(0,	XK_Up, BIND(addXMouseControlMask, MOVE_UP_MASK), BIND(removeXMouseControlMask, MOVE_UP_MASK)),
    PAIR(0,	XK_Left, BIND(addXMouseControlMask, MOVE_LEFT_MASK), BIND(removeXMouseControlMask, MOVE_LEFT_MASK)),
    PAIR(0,	XK_Down, BIND(addXMouseControlMask, MOVE_DOWN_MASK), BIND(removeXMouseControlMask, MOVE_DOWN_MASK)),
    PAIR(0,	XK_Right, BIND(addXMouseControlMask, MOVE_RIGHT_MASK), BIND(removeXMouseControlMask, MOVE_RIGHT_MASK)),

    PAIR(0,	XK_KP_Up, BIND(addXMouseControlMask, MOVE_UP_MASK), BIND(removeXMouseControlMask, MOVE_UP_MASK)),
    PAIR(0,	XK_KP_Left, BIND(addXMouseControlMask, MOVE_LEFT_MASK), BIND(removeXMouseControlMask, MOVE_LEFT_MASK)),
    PAIR(0,	XK_KP_Down, BIND(addXMouseControlMask, MOVE_DOWN_MASK), BIND(removeXMouseControlMask, MOVE_DOWN_MASK)),
    PAIR(0,	XK_KP_Right, BIND(addXMouseControlMask, MOVE_RIGHT_MASK), BIND(removeXMouseControlMask, MOVE_RIGHT_MASK)),

    {0,	XK_e, BIND(adjustScrollSpeed, 2)},
    {0 | ShiftMask,	XK_e, BIND(adjustScrollSpeed, .5)},
    {0 | Mod1Mask,	XK_e, BIND(adjustSpeed, 2)},
    {0 | Mod1Mask | ShiftMask,	XK_e, BIND(adjustSpeed, .5)},
    {0,	XK_q, PIPE(BIND(getActiveMasterKeyboardID), BIND(removeXmouseControlMasterInfo))},

    {0, XK_c, BIND(clickButton, Button2)},
    {0, XK_x, BIND(clickButton, Button3)},
    {0, XK_space, BIND(clickButton, Button1)},
    {0, XK_Insert, BIND(clickButton, Button2)},
    {0, XK_space, BIND(clickButton, Button1)},
    {0 | ShiftMask,	XK_space, BIND(clickButton, Button1)},
    {0,	XK_Insert, BIND(clickButton, Button2)},

    {0,	XK_Return, BIND(grabActiveKeyboard)},
    {0 | ShiftMask,	XK_Return, PIPE(BIND(getActiveMasterKeyboardID), BIND(ungrabDevice))},
};
static void garbageCollectMasterInfo(void){
    xcb_input_hierarchy_event_t* event = getLastEvent();
    if(event->flags & (XCB_INPUT_HIERARCHY_MASK_MASTER_REMOVED)){
        FOR_EACH_REVERSED(MasterInfo*, info, &masterInfo){
            if(find(getAllMasters(), info, sizeof(MasterID)))
                removeXmouseControlMasterInfo(info->id);
        }
    }
}
int XMOUSE_CONTROL_DEFAULT_MASK = Mod3Mask;
void enableXMouseControl(void){
    for(int i = 0; i < LEN(bindings); i++)
        bindings[i].mod |= XMOUSE_CONTROL_DEFAULT_MASK;
    addBindings(bindings, LEN(bindings));
    static Rule gcRule = CREATE_WILDCARD(BIND(garbageCollectMasterInfo));
    addToList(getEventRules(GENERIC_EVENT_OFFSET + XCB_INPUT_HIERARCHY), &gcRule);
}
#endif
