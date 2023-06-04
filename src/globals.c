#include <xcb/xinput.h>

#include "globals.h"
#include "window-masks.h"

bool ALLOW_SETTING_UNSYNCED_MASKS = 0;
bool ALLOW_UNSAFE_OPTIONS = 1;
bool ASSUME_PRIMARY_MONITOR = 0;
bool HIDE_WM_STATUS = 0;
bool RUN_AS_WM = 1;
bool STEAL_WM_SELECTION = 0;
const char* MASTER_INFO_PATH = "$HOME/.config/mpxmanager/master-info.txt";
const char* SHELL = "/bin/sh";
int16_t DEFAULT_BORDER_WIDTH = 1;
uint32_t AUTO_FOCUS_NEW_WINDOW_TIMEOUT = 4000;
uint32_t CRASH_ON_ERRORS = 0;
uint32_t DEFAULT_BORDER_COLOR = 0x00FF00;
uint32_t DEFAULT_NUMBER_OF_WORKSPACES = 10;
uint32_t DEFAULT_UNFOCUS_BORDER_COLOR = 0xDDDDDD;
uint32_t IGNORE_MASK = Mod2Mask;
uint32_t KILL_TIMEOUT = 100;
uint32_t NON_ROOT_DEVICE_EVENT_MASKS = XCB_INPUT_XI_EVENT_MASK_FOCUS_OUT | XCB_INPUT_XI_EVENT_MASK_FOCUS_IN;
uint32_t NON_ROOT_EVENT_MASKS = XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_VISIBILITY_CHANGE;
uint32_t IDLE_TIMEOUT = 20;
uint32_t IDLE_TIMEOUT_CLI_SEC = 1;
uint32_t ROOT_DEVICE_EVENT_MASKS = XCB_INPUT_XI_EVENT_MASK_HIERARCHY | XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS;
uint32_t ROOT_EVENT_MASKS = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
    XCB_EVENT_MASK_STRUCTURE_NOTIFY;
uint32_t SRC_INDICATION = 7;

WindowMask MASKS_TO_SYNC = MODAL_MASK | ABOVE_MASK | BELOW_MASK | HIDDEN_MASK | NO_TILE_MASK | STICKY_MASK |
    URGENT_MASK;
