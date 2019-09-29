/**
 * @file settings.c
 * @copybrief settings.h
 */

#include <stdlib.h>

#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#include <X11/extensions/XInput2.h>
#include <xcb/xinput.h>

#include "communications.h"
#include "functions.h"
#include "globals.h"
#include "layouts.h"
#include "session.h"
#include "logger.h"
#include "window-properties.h"
#include "masters.h"
#include "session.h"
#include "settings.h"
#include "workspaces.h"
#include "system.h"
#include "windows.h"
#include "chain.h"
#include "wm-rules.h"
#include "extra-rules.h"
#include "ewmh.h"
#include "wmfunctions.h"
#include "workspaces.h"
#include "monitors.h"

#define _startCycleWindowChainBinding(M,K) Chain(M,K,{ \
            Binding{Mod1Mask, XK_Tab, {[](){cycleWindows(DOWN);}}, {.passThrough = NO_PASSTHROUGH,.noGrab = 1}}, \
            Binding{Mod1Mask | ShiftMask, XK_Tab, {[](){cycleWindows(UP);}}, {.passThrough = NO_PASSTHROUGH,.noGrab = 1}}, \
            Binding{WILDCARD_MODIFIER, XK_Alt_L, {[](){endCycleWindows();}}, {.noGrab = 1,.mask = XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE}  },\
            Binding {WILDCARD_MODIFIER, 0, {},{.passThrough = NO_PASSTHROUGH,.noGrab = 1}},\
            },{},\
        XCB_INPUT_XI_EVENT_MASK_KEY_PRESS | XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE)

static Chain DEFAULT_CHAIN_BINDINGS[] = {
    _startCycleWindowChainBinding(Mod1Mask, XK_Tab),
    _startCycleWindowChainBinding(Mod1Mask | ShiftMask, XK_Tab),
    Chain{
        Mod4Mask, Button1, +[](WindowInfo * winInfo){startWindowMoveResize(winInfo, 1);},
        {
            {DEFAULT_MOD_MASK, 0, updateWindowMoveResize, {.passThrough = NO_PASSTHROUGH, .noGrab = 1, .mask = XCB_INPUT_XI_EVENT_MASK_MOTION | XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS}},
            {WILDCARD_MODIFIER, XK_Escape, cancelWindowMoveResize, {.noGrab = 1}},
            {WILDCARD_MODIFIER, 0, commitWindowMoveResize, {.noGrab = 1}},
        },
        {.mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS},
        XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE | XCB_INPUT_XI_EVENT_MASK_MOTION,
    },
    /*
    {
        Mod4Mask, Button3, BOTH(BIND(startMouseOperation), CHAIN_GRAB(XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE | XCB_INPUT_XI_EVENT_MASK_MOTION,
        {Mod4Mask, Button3, resizeWindowWithMouse, .mask = XCB_INPUT_XI_EVENT_MASK_MOTION | XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS, .noGrab = 1, .passThrough = NO_PASSTHROUGH},
        {Mod4Mask, 0, resizeWindowWithMouse, .mask = XCB_INPUT_XI_EVENT_MASK_MOTION, .noGrab = 1, .passThrough = NO_PASSTHROUGH},
        {WILDCARD_MODIFIER, 0, stopMouseOperation, .mask = -1, .noGrab = 1})),
        .mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS
    },
    */
};
/**
 * Creates a set of bindings related to switching workspaces
 * @param K the key to bind; various modifiers will be used for the different functions
 * @param N the workspace to switch to/act on
 */
#define WORKSPACE_OPERATION(K,N) \
    {DEFAULT_MOD_MASK,             K, +[](){switchToWorkspace(N); activateWorkspace(N);}}, \
    {DEFAULT_MOD_MASK|ShiftMask,   K, +[](WindowInfo*winInfo){winInfo->moveToWorkspace(N);}}, \
    {DEFAULT_MOD_MASK|ControlMask,   K, +[](){swapMonitors(getActiveWorkspaceIndex(),N);activateWorkspace(N);}}

/**
 * Creates a set of bindings related to the windowStack
 * @param KEY_UP
 * @param KEY_DOWN
 * @param KEY_LEFT
 * @param KEY_RIGHT
 */
#define STACK_OPERATION(KEY_UP,KEY_DOWN,KEY_LEFT,KEY_RIGHT) \
    {DEFAULT_MOD_MASK,             KEY_UP, +[](){swapPosition(UP);}}, \
    {DEFAULT_MOD_MASK,             KEY_DOWN, +[](){swapPosition(DOWN);}}, \
    {DEFAULT_MOD_MASK,             KEY_LEFT, +[](){shiftFocus(UP);}}, \
    {DEFAULT_MOD_MASK,             KEY_RIGHT, +[](){shiftFocus(DOWN);}}
void addDefaultBindings() {
    Binding DEFAULT_BINDINGS[] = {
        WORKSPACE_OPERATION(XK_1, 0),
        WORKSPACE_OPERATION(XK_2, 1),
        WORKSPACE_OPERATION(XK_3, 2),
        WORKSPACE_OPERATION(XK_4, 3),
        WORKSPACE_OPERATION(XK_5, 4),
        WORKSPACE_OPERATION(XK_6, 5),
        WORKSPACE_OPERATION(XK_7, 6),
        WORKSPACE_OPERATION(XK_8, 7),
        WORKSPACE_OPERATION(XK_9, 8),
        WORKSPACE_OPERATION(XK_0, 9),
        STACK_OPERATION(XK_Up, XK_Down, XK_Left, XK_Right),
        STACK_OPERATION(XK_H, XK_J, XK_K, XK_L),

        {WILDCARD_MODIFIER, Button1, activateWorkspaceUnderMouse, {.passThrough = ALWAYS_PASSTHROUGH, .noGrab = 1,  .mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS}},
        {DEFAULT_MOD_MASK, XK_c, killClientOfWindow, {.noKeyRepeat = 1}},
        {DEFAULT_MOD_MASK | ShiftMask, XK_c, killClientOfWindow, { .windowTarget = TARGET_WINDOW, .noKeyRepeat = 1}},
        {DEFAULT_MOD_MASK, Button1, floatWindow, { .passThrough = ALWAYS_PASSTHROUGH, .mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS}},
        {DEFAULT_MOD_MASK | ShiftMask, Button1, sinkWindow, {.passThrough = ALWAYS_PASSTHROUGH, .mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS}},
        {DEFAULT_MOD_MASK, XK_t, sinkWindow},
        {DEFAULT_MOD_MASK | ControlMask, XK_t, +[](WindowInfo * winInfo) {winInfo->toggleMask(ALWAYS_ON_TOP);}},
        {DEFAULT_MOD_MASK | Mod1Mask, XK_t, +[](WindowInfo * winInfo) {winInfo->toggleMask(STICKY_MASK);}},

        {0, XF86XK_AudioPlay, +[]() {toggleLayout(getDefaultLayouts()[FULL]);}},
        {DEFAULT_MOD_MASK, XK_F11, +[]() {toggleLayout(getDefaultLayouts()[FULL]);}},
        {DEFAULT_MOD_MASK, XF86XK_AudioPlay, +[](WindowInfo * winInfo) {winInfo->toggleMask(FULLSCREEN_MASK);}},
        {DEFAULT_MOD_MASK | ShiftMask, XF86XK_AudioPlay, +[](WindowInfo * winInfo) {winInfo->toggleMask(ROOT_FULLSCREEN_MASK);}},

        {DEFAULT_MOD_MASK, XK_space, +[]() {cycleLayouts(DOWN);}},

        //{DEFAULT_MOD_MASK,Button2,BIND(resizeWindowWithMouse), .noGrab=1, .passThrough=1 ,.mask=XCB_INPUT_XI_EVENT_MASK_MOTION},


        {DEFAULT_MOD_MASK, XK_Return, +[]() {shiftTop();}},
        {DEFAULT_MOD_MASK | ShiftMask, XK_Return, +[]() {focusTop();}},

        {DEFAULT_MOD_MASK | ShiftMask, XK_q, requestShutdown},
        {DEFAULT_MOD_MASK, XK_q, saveCustomState, {.passThrough = ALWAYS_PASSTHROUGH}},
        {DEFAULT_MOD_MASK, XK_q, +[]() {if(waitForChild(spawn("mpxmanager --recompile -g")) == 0)restart();}},

        /* TODO fix modes
        {DEFAULT_MOD_MASK, XK_F1, [](){getActiveMaster()->setCurrentMode(0);}, .mode = ANY_MODE},
        {DEFAULT_MOD_MASK, XK_F2, [](){getActiveMaster()->setCurrentMode(1);}, .mode = ANY_MODE},
        {DEFAULT_MOD_MASK, XK_F3, [](){getActiveMaster()->setCurrentMode(2);}, .mode = ANY_MODE},
        {DEFAULT_MOD_MASK, XK_F4, [](){getActiveMaster()->setCurrentMode(3);}, .mode = ANY_MODE},

        {DEFAULT_MOD_MASK, XK_r, [](){if(getActiveLayout())getActiveLayout()->restoreArgs();}, .mode = LAYOUT_MODE},
        {DEFAULT_MOD_MASK, XK_p, [](){increaseLayoutArg(LAYOUT_PADDING, 10);}, .mode = LAYOUT_MODE},
        {DEFAULT_MOD_MASK | ShiftMask, XK_p, [](){increaseLayoutArg(LAYOUT_PADDING, -10);}, .mode = LAYOUT_MODE},
        {DEFAULT_MOD_MASK, XK_b, [](){increaseLayoutArg(LAYOUT_NO_BORDER, 1);}, .mode = LAYOUT_MODE},
        {DEFAULT_MOD_MASK, XK_d, [](){increaseLayoutArg(LAYOUT_DIM, 1);}, .mode = LAYOUT_MODE},
        {DEFAULT_MOD_MASK, XK_t, [](){increaseLayoutArg(LAYOUT_TRANSFORM, 1);}, .mode = LAYOUT_MODE},
        {DEFAULT_MOD_MASK | ShiftMask, XK_t, [](){increaseLayoutArg(LAYOUT_TRANSFORM, -1);}, .mode = LAYOUT_MODE},
        {DEFAULT_MOD_MASK, XK_a, [](){increaseLayoutArg(LAYOUT_ARG, 1);}, .mode = LAYOUT_MODE},
        {DEFAULT_MOD_MASK | ShiftMask, XK_a, [](){increaseLayoutArg(LAYOUT_ARG, -1);}, .mode = LAYOUT_MODE},
        */
    };
    for(int i = 0; i < LEN(DEFAULT_BINDINGS); i++) {
        if(DEFAULT_BINDINGS[i].mod & Mod4Mask) {
            DEFAULT_BINDINGS[i].mod &= ~Mod4Mask;
            DEFAULT_BINDINGS[i].mod |= DEFAULT_MOD_MASK;
        }
    }
    for(Binding& binding : DEFAULT_BINDINGS)
        getDeviceBindings().add(binding);
}


void defaultPrintFunction(void) {
    dprintf(STATUS_FD, "%d ", getActiveMaster()->getCurrentMode());
    for(Workspace* w : getAllWorkspaces()) {
        const char* color;
        if(w->isVisible())
            color = "green";
        else if(doesWorkspaceHaveWindowWithMask(w->getID(), MAPPABLE_MASK))
            color = "yellow";
        else continue;
        dprintf(STATUS_FD, "^fg(%s)%s%s:%s^fg() ", color, w->getName().c_str(), w == getActiveWorkspace() ? "*" : "",
                w->getActiveLayout() ? w->getActiveLayout()->getName().c_str() : "");
    }
    if(getFocusedWindow() && getFocusedWindow()->isNotInInvisibleWorkspace()) {
        if(isLogging(LOG_LEVEL_DEBUG))
            dprintf(STATUS_FD, "%0xd ", getFocusedWindow()->getID());
        dprintf(STATUS_FD, "^fg(%s)%s^fg()", "green", getFocusedWindow()->getTitle().c_str());
    }
    dprintf(STATUS_FD, "\n");
}
void loadNormalSettings() {
    SHELL = getenv("SHELL");
    printStatusMethod = defaultPrintFunction;
    enableInterClientCommunication();
    addDefaultBindings();
}
void (*startupMethod)();
void onStartup(void) {
    addWorkspaces(DEFAULT_NUMBER_OF_WORKSPACES);
    addDefaultMaster();
    if(RUN_AS_WM) {
        addBasicRules();
        addAutoTileRules();
        addEWMHRules();
        addResumeRules();
    }
    if(!RUN_AS_WM)
        ROOT_EVENT_MASKS &= ~WM_MASKS;
    if(startupMethod)
        startupMethod();
    openXDisplay();
    assert(getActiveMaster());
    assert(getNumberOfWorkspaces());
}
