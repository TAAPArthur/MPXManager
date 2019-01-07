

#include "config.h"
#include <stdio.h>
#include <unistd.h>
#define NONFOCUSED_WINDOW_COLOR 0xdddddd

//char* workspaceNames[NUMBER_OF_WORKSPACES]={"Main"};

/*TODO moveto rule
unsigned int masterColors[]={
        0x00FF00,0x0000FF,0xFF0000,0x00FFFF,0xFFFFFF,0x000000
};
*/


Binding bindings[]={
    WORKSPACE_OPERATION(XK_1,0),
    WORKSPACE_OPERATION(XK_2,1),
    WORKSPACE_OPERATION(XK_3,2),
    WORKSPACE_OPERATION(XK_4,3),
    WORKSPACE_OPERATION(XK_5,4),
    WORKSPACE_OPERATION(XK_6,5),
    WORKSPACE_OPERATION(XK_7,6),
    WORKSPACE_OPERATION(XK_8,7),
    WORKSPACE_OPERATION(XK_9,8),
    WORKSPACE_OPERATION(XK_0,9),
    STACK_OPERATION(XK_Up,XK_Down,XK_Left,XK_Right),
    STACK_OPERATION(XK_H,XK_J,XK_K,XK_L),
    {0,Button1,BIND(activateWindow), .noGrab=1, .passThrough=1 ,.mask=XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS},

    {Mod1Mask,XK_Tab,AUTO_CHAIN_GRAB(0,1,
        {Mod1Mask,XK_Tab,BIND(cycleWindows,1),.noGrab=1},
        {Mod1Mask|ShiftMask,XK_Tab,BIND(cycleWindows,-1),.noGrab=1},
        END_CHAIN(Mod1Mask,XK_Alt_L,BIND(endCycleWindows)))},
    {Mod1Mask|ShiftMask,XK_Tab,AUTO_CHAIN_GRAB(0,1,
        {Mod1Mask|ShiftMask,XK_Tab,BIND(cycleWindows,-1),.noGrab=1},
        {Mod1Mask,XK_Tab,BIND(cycleWindows,1),.noGrab=1},
        END_CHAIN(Mod1Mask,XK_Alt_L,BIND(endCycleWindows)))},
    {Mod4Mask,XK_Return, BIND(shiftTop)},
    {Mod4Mask|ShiftMask,XK_Return, BIND(focusTop)},

    {0, XF86XK_AudioPlay, BIND((void (*)(void*))toggleLayout,&DEFAULT_LAYOUTS[FULL]),.passThrough=1 },
    {Mod4Mask, XK_F11, BIND((void (*)(void*))toggleLayout,&DEFAULT_LAYOUTS[FULL]),.passThrough=1 },

    {Mod4Mask,XK_space, BIND(cycleLayouts,1)},
    {Mod4Mask,XK_c, BIND(killFocusedWindow)},
    {Mod4Mask | ShiftMask ,XK_q, BIND(quit)}
};

Binding customBindings[]={

    {0,XK_F6, RUN_OR_RAISE_CLASS("arandr")},
    {Mod4Mask,XK_F6, RUN_OR_RAISE_CLASS("arandr")},

    {Mod4Mask | ControlMask | Mod1Mask |ShiftMask,XK_Up, SPAWN(".screenlayout/leftSetup.sh")},
    {Mod4Mask | ControlMask | Mod1Mask |ShiftMask,XK_Down, SPAWN(".screenlayout/rightSetup.sh")},
    {Mod4Mask | ControlMask | Mod1Mask |ShiftMask,XK_End, SPAWN(".screenlayout/baseSetup.sh")},
    {Mod4Mask | ControlMask | Mod1Mask |ShiftMask,XK_Home, SPAWN(".screenlayout/rebase.sh")},
    {Mod4Mask, XF86XK_MonBrightnessUp, SPAWN("Documents/Keyboard_Commands/brightness .1")},
    {Mod4Mask |ShiftMask, XF86XK_MonBrightnessUp, SPAWN("Documents/Keyboard_Commands/brightness -a 1")},
    {Mod4Mask, XF86XK_MonBrightnessDown, SPAWN("Documents/Keyboard_Commands/brightness -.1")},
    {Mod4Mask |ShiftMask, XF86XK_MonBrightnessDown, SPAWN("Documents/Keyboard_Commands/brightness -a 1")},
    {Mod4Mask | ControlMask , XF86XK_MonBrightnessDown, SPAWN("xrandr-invert-colors")},
    {Mod4Mask | ControlMask , XF86XK_MonBrightnessUp, SPAWN("xrandr-invert-colors")},
    {0 , XF86XK_Tools, SPAWN("xrandr-invert-colors")},

    {0, XF86XK_AudioLowerVolume, SPAWN("pamixer -d 5")},
    {0, XF86XK_AudioRaiseVolume, SPAWN("pamixer -i 5")},
    {0, XF86XK_AudioMute, SPAWN("pamixer -t")},
    {Mod4Mask, XF86XK_AudioLowerVolume, SPAWN("omnivolctrl -5%")},
    {Mod4Mask, XF86XK_AudioRaiseVolume, SPAWN("omnivolctrl +5%")},
    {Mod4Mask, XF86XK_AudioMute, SPAWN("omnivolctrl sink-input-mute toggle")},
    {Mod4Mask, XF86XK_AudioPrev, SPAWN("omnivolctrl sink-inputs")},
    {0, XF86XK_AudioPrev, SPAWN("omnivolctrl sinks")},

    {Mod4Mask, XK_Menu, RUN_OR_RAISE_CLASS("pavucontrol")},


    {Mod4Mask,XK_w, RUN_OR_RAISE_ENV_CLASS("$BROWSER")},
    {Mod4Mask | ShiftMask ,XK_w, SPAWN("$BROWSER --new-window")},
    {Mod4Mask | ControlMask,XK_w, SPAWN("google-chrome-stable --new-window --disk-cache-size=1000")},
    {Mod4Mask | ControlMask| ShiftMask,XK_w, SPAWN("google-chrome-stable --new-window")},
    {Mod4Mask,XK_f, RUN_OR_RAISE_CLASS("firefox")},
    {Mod4Mask | ShiftMask ,XK_f, SPAWN(" firefox --new-window")},

    {Mod4Mask,XK_v, RUN_OR_RAISE_CLASS("vlc")},
    {Mod4Mask | ControlMask ,XK_v, RUN_OR_RAISE_CLASS("virtualbox")},
    {Mod4Mask | ControlMask | Mod1Mask ,XK_v, RUN_OR_RAISE_CLASS("pitivi")},
    {Mod4Mask | ControlMask | Mod1Mask ,XK_a, RUN_OR_RAISE_CLASS("audacity")},
    {Mod4Mask | ControlMask, XK_z, RUN_OR_RAISE_CLASS("zim")},
    {Mod4Mask | ControlMask,XK_g, RUN_OR_RAISE_CLASS("gummi")},
    //{Mod4Mask ,XK_n, RUN_OR_RAISE_CLASS("write_stylus")},
    {Mod4Mask | ControlMask,XK_b, RUN_OR_RAISE_TYPE( "App.py",CLASS, "backintime-qt4")},
    {Mod4Mask | ControlMask,XK_d, RUN_OR_RAISE_CLASS("dolphin-emu")},
    {Mod4Mask ,XK_s, RUN_OR_RAISE_CLASS("steam")},
    {Mod4Mask,XK_a, RUN_OR_RAISE_CLASS("atril")},
    {Mod4Mask,XK_g, RUN_OR_RAISE_CLASS("gedit")},
    {Mod4Mask | ShiftMask,XK_g, SPAWN("gedit -s")},
    {Mod4Mask,XK_d, RUN_OR_RAISE_CLASS("thunar")},
    {Mod4Mask | ShiftMask,XK_d, SPAWN("thunar")},
    {ControlMask | Mod1Mask,XK_t, RUN_OR_RAISE_ENV_CLASS("$TERMINAL")},
    {ControlMask | Mod1Mask | ShiftMask,XK_t, SPAWN("$TERMINAL")},


    {Mod4Mask,XK_p, SPAWN("dmenu_run_history")},
    {Mod4Mask | ControlMask,XK_p, SPAWN("dmenu_extended_run")},
    {Mod4Mask | ControlMask, XK_Menu, RUN_OR_RAISE_CLASS("blueman-manager")},
    {Mod4Mask | ControlMask ,XK_Print, RUN_OR_RAISE_CLASS("simplescreenrecorder")},
    {0, XF86XK_Eject, SPAWN("dmenu-pycalc")},
    {0, XF86XK_Calculator, SPAWN("dmenu-pycalc")},
    {Mod4Mask,XK_Delete, RUN_OR_RAISE_ENV_TITLE("$TERMINAL -e gt5 -T gt5","gt5")},

    {ControlMask | Mod1Mask , XK_Delete, RUN_OR_RAISE_CLASS("xfce4-taskmanager")},
    {Mod4Mask | ControlMask | Mod1Mask  , XK_l, SPAWN("xtrlock")},
    {Mod4Mask, XK_Escape, SPAWN("xtrlock")},
    {ControlMask | Mod1Mask, XK_Escape, SPAWN("systemctl suspend")},
    {Mod4Mask | ControlMask | Mod1Mask, XK_Escape, SPAWN("notify-send 'System will shutdown in 1 min'; shutdown 1")},
    {Mod4Mask | ControlMask | Mod1Mask ,XK_d, SPAWN("dual-pointer-x -s")},
    {Mod4Mask | ControlMask | Mod1Mask ,XK_e, SPAWN("dual-pointer-x -d")},
    {Mod4Mask | ControlMask | Mod1Mask |ShiftMask,XK_d, SPAWN("dual-pointer-x --reset")},




    {Mod4Mask | ShiftMask,XK_asciitilde, BIND(printSummary)},
    {Mod4Mask |ControlMask |ShiftMask,XK_asciitilde, BIND(dumpAllWindowInfo)},
    {Mod4Mask ,XK_grave, BIND(resetUserMask)},
};
/*
Binding onKeyPressBindings[] = {





    //{.arbitArgs={(*funcVoidArg)(Context *,void*)setLayout,&DEFAULT_LAYOUTS[FULL]}},ARBIT_ARG
    //{.arbitArgs={(*funcVoidArg)(Context *,void*)&DEFAULT_LAYOUTS[FULL],}},ARBIT_ARG


    //{Mod4Mask,XK_h, BIND_TO_STR_FUNC(runOrRaiseClassMatch,"/opt/google/chrome/google-chrome --profile-directory=Default --app-id=knipolnnllmklapflnccelgolnpehhpl" (fmap ( "Google Hangouts") `isPrefixOf`) title )},


    {ControlMask, XK_Menu, SPAWN("xfce4-popup-clipman")},
    {0,XK_Print, SPAWN("xfce4-screenshooter -r")},
    {Mod4Mask,XK_Print, SPAWN("xfce4-screenshooter -f")},
    {ControlMask,XK_Print, SPAWN("xfce4-screenshooter -w")},
    {ShiftMask,XK_Print, SPAWN("xfce4-screenshooter")},

    // {Mod4Mask | ControlMask | Mod1Mask ,XK_a, SPAWN("antimicro")},
    {Mod4Mask | ControlMask | Mod1Mask,XK_c, SPAWN("gksudo Documents/Keyboard_Commands/changeMacAddress")},
    {Mod4Mask | ControlMask | Mod1Mask | ShiftMask,XK_c, SPAWN("gksudo Documents/Keyboard_Commands/changeMacAddress -r")},
    //{Mod4Mask | ControlMask,XK_w, BIND_TO_STR_FUNC(runOrRaiseClassMatch,"google-chrome-stable" (className=?"Google-chrome"<&&>  negateQuery (fmap ( "Google Hangouts") `isPrefixOf` ) title))},


    //{Mod4Mask,XK_m, RUN_OR_RAISE_CLASS("thunderbird")},
};
*/

void printFunction(void){
    const char* format="<fc=%s>%s:%s</fc> ";
    for(int i=0;i<getNumberOfWorkspaces();i++){
        char * color;
        if(isWorkspaceVisible(i))
            color="green";
        else if(isWorkspaceNotEmpty(i))
            color="yellow";
        else continue;
        dprintf(STATUS_FD,format,color,getWorkspaceName(i),getNameOfLayout(getActiveLayoutOfWorkspace(i)));
    }
    if(getFocusedWindow())
        dprintf(STATUS_FD,"<fc=%s>%s</fc>","green",getFocusedWindow()->title);
    dprintf(STATUS_FD,"\n");
    fsync(STATUS_FD);
}
void loadSettings(){
    SHELL=getenv("SHELL");
    spawnPipe("xmobar");
    printStatusMethod=printFunction;
    addBindings(bindings,LEN(bindings));
    addBindings(customBindings,LEN(customBindings));
    /*
    ADD_WINDOW_RULE(ProcessingWindow, CREATE_LITERAL_RULE("_NET_WM_WINDOW_TYPE_DIALOG",TYPE,BIND_TO_WIN_FUNC(floatWindow)));
    ADD_WINDOW_RULE(ProcessingWindow, CREATE_RULE(".*__WM_IGNORE.*",TITLE,NULL));
    ADD_WINDOW_RULE(ProcessingWindow,CREATE_RULE("Eclipse",CLASS,BIND_TO_STR_INT_FUNC(sendWindowToWorkspaceByName,"Writer",0)));

    */
}
