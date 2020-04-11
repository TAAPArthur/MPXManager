#include <X11/extensions/XInput2.h>
#include <xcb/xinput.h>

#include "globals.h"
#include "window-masks.h"

bool ALLOW_SETTING_UNSYNCED_MASKS = 0;
bool LD_PRELOAD_INJECTION = 0;
// TODO start up option
bool RUN_AS_WM = 1;
bool STEAL_WM_SELECTION = 0;
std::string LD_PRELOAD_PATH = "/usr/lib/libmpx-patch.so";
std::string MASTER_INFO_PATH = "$HOME/.mpxmanager/master-info.txt";
std::string SHELL = "/bin/sh";
std::string NOTIFY_CMD = "notify-send -h string:x-canonical-private-synchronous:MPX_NOTIFICATIONS -a "
    WINDOW_MANAGER_NAME;
uint32_t AUTO_FOCUS_NEW_WINDOW_TIMEOUT = 5000;
// TODO make configurable per window
uint32_t CLONE_REFRESH_RATE = 15;
uint32_t CRASH_ON_ERRORS = 0;
uint32_t DEFAULT_BORDER_COLOR = 0x0;
uint32_t DEFAULT_BORDER_WIDTH = 1;
uint32_t DEFAULT_MOD_MASK = Mod4Mask;
// TODO start up option
uint32_t DEFAULT_NUMBER_OF_WORKSPACES = 10;
uint32_t DEFAULT_UNFOCUS_BORDER_COLOR = 0xDDDDDD;
uint32_t EVENT_PERIOD = 100;
uint32_t IGNORE_MASK = Mod2Mask;
uint32_t KILL_TIMEOUT = 100;
uint32_t NON_ROOT_DEVICE_EVENT_MASKS = XCB_INPUT_XI_EVENT_MASK_FOCUS_OUT | XCB_INPUT_XI_EVENT_MASK_FOCUS_IN;
uint32_t NON_ROOT_EVENT_MASKS = XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_VISIBILITY_CHANGE;
uint32_t POLL_COUNT = 3;
uint32_t POLL_INTERVAL = 10;
uint32_t ROOT_DEVICE_EVENT_MASKS = XCB_INPUT_XI_EVENT_MASK_HIERARCHY;
uint32_t ROOT_EVENT_MASKS = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
    XCB_EVENT_MASK_STRUCTURE_NOTIFY;
WindowMask MASKS_TO_SYNC = MODAL_MASK | ABOVE_MASK | BELOW_MASK | HIDDEN_MASK | NO_TILE_MASK | STICKY_MASK |
    URGENT_MASK;
uint32_t SRC_INDICATION = 7;

static volatile bool shuttingDown = 0;
void requestShutdown(void) {
    shuttingDown = 1;
}
bool isShuttingDown(void) {
    return shuttingDown;
}
