#include <stdlib.h>

#include <X11/keysym.h>

#include "../bindings.h"
#include "../devices.h"
#include "../device-grab.h"
#include "../ext.h"
#include "../globals.h"
#include "../masters.h"
#include "../test-functions.h"
#include "../system.h"
#include "../time.h"
#include "../xsession.h"
#include "xmousecontrol.h"

#if XMOUSE_CONTROL_EXT_ENABLED

unsigned int XMOUSE_CONTROL_UPDATER_INTERVAL = 30;
unsigned int BASE_MOUSE_SPEED = 10;
unsigned int BASE_SCROLL_SPEED = 1;

struct XMouseControlMasterState {
    /// ID of the backing master device
    MasterID id;
    /// the id of the pointer of said master device
    MasterID pointerId;
    /// The masks that are currently applied
    int mask;
    /// how many times the scroll button is pressed when scrolling
    int scrollScale;
    /// The displacement every updater when the mouse moves
    double vScale;
} ;

static XMouseControlMasterState* getXMouseControlMasterState(Master* m = getActiveMaster(), bool createNew = 1) {
    static Index<XMouseControlMasterState> index;
    bool newElement = 0;
    XMouseControlMasterState* info = get(index, m, createNew, &newElement);
    if(newElement) {
        info->id = m->getID();
        info->pointerId = m->getPointerID();
        resetXMouseControl();
    }
    return info;
}
void resetXMouseControl() {
    XMouseControlMasterState* info = getXMouseControlMasterState();
    info->scrollScale = BASE_SCROLL_SPEED;
    info->vScale = BASE_MOUSE_SPEED ;
    info->mask = 0;
}
void addXMouseControlMask(int mask) {
    XMouseControlMasterState* info = getXMouseControlMasterState();
    info->mask |= mask;
}
void removeXMouseControlMask(int mask) {
    XMouseControlMasterState* info = getXMouseControlMasterState();
    info->mask &= ~mask;
}
void adjustScrollSpeed(int multiplier) {
    XMouseControlMasterState* info = getXMouseControlMasterState();
    info->scrollScale *= multiplier >= 0 ? multiplier : -1.0 / multiplier;
    if(info->scrollScale < 1e-6)
        info->scrollScale = 1;
}

void adjustSpeed(int multiplier) {
    XMouseControlMasterState* info = getXMouseControlMasterState();
    info->vScale *= multiplier >= 0 ? multiplier : -1.0 / multiplier;
    if(info->vScale < 1e-6)
        info->vScale = 1;
}

#define _IS_SET(info,A,B)\
       ((info->mask & (A|B)) && (((info->mask & A)?1:0) ^ ((info->mask&B)?1:0)))
void xmousecontrolUpdate(void) {
    assert(dis);
    for(Master* master : getAllMasters()) {
        XMouseControlMasterState* info = getXMouseControlMasterState(master, 0);
        if(info) {
            if(_IS_SET(info, SCROLL_RIGHT_MASK, SCROLL_LEFT_MASK))
                for(int i = 0; i < info->scrollScale; i++)
                    clickButton(info->mask & SCROLL_RIGHT_MASK ? SCROLL_RIGHT : SCROLL_LEFT, info->pointerId);
            if(_IS_SET(info, SCROLL_UP_MASK, SCROLL_DOWN_MASK))
                for(int i = 0; i < info->scrollScale; i++)
                    clickButton(info->mask & SCROLL_UP_MASK ? SCROLL_UP : SCROLL_DOWN, info->pointerId);
            double deltaX = 0, deltaY = 0;
            if(_IS_SET(info, MOVE_RIGHT_MASK, MOVE_LEFT_MASK))
                deltaX = info->mask & MOVE_RIGHT_MASK ? info->vScale : -info->vScale;
            if(_IS_SET(info, MOVE_UP_MASK, MOVE_DOWN_MASK))
                deltaY = info->mask & MOVE_DOWN_MASK ? info->vScale : -info->vScale;
            if(deltaX || deltaY)
                movePointer(deltaX, deltaY, info->pointerId, None);
        }
    }
    flush();
}
void* runXMouseControl(void* c __attribute__((unused))) {
    while(!isShuttingDown()) {
        ATOMIC(xmousecontrolUpdate());
        msleep(XMOUSE_CONTROL_UPDATER_INTERVAL);
    }
    return NULL;
}
void addStartXMouseControlRule() {
    getEventRules(onXConnection).add({[]() {runInNewThread(runXMouseControl, NULL, "xmousecontrol");}, FUNC_NAME});
}

#define PAIR(MASK,KEY,KP,A1,KR,A2)\
    {MASK,KEY,{KP,A1},{.mask=XCB_INPUT_XI_EVENT_MASK_KEY_PRESS}},\
    {MASK,KEY,{KR,A2},{.mask=XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE}}


static Binding bindings[] = {
    // Directional control with WASD.
    PAIR(0,	XK_w, addXMouseControlMask, SCROLL_UP_MASK, removeXMouseControlMask, SCROLL_UP_MASK),
    PAIR(0,	XK_a, addXMouseControlMask, SCROLL_LEFT_MASK, removeXMouseControlMask, SCROLL_LEFT_MASK),
    PAIR(0,	XK_s, addXMouseControlMask, SCROLL_DOWN_MASK, removeXMouseControlMask, SCROLL_DOWN_MASK),
    PAIR(0,	XK_d, addXMouseControlMask, SCROLL_RIGHT_MASK, removeXMouseControlMask, SCROLL_RIGHT_MASK),

    PAIR(0,	XK_Up, addXMouseControlMask, MOVE_UP_MASK, removeXMouseControlMask, MOVE_UP_MASK),
    PAIR(0,	XK_Left, addXMouseControlMask, MOVE_LEFT_MASK, removeXMouseControlMask, MOVE_LEFT_MASK),
    PAIR(0,	XK_Down, addXMouseControlMask, MOVE_DOWN_MASK, removeXMouseControlMask, MOVE_DOWN_MASK),
    PAIR(0,	XK_Right, addXMouseControlMask, MOVE_RIGHT_MASK, removeXMouseControlMask, MOVE_RIGHT_MASK),

    PAIR(0,	XK_KP_Up, addXMouseControlMask, MOVE_UP_MASK, removeXMouseControlMask, MOVE_UP_MASK),
    PAIR(0,	XK_KP_Left, addXMouseControlMask, MOVE_LEFT_MASK, removeXMouseControlMask, MOVE_LEFT_MASK),
    PAIR(0,	XK_KP_Down, addXMouseControlMask, MOVE_DOWN_MASK, removeXMouseControlMask, MOVE_DOWN_MASK),
    PAIR(0,	XK_KP_Right, addXMouseControlMask, MOVE_RIGHT_MASK, removeXMouseControlMask, MOVE_RIGHT_MASK),

    {0, XK_Hyper_L, {removeXMouseControlMask, -1}, {.mask = XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE}},

    {0,	XK_e, {adjustScrollSpeed, 2}},
    {0 | ShiftMask,	XK_e, {adjustScrollSpeed, -2}},
    {0 | Mod1Mask,	XK_e, {adjustSpeed, 0}},
    {0,	XK_r, {adjustScrollSpeed, 2}},
    {0 | ShiftMask,	XK_r, {adjustScrollSpeed, -2}},
    {0 | Mod1Mask,	XK_r, {adjustSpeed, 0}},
    {0,	XK_q, resetXMouseControl},

    {0, XK_c, {clickButton, Button2}},
    {0, XK_x, {clickButton, Button3}},
    {0, XK_space, {clickButton, Button1}},
    {0, XK_Insert, {clickButton, Button2}},
    {0, XK_space, {clickButton, Button1}},
    {0 | ShiftMask,	XK_space, {clickButton, Button1}},
    {0,	XK_Insert, {clickButton, Button2}},

    {0,	XK_Return, grabKeyboard},
    {0 | ShiftMask,	XK_Return, {[]() {ungrabDevice(getActiveMasterKeyboardID());}}},
};
int XMOUSE_CONTROL_DEFAULT_MASK = Mod3Mask;
void addDefaultXMouseControlBindings(void) {
    for(int i = 0; i < LEN(bindings); i++) {
        Binding* b = new Binding(bindings[i]);
        b->mod |= XMOUSE_CONTROL_DEFAULT_MASK;
        getDeviceBindings().add(b);
    }
}
#endif
