/**
 * @file settings.h
 * @brief contains default user settings
 */


#ifndef MPX_SETTINGS_H_
#define MPX_SETTINGS_H_

/// Layout related keybindings
#define LAYOUT_MODE 1

/// Set an global var from the env
#define SET_FROM_ENV(VAR) if(getenv("MPX" # VAR)) VAR = atoi("MPX" # VAR)

/// User specified start up method to run before connection to X
extern void (*startupMethod)();
/**
 * Loads default settings
 */
void onStartup(void);

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
/**
 * Adds a collection of rules that aren't strictly needed to call this a functioning WM, but may align with expected
 * behavior
 */
void addSuggestedRules();

#define START_WINDOW_MOVE_RESIZE_CHAIN_MEMBERS \
    CHAIN_MEM( \
            {WILDCARD_MODIFIER, 0, {updateWindowMoveResize}, .flags = {.shortCircuit = 1, .noGrab = 1, .mask = XCB_INPUT_XI_EVENT_MASK_MOTION | XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS}}, \
            {WILDCARD_MODIFIER, XK_Escape, {cancelWindowMoveResize}, .flags = {.noGrab = 1, .popChain = 1}}, \
            {WILDCARD_MODIFIER, 0, {commitWindowMoveResize}, .flags = {.noGrab = 1,.mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE, .popChain = 1}},)

#define CYCLE_BINDINGS(MOD, REV_MOD, KEY, FUNC, END_KEY) \
    { \
        MOD, KEY, {setFocusStackFrozen, {1}}, .flags = {.grabDevice = 1, .ignoreMod = REV_MOD}, .chainMembers = CHAIN_MEM( \
        {MOD, KEY, {FUNC, {DOWN}}, .flags = {.noGrab = 1}}, \
        {MOD| REV_MOD, KEY, {FUNC, {UP}}, .flags = {.noGrab = 1}}, \
        {WILDCARD_MODIFIER, END_KEY, {setFocusStackFrozen, {0}}, .flags = {.noGrab = 1, .popChain = 1, .mask = XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE}}, \
        ) \
    }
#endif
