/**
 * @file settings.c
 * @copybrief settings.h
 */

#include <stdlib.h>

#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#include <xcb/xinput.h>

#include "args.h"
#include "functions.h"
#include "globals.h"
#include "layouts.h"
#include "logger.h"
#include "masters.h"
#include "mpx.h"
#include "mywm-util.h"
#include "settings.h"
#include "windows.h"
#include "wmfunctions.h"
#include "workspaces.h"

static Binding DEFAULLT_BINDINGS[] = {
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
    {WILDCARD_MODIFIER, Button1, AND(BIND(activateWorkspaceUnderMouse), BIND(raiseWindowInfo), BIND(focusWindowInfo)), .noGrab = 1, .passThrough = ALWAYS_PASSTHROUGH, .mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS},
    {Mod4Mask, XK_c, BIND(killWindowInfo)},
    {Mod4Mask | ShiftMask, XK_c, BIND(killWindowInfo), .windowTarget = TARGET_WINDOW},
    {Mod4Mask, Button1, BIND(floatWindow), .passThrough = ALWAYS_PASSTHROUGH, .mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS},
    {Mod4Mask | ShiftMask, Button1, BIND(sinkWindow), .passThrough = ALWAYS_PASSTHROUGH, .mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS},
    {Mod4Mask, XK_t, BIND(sinkWindow)},

    {0, XF86XK_AudioPlay, OR(BIND(toggleLayout, &DEFAULT_LAYOUTS[FULL]), BIND(cycleLayouts, DOWN))},
    {Mod4Mask, XK_F11, OR(BIND(toggleLayout, &DEFAULT_LAYOUTS[FULL]), BIND(cycleLayouts, DOWN))},
    {Mod4Mask, XF86XK_AudioPlay, BIND(toggleMask, FULLSCREEN_MASK) },
    {Mod4Mask | ShiftMask, XF86XK_AudioPlay, BIND(toggleMask, ROOT_FULLSCREEN_MASK) },

    {Mod4Mask, XK_space, BIND(cycleLayouts, DOWN)},

    {
        Mod4Mask, Button1, BOTH(BIND(startMouseOperation), CHAIN_GRAB(XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE | XCB_INPUT_XI_EVENT_MASK_MOTION,
        {Mod4Mask, Button1, BIND(moveWindowWithMouse), .mask = XCB_INPUT_XI_EVENT_MASK_MOTION | XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS, .noGrab = 1, .passThrough = NO_PASSTHROUGH},
        {Mod4Mask, 0, BIND(moveWindowWithMouse), .mask = XCB_INPUT_XI_EVENT_MASK_MOTION, .noGrab = 1, .passThrough = NO_PASSTHROUGH},
        END_CHAIN(WILDCARD_MODIFIER, 0, BIND(stopMouseOperation), .mask = -1, .noGrab = 1))),
        .mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS
    },
    {
        Mod4Mask, Button3, BOTH(BIND(startMouseOperation), CHAIN_GRAB(XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE | XCB_INPUT_XI_EVENT_MASK_MOTION,
        {Mod4Mask, Button3, BIND(resizeWindowWithMouse), .mask = XCB_INPUT_XI_EVENT_MASK_MOTION | XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS, .noGrab = 1, .passThrough = NO_PASSTHROUGH},
        {Mod4Mask, 0, BIND(resizeWindowWithMouse), .mask = XCB_INPUT_XI_EVENT_MASK_MOTION, .noGrab = 1, .passThrough = NO_PASSTHROUGH},
        END_CHAIN(WILDCARD_MODIFIER, 0, BIND(stopMouseOperation), .mask = -1, .noGrab = 1))),
        .mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS
    },
    //{Mod4Mask,Button2,BIND(resizeWindowWithMouse), .noGrab=1, .passThrough=1 ,.mask=XCB_INPUT_XI_EVENT_MASK_MOTION},

    {
        Mod1Mask, XK_Tab, AUTO_CHAIN_GRAB(XCB_INPUT_XI_EVENT_MASK_KEY_PRESS | XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE, 1,
        {Mod1Mask, XK_Tab, BIND(cycleWindows, DOWN), .noGrab = 1, .passThrough = NO_PASSTHROUGH},
        {Mod1Mask | ShiftMask, XK_Tab, BIND(cycleWindows, UP), .noGrab = 1, .passThrough = NO_PASSTHROUGH},
        END_CHAIN(WILDCARD_MODIFIER, XK_Alt_L, BIND(endCycleWindows), .noGrab = 1, .noEndOnPassThrough = 1, .mask = XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE))
    },

    {
        Mod1Mask | ShiftMask, XK_Tab, AUTO_CHAIN_GRAB(XCB_INPUT_XI_EVENT_MASK_KEY_PRESS | XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE, 1,
        {Mod1Mask | ShiftMask, XK_Tab, BIND(cycleWindows, UP), .noGrab = 1, .passThrough = NO_PASSTHROUGH},
        {Mod1Mask, XK_Tab, BIND(cycleWindows, DOWN), .noGrab = 1, .passThrough = NO_PASSTHROUGH},
        END_CHAIN(WILDCARD_MODIFIER, XK_Alt_L, BIND(endCycleWindows), .noGrab = 1, .noEndOnPassThrough = 1, .mask = XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE))
    },

    {Mod4Mask, XK_Return, BIND(shiftTop)},
    {Mod4Mask | ShiftMask, XK_Return, BIND(focusTop)},

    {Mod4Mask | ShiftMask, XK_q, BIND(quit)},
    {Mod4Mask, XK_q, AND(PIPE(BIND(spawn, "mpxmanager --recompile"), BIND(waitForChild)), BIND(restart))},

    {Mod4Mask | ShiftMask, XK_asciitilde, BIND(printSummary)},
    {Mod4Mask | ControlMask | ShiftMask, XK_asciitilde, BIND(dumpAllWindowInfo)},
};

void defaultPrintFunction(void){
    int activeWorkspaceIndex = getActiveWorkspaceIndex();
    for(int i = 0; i < getNumberOfWorkspaces(); i++){
        char* color;
        if(isWorkspaceVisible(i))
            color = "green";
        else if(doesWorkspaceHaveWindowsWithMask(i, MAPPABLE_MASK))
            color = "yellow";
        else continue;
        dprintf(STATUS_FD, "<fc=%s>%s%s:%s</fc> ", color, getWorkspaceName(i), i == activeWorkspaceIndex ? "*" : "",
                getNameOfLayout(getActiveLayoutOfWorkspace(i)));
    }
    if(getFocusedWindow() && isWindowNotInInvisibleWorkspace(getFocusedWindow()))
        dprintf(STATUS_FD, "<fc=%s>%s</fc>", "green", getFocusedWindow()->title);
    dprintf(STATUS_FD, "\n");
}
void loadNormalSettings(){
    addAutoMPXRules();
    SHELL = getenv("SHELL");
    printStatusMethod = defaultPrintFunction;
    addBindings(DEFAULLT_BINDINGS, LEN(DEFAULLT_BINDINGS));
    startReadingUserInput();
}
