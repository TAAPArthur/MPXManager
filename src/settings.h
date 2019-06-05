/**
 * @file settings.h
 * @brief contains default user settings
 */


#ifndef SETTINGS_H_
#define SETTINGS_H_

/**
 * Creates a set of bindings related to switching workspaces
 * @param K the key to bind; various modifiers will be used for the different functions
 * @param N the workspace to switch to/act on
 */
#define WORKSPACE_OPERATION(K,N) \
    {Mod4Mask,             K, AND(BIND(switchToWorkspace,N),BIND(activateWorkspace,N))}, \
    {Mod4Mask|ShiftMask,   K, BIND(moveWindowToWorkspace,N)}, \
    {Mod4Mask|ControlMask,   K, AND(BIND(swapWithWorkspace,N),BIND(activateWorkspace,N))}

/**
 * Creates a set of bindings related to the windowStack
 * @param KEY_UP
 * @param KEY_DOWN
 * @param KEY_LEFT
 * @param KEY_RIGHT
 */
#define STACK_OPERATION(KEY_UP,KEY_DOWN,KEY_LEFT,KEY_RIGHT) \
    {Mod4Mask,             KEY_UP, BIND(swapPosition,UP)}, \
    {Mod4Mask,             KEY_DOWN, BIND(swapPosition,DOWN)}, \
    {Mod4Mask,             KEY_LEFT, BIND(shiftFocus,UP)}, \
    {Mod4Mask,             KEY_RIGHT, BIND(shiftFocus,DOWN)}

/**
 * Load default settings
 */
void loadNormalSettings(void);

/**
 * Prints names of workspaces that are visible or have windows.
 * The layout of each workspace is also printed
 * The title of the active window is printed and the active workspace is marked
 */
void defaultPrintFunction(void);

/**
 * Will be called on startup before the XServer is initialized so the user can set global vars, run
 * arbitrary commands and most commonly add device bindings and event rules
 */
void loadSettings(void);
#endif
