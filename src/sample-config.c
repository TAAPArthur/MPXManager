#include "config.h"
#include "settings.c"

Binding customBindings[]={
    {Mod4Mask | ShiftMask,XK_asciitilde, BIND(printSummary)},
    {Mod4Mask |ControlMask |ShiftMask,XK_asciitilde, BIND(dumpAllWindowInfo)},
    {Mod4Mask ,XK_grave, BIND(resetUserMask)},
};

void loadSettings(void){
    SHELL=getenv("SHELL");
    loadNormalSettings();
    addBindings(customBindings,LEN(customBindings));
}
