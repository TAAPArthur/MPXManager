#include <stdlib.h>

#include <X11/keysym.h>

#include "../bindings.h"
#include "../device-grab.h"
#include "../devices.h"
#include "../ext.h"
#include "../communications.h"
#include "../globals.h"
#include "../logger.h"
#include "../masters.h"
#include "../system.h"
#include "../test-functions.h"
#include "../threads.h"
#include "../time.h"
#include "../xsession.h"
#include "xmousecontrol.h"

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
static ThreadSignaler signaler;
void addXMouseControlMask(int mask) {
    XMouseControlMasterState* info = getXMouseControlMasterState();
    bool change = (info->mask ^ mask);
    info->mask |= mask;
    if(change)
        signaler.signal();
}
void removeXMouseControlMask(int mask) {
    XMouseControlMasterState* info = getXMouseControlMasterState();
    info->mask &= ~mask;
}

static inline void _notify(XMouseControlMasterState* info, bool scroll) {
    std::string summary = "XMouseControl " + std::to_string(info->id);
    std::string body = scroll ?
        "Scroll speed " + std::to_string(info->scrollScale) :
        "Pointer Speed " + std::to_string(info->vScale);
    notify(summary, body);
}

void adjustScrollSpeed(int diff) {
    XMouseControlMasterState* info = getXMouseControlMasterState();
    info->scrollScale = diff == 0 ? 0 : info->scrollScale + diff;
    info->scrollScale = std::max(1, std::min(info->scrollScale, 256));
    INFO("scrollScale is now " << info->scrollScale << "Master " << info->id);
    _notify(info, 1);
}

void adjustSpeed(int diff) {
    XMouseControlMasterState* info = getXMouseControlMasterState();
    info->vScale = diff == 0 ? 0 : info->vScale + diff;
    info->vScale = std::max(1, std::min(info->vScale, 256));
    INFO("vScale is now " << info->vScale << " Master " << info->id);
    _notify(info, 0);
}

#define _IS_SET(info,A,B)\
       ((info->mask & (A|B)) && (((info->mask & A)?1:0) ^ ((info->mask&B)?1:0)))
bool xmousecontrolUpdate(void) {
    assert(dis);
    bool update = 0;
    for(Master* master : getAllMasters()) {
        XMouseControlMasterState* info = getXMouseControlMasterState(master, 0);
        if(info && info->mask) {
            update = 1;
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
                movePointerRelative(deltaX, deltaY, info->pointerId);
        }
    }
    if(update)
        flush();
    return update;
}
void runXMouseControl() {
    INFO("Starting thread XMouseControl");
    while(!isShuttingDown()) {
        lock();
        auto update = xmousecontrolUpdate();
        unlock();
        signaler.sleepOrWait(update, XMOUSE_CONTROL_UPDATER_INTERVAL);
    }
    INFO("Ending thread XMouseControl");
}
void addStartXMouseControlRule() {
    getEventRules(X_CONNECTION).add({[]() {spawnThread(runXMouseControl);}, FUNC_NAME});
}

#define PAIR(MASK,KEY,KP,A1,KR,A2)\
    {MASK, KEY, {KP, A1, DEFAULT_EVENT_NAME(KP)}, {.mask = XCB_INPUT_XI_EVENT_MASK_KEY_PRESS}}, \
    {MASK, KEY, {KR, A2, DEFAULT_EVENT_NAME(KR)}, {.mask = XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE}}

#define BINDING(MASK, KEY, BUTTON_MASK)\
    PAIR(MASK, KEY, addXMouseControlMask, BUTTON_MASK, removeXMouseControlMask, BUTTON_MASK)

#define CLICK(BTN) {[](){clickButton(BTN);}, "click_" # BTN}
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
        {mask | Mod1Mask,	XK_e, {adjustSpeed, 1}},
        {mask | Mod1Mask | ShiftMask,	XK_e, {adjustSpeed, -1}},

        {mask,	XK_semicolon, {adjustScrollSpeed, 2}},
        {mask | ShiftMask,	XK_semicolon, {adjustScrollSpeed, -2}},
        {mask | Mod1Mask,	XK_semicolon, {adjustSpeed, 2}},
        {mask | Mod1Mask | ShiftMask,	XK_semicolon, {adjustSpeed, -2}},
        {mask,	XK_q, resetXMouseControl},

        {mask, XK_c, CLICK(Button2)},
        {mask, XK_x, CLICK(Button3)},
        {mask, XK_space, CLICK(Button1)},
        {mask, XK_Return, CLICK(Button2)},
        {mask | ShiftMask,	XK_space, CLICK(Button3)},

        {mask,	XK_Tab, +[]{grabKeyboard();}},
        {mask | ShiftMask,	XK_Tab, {[]() {ungrabDevice(getActiveMasterKeyboardID());}}},
    };
    for(Binding& b : bindings)
        getDeviceBindings().add(b);
}

__attribute__((constructor)) static void registerModes() {
    addStartupMode("xmousecontrol", addStartXMouseControlRule);
}
