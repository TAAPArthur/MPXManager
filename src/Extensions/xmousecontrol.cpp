#include <stdlib.h>

#include <X11/keysym.h>

#include "../bindings.h"
#include "../device-grab.h"
#include "../devices.h"
#include "../ext.h"
#include "../globals.h"
#include "../logger.h"
#include "../masters.h"
#include "../system.h"
#include "../test-functions.h"
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
    int vScale;
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
void adjustScrollSpeed(int diff) {
    XMouseControlMasterState* info = getXMouseControlMasterState();
    info->scrollScale = diff == 0 ? 0 : info->scrollScale + diff;
    info->scrollScale = std::max(1, std::min(info->scrollScale, 8));
    logger.info() << "scrollScale is now " << info->scrollScale << "Master " << info->id << std::endl;
}

void adjustSpeed(int multiplier) {
    XMouseControlMasterState* info = getXMouseControlMasterState();
    info->vScale *= multiplier >= 0 ? multiplier : -1.0 / multiplier;
    info->vScale = std::max(1, info->vScale);
    logger.info() << "vScale is now " << info->scrollScale << "Master " << info->id << std::endl;
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
    {MASK, KEY, {KP, A1, DEFAULT_EVENT_NAME(KP)}, {.mask = XCB_INPUT_XI_EVENT_MASK_KEY_PRESS}}, \
    {MASK, KEY, {KR, A2, DEFAULT_EVENT_NAME(KR)}, {.mask = XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE}}

#define BINDING(MASK, KEY, BUTTON_MASK)\
    PAIR(MASK, KEY, addXMouseControlMask, BUTTON_MASK, removeXMouseControlMask, BUTTON_MASK)

void addDefaultXMouseControlBindings(uint32_t mask) {
    Binding bindings[] = {
        // Directional control with WASD.
        BINDING(mask, XK_w, SCROLL_UP_MASK),
        BINDING(mask, XK_a, SCROLL_LEFT_MASK),
        BINDING(mask, XK_s, SCROLL_DOWN_MASK),
        BINDING(mask, XK_d, SCROLL_RIGHT_MASK),
        BINDING(mask, XK_Up, MOVE_UP_MASK),
        BINDING(mask, XK_Left, MOVE_LEFT_MASK),
        BINDING(mask, XK_Down, MOVE_DOWN_MASK),
        BINDING(mask, XK_Right, MOVE_RIGHT_MASK),

        BINDING(mask, XK_k, SCROLL_UP_MASK),
        BINDING(mask, XK_l, SCROLL_RIGHT_MASK),
        BINDING(mask, XK_j, SCROLL_DOWN_MASK),
        BINDING(mask, XK_h, SCROLL_LEFT_MASK),
        BINDING(mask | ShiftMask, XK_k, MOVE_UP_MASK),
        BINDING(mask | ShiftMask, XK_l, MOVE_RIGHT_MASK),
        BINDING(mask | ShiftMask, XK_j, MOVE_DOWN_MASK),
        BINDING(mask | ShiftMask, XK_h, MOVE_LEFT_MASK),

        {mask, XK_Hyper_L, {removeXMouseControlMask, -1}, {.mask = XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE}},

        {mask,	XK_e, {adjustScrollSpeed, 1}},
        {mask | ShiftMask,	XK_e, {adjustScrollSpeed, -1}},
        {mask | Mod1Mask,	XK_e, {adjustSpeed, 0}},
        {mask,	XK_semicolon, {adjustScrollSpeed, 2}},
        {mask | ShiftMask,	XK_semicolon, {adjustScrollSpeed, -2}},
        {mask | Mod1Mask,	XK_semicolon, {adjustSpeed, 0}},
        {mask,	XK_q, resetXMouseControl},

        {mask, XK_c, {clickButton, Button2}},
        {mask, XK_x, {clickButton, Button3}},
        {mask, XK_space, {clickButton, Button1}},
        {mask, XK_Return, {clickButton, Button2}},
        {mask | ShiftMask,	XK_space, {clickButton, Button3}},

        {mask,	XK_Tab, grabKeyboard},
        {mask | ShiftMask,	XK_Tab, {[]() {ungrabDevice(getActiveMasterKeyboardID());}}},
    };
    for(Binding& b : bindings)
        getDeviceBindings().add(b);
}
#endif
