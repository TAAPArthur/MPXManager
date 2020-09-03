/**
 * @file
 */

#include <stdlib.h>

#include <X11/keysym.h>

#include "chain.h"
#include "threads.h"
#include "communications.h"
#include "ewmh.h"
#include "extra-rules.h"
#include "functions.h"
#include "globals.h"
#include "devices.h"
#include "layouts.h"
#include "logger.h"
#include "masters.h"
#include "monitors.h"
#include "settings.h"
#include "system.h"
#include "window-properties.h"
#include "windows.h"
#include "wm-rules.h"
#include "wmfunctions.h"
#include "workspaces.h"

/// Add default chain bindings
void addChainDefaultBindings() {
    Chain* DEFAULT_CHAIN_BINDINGS[] = {
        new Chain({{Mod1Mask, XK_Tab}, {Mod1Mask | ShiftMask, XK_Tab}, {Mod4Mask, XK_Tab},}, {}, {
            Binding{Mod1Mask, XK_Tab, {cycleWindows, DOWN}, {.passThrough = NO_PASSTHROUGH, .noGrab = 1}, "_cycleWindowsDown"}, \
            Binding{Mod1Mask | ShiftMask, XK_Tab, {cycleWindows, UP}, {.passThrough = NO_PASSTHROUGH, .noGrab = 1}, "_cycleWindowsUp"}, \
            Binding{Mod4Mask, XK_Tab, []{cycleWindowsMatching(DOWN, matchesFocusedWindowClass);}, {.passThrough = NO_PASSTHROUGH, .noGrab = 1}, "_cycleWindowsMDown"}, \
            Binding{Mod4Mask | ShiftMask, XK_Tab, []{cycleWindowsMatching(UP, matchesFocusedWindowClass);}, {.passThrough = NO_PASSTHROUGH, .noGrab = 1}, "_cycleWindowsUp"}, \
            Binding{{{WILDCARD_MODIFIER, XK_Alt_L}, {WILDCARD_MODIFIER, XK_Super_L} }, []{endCycleWindows(); endActiveChain();}, {.passThrough = ALWAYS_PASSTHROUGH, .noGrab = 1, .mask = XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE}, "_endCycleWindows" }, \
            Binding {WILDCARD_MODIFIER, 0, {}, {.passThrough = NO_PASSTHROUGH, .noGrab = 1}, "_absorbCycleWindows"}, \
        }, {}, XCB_INPUT_XI_EVENT_MASK_KEY_PRESS | XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE, "_cycleWindows"),
        new Chain{
            DEFAULT_MOD_MASK, Button1, {+[](WindowInfo * winInfo) {startWindowMoveResize(winInfo, 1);}},
            {
                {DEFAULT_MOD_MASK, 0, DEFAULT_EVENT(updateWindowMoveResize), {.passThrough = NO_PASSTHROUGH, .noGrab = 1, .mask = XCB_INPUT_XI_EVENT_MASK_MOTION | XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS}},
                {WILDCARD_MODIFIER, XK_Escape, DEFAULT_EVENT(cancelWindowMoveResize), {.passThrough = ALWAYS_PASSTHROUGH, .noGrab = 1}},
                {WILDCARD_MODIFIER, 0, DEFAULT_EVENT(commitWindowMoveResize), {.passThrough = ALWAYS_PASSTHROUGH, .noGrab = 1}},
            }, {},
            XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS | XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE | XCB_INPUT_XI_EVENT_MASK_MOTION,
            "_windowMove"
        },
        new Chain{
            DEFAULT_MOD_MASK, Button3, {+[](WindowInfo * winInfo) {startWindowMoveResize(winInfo, 0);}},
            {
                {DEFAULT_MOD_MASK, 0, updateWindowMoveResize, {.passThrough = NO_PASSTHROUGH, .noGrab = 1, .mask = XCB_INPUT_XI_EVENT_MASK_MOTION | XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS}},
                {WILDCARD_MODIFIER, XK_Escape, cancelWindowMoveResize, {.noGrab = 1}},
                {WILDCARD_MODIFIER, 0, commitWindowMoveResize, {.noGrab = 1}},
            },
            {}, XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS | XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE | XCB_INPUT_XI_EVENT_MASK_MOTION,
            "_windowResize"
        },
    };
    for(Chain* binding : DEFAULT_CHAIN_BINDINGS)
        getDeviceBindings().add(binding);
}
/**
 * Creates a set of bindings related to switching workspaces
 * @param K the key to bind; various modifiers will be used for the different functions
 * @param N the workspace to switch to/act on
 */
#define WORKSPACE_OPERATION(K,N) \
    {DEFAULT_MOD_MASK, K, +[](){switchToWorkspace(N); ;}}, \
    {DEFAULT_MOD_MASK|ShiftMask, K, +[](WindowInfo*winInfo){winInfo->moveToWorkspace(N);}}, \
    {DEFAULT_MOD_MASK|ControlMask, K, +[](){swapMonitors(getActiveWorkspaceIndex(),N);}}

/**
 * Creates a set of bindings related to the windowStack
 * @param KEY_UP
 * @param KEY_DOWN
 * @param KEY_LEFT
 * @param KEY_RIGHT
 */
#define STACK_OPERATION(KEY_UP,KEY_DOWN,KEY_LEFT,KEY_RIGHT) \
    {DEFAULT_MOD_MASK, KEY_UP, +[](){swapPosition(UP);}}, \
    {DEFAULT_MOD_MASK, KEY_DOWN, +[](){swapPosition(DOWN);}}, \
    {DEFAULT_MOD_MASK, KEY_LEFT, +[](){shiftFocus(UP);}}, \
    {DEFAULT_MOD_MASK | ShiftMask, KEY_LEFT, +[](){shiftFocus(UP, matchesFocusedWindowClass);}}, \
    {DEFAULT_MOD_MASK, KEY_RIGHT, +[](){shiftFocus(DOWN);}}, \
    {DEFAULT_MOD_MASK | ShiftMask, KEY_RIGHT, +[](){shiftFocus(DOWN, matchesFocusedWindowClass);}} \

/// add default bindings
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

        {WILDCARD_MODIFIER, Button1, activateWorkspaceUnderMouse, {.passThrough = ALWAYS_PASSTHROUGH, .noGrab = 1}},
        {WILDCARD_MODIFIER, Button1, activateWindow, {.passThrough = ALWAYS_PASSTHROUGH, .noGrab = 1}},
        {0, Button1, replayPointerEvent},

        {DEFAULT_MOD_MASK, XK_c, killClientOfWindowInfo, {.noKeyRepeat = 1}},
        {DEFAULT_MOD_MASK | ShiftMask, XK_c, killClientOfWindowInfo, { .windowTarget = TARGET_WINDOW, .noKeyRepeat = 1}},
        {DEFAULT_MOD_MASK | ControlMask, XK_c, destroyWindow, {.noKeyRepeat = 1}},
        {DEFAULT_MOD_MASK | ControlMask | ShiftMask, XK_c, destroyWindow, { .windowTarget = TARGET_WINDOW, .noKeyRepeat = 1}},
        {DEFAULT_MOD_MASK, Button1, floatWindow, { .passThrough = ALWAYS_PASSTHROUGH, .mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS}},
        {DEFAULT_MOD_MASK, Button3, floatWindow, { .passThrough = ALWAYS_PASSTHROUGH, .mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS}},
        {DEFAULT_MOD_MASK | ShiftMask, Button1, sinkWindow, {.passThrough = ALWAYS_PASSTHROUGH, .mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS}},
        {DEFAULT_MOD_MASK, XK_t, sinkWindow},
        {DEFAULT_MOD_MASK | Mod1Mask, XK_t, {toggleMask, STICKY_MASK}},
        {DEFAULT_MOD_MASK, XK_u, +[]{activateNextUrgentWindow();}},
        {DEFAULT_MOD_MASK | ShiftMask, XK_y, +[]{popHiddenWindow();}},
        {DEFAULT_MOD_MASK, XK_y, {toggleMask, HIDDEN_MASK}},

        {DEFAULT_MOD_MASK, XK_F11, +[]() {toggleLayout(FULL);}},
        {DEFAULT_MOD_MASK, XK_F, {toggleMask, FULLSCREEN_MASK}},
        {DEFAULT_MOD_MASK | ShiftMask, XK_F, {toggleMask, ROOT_FULLSCREEN_MASK}},
        {DEFAULT_MOD_MASK | ShiftMask, XK_F11, {toggleMask, FULLSCREEN_MASK}},

        {DEFAULT_MOD_MASK, XK_space, +[]() {cycleLayouts(DOWN);}},
        {DEFAULT_MOD_MASK | ShiftMask, XK_space, +[]() {cycleLayouts(UP);}},

        {DEFAULT_MOD_MASK, XK_Return, +[]() {shiftTopAndFocus();}},

        {DEFAULT_MOD_MASK | ControlMask | ShiftMask, XK_q, requestShutdown},
        {DEFAULT_MOD_MASK | ShiftMask, XK_q, []{restart();}},
        {DEFAULT_MOD_MASK, XK_q, +[]() {if(waitForChild(spawn("mpxmanager --recompile")) == 0)restart();}},

        {DEFAULT_MOD_MASK, XK_F1, +[](){getActiveMaster()->setCurrentMode(0);}, {.mode = ANY_MODE}},
        {DEFAULT_MOD_MASK, XK_F2, +[](){getActiveMaster()->setCurrentMode(1);}, {.mode = ANY_MODE}},
        {DEFAULT_MOD_MASK, XK_F3, +[](){getActiveMaster()->setCurrentMode(2);}, {.mode = ANY_MODE}},
        {DEFAULT_MOD_MASK, XK_F4, +[](){getActiveMaster()->setCurrentMode(3);}, {.mode = ANY_MODE}},

        {DEFAULT_MOD_MASK, XK_r, retile},
        {DEFAULT_MOD_MASK | ShiftMask, XK_r, +[](){if(getActiveLayout())getActiveLayout()->restoreArgs();}, {.mode = LAYOUT_MODE}},
        {DEFAULT_MOD_MASK, XK_p, +[](){increaseLayoutArg(LAYOUT_PADDING, 10);}, {.mode = LAYOUT_MODE}},
        {DEFAULT_MOD_MASK | ShiftMask, XK_p, +[](){increaseLayoutArg(LAYOUT_PADDING, -10);}, {.mode = LAYOUT_MODE}},
        {DEFAULT_MOD_MASK, XK_b, +[](){increaseLayoutArg(LAYOUT_NO_BORDER, 1);}, {.mode = LAYOUT_MODE}},
        {DEFAULT_MOD_MASK, XK_d, +[](){increaseLayoutArg(LAYOUT_DIM, 1);}, {.mode = LAYOUT_MODE}},
        {DEFAULT_MOD_MASK, XK_t, +[](){increaseLayoutArg(LAYOUT_TRANSFORM, 1);}, {.mode = LAYOUT_MODE}},
        {DEFAULT_MOD_MASK | ShiftMask, XK_t, +[](){increaseLayoutArg(LAYOUT_TRANSFORM, -1);}, {.mode = LAYOUT_MODE}},
        {DEFAULT_MOD_MASK, XK_a, +[](){increaseLayoutArg(LAYOUT_ARG, 1);}, {.mode = LAYOUT_MODE}},
        {DEFAULT_MOD_MASK | ShiftMask, XK_a, +[](){increaseLayoutArg(LAYOUT_ARG, -1);}, {.mode = LAYOUT_MODE}},
    };
    for(Binding& binding : DEFAULT_BINDINGS)
        getDeviceBindings().add(binding);
}


void defaultPrintFunction(void) {
    if(STATUS_FD == 0)
        return;
    if(isLogging(LOG_LEVEL_DEBUG)) {
        dprintf(STATUS_FD, "[%d]: ", getIdleCount());
    }
    if(getActiveMaster()->getCurrentMode())
        dprintf(STATUS_FD, "%d: ", getActiveMaster()->getCurrentMode());
    if(getAllMasters().size() > 1)
        dprintf(STATUS_FD, "%dM %dA ", getAllMasters().size(), getActiveMasterKeyboardID());
    for(Workspace* w : getAllWorkspaces()) {
        const char* color;
        if(w->hasWindowWithMask(URGENT_MASK))
            color = "cyan";
        else if(w->isVisible())
            color = "green";
        else if(w->hasWindowWithMask(MAPPABLE_MASK))
            color = "yellow";
        else continue;
        dprintf(STATUS_FD, "^fg(%s)%s%s:%s^fg() ", color, w->getName().c_str(), w == getActiveWorkspace() ? "*" : "",
            w->getActiveLayout() ? w->getActiveLayout()->getName().c_str() : "");
    }
    if(getActiveChain())
        dprintf(STATUS_FD, "[%s (%d)] ", getActiveChain()->getName().c_str(), getNumberOfActiveChains());
    if(getActiveMaster()->getCurrentMode())
        dprintf(STATUS_FD, "{%d} ", getActiveMaster()->getCurrentMode());
    if(getFocusedWindow() && getFocusedWindow()->isNotInInvisibleWorkspace()) {
        if(isLogging(LOG_LEVEL_DEBUG))
            dprintf(STATUS_FD, "%0xd ", getFocusedWindow()->getID());
        dprintf(STATUS_FD, "^fg(%s)%s^fg()", "green", getFocusedWindow()->getTitle().c_str());
    }
    else {
        dprintf(STATUS_FD, "Focused on %0xd (root: %0xd)", getActiveFocus(), root);
    }
    dprintf(STATUS_FD, "\n");
}
void loadNormalSettings() {
    INFO("Loading normal settings");
    printStatusMethod = defaultPrintFunction;
    addDefaultBindings();
    addChainDefaultBindings();
}
void addSuggestedRules() {
    addApplyChainBindingsRule();
    addAutoFocusRule();
    addAutoTileRules();
    addDesktopRule();
    addEWMHRules();
    addFloatRule();
    addIgnoreInputOnlyWindowsRule();
    addIgnoreNonTopLevelWindowsRule();
    addInterClientCommunicationRule();
    addKeepTransientsOnTopRule();
    addMoveNonTileableWindowsToWorkspaceBounds();
    addPrintStatusRule();
    addStickyPrimaryMonitorRule();
}
void __attribute__((weak)) loadSettings(void) {
    addSuggestedRules();
    loadNormalSettings();
}
void (*startupMethod)();
void onStartup(void) {
    INFO("Starting up");
    addDefaultMaster();
    addWorkspaces(DEFAULT_NUMBER_OF_WORKSPACES);
    addBasicRules();
    if(!RUN_AS_WM)
        ROOT_EVENT_MASKS &= ~WM_MASKS;
    if(startupMethod)
        startupMethod();
    lock();
    openXDisplay();
    unlock();
}
