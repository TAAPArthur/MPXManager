/**
 * @file functions.h
 *
 * @brief functions that users may add to their config that are not used elsewhere
 */


#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_


#include "boundfunction.h"
#include "bindings.h"
#include "masters.h"
#include "monitors.h"
#include "wmfunctions.h"

/// Direction towards the head of the list
#define UP -1
/// Direction away from the head of the list
#define DOWN 1

/// arguments to findAndRaise
typedef enum WindowAction {
    /// @{ performs the specified action
    ACTION_NONE, ACTION_RAISE, ACTION_FOCUS, ACTION_ACTIVATE, ACTION_LOWER
    /// @}
} WindowAction ;

/// Flags to findAndRaise
typedef struct FindAndRaiseArg {
    /// checkLocalFirst checks the master window stack before checking all windows
    bool skipMasterStack;
    /// only consider activatable windows
    bool includeNonActivatable;
} FindAndRaiseArg ;

/// Determines what WindowInfo field to match on
typedef enum RaiseOrRunType {
    MATCHES_CLASS,
    MATCHES_TITLE,
    MATCHES_ROLE,
} RaiseOrRunType ;
typedef struct {
    bool(*func)();
    Arg arg;
} FindWindowArg;
/**
 * Attempts to find a that rule matches a managed window.
 * First the active Master's window& stack is checked ignoring the master's window cache.
 * Then all windows are checked
 * and finally the master's window complete window& stack is checked
 * @param rule
 * @param action
 * @param arg
 * @param master
 * @return 1 if a matching window was found
 */
WindowInfo* findAndRaise(const FindWindowArg* rule, WindowAction action, FindAndRaiseArg arg);


/**
 * @param winInfo
 * @param str
 *
 * @return 1 iff the window class or instance name is equal to str
 */
bool matchesClass(WindowInfo* winInfo, const char* str);
/**
 * @param winInfo
 * @param str
 *
 * @return if the window title is equal to str
 */
bool matchesTitle(WindowInfo* winInfo, const char* str);

/**
 * @param winInfo
 * @param str
 *
 * @return if the window role is equal to str
 */
bool matchesRole(WindowInfo* winInfo, const char* str);
/**
 * Tries to raise a window that matches s. If it cannot find one, spawn cmd
 *
 * @param s
 * @param cmd command to spawn
 * @param matchType whether to use matchesClass or matchesTitle
 * @param silent if 1 redirect stderr and out of child process to /dev/null
 * @param preserveSession if true, spawn won't double fork
 *
 * @return 0 iff a window was found else the pid of the spawned process
 */
int raiseOrRun2(const char* s, const char* cmd, RaiseOrRunType matchType);
/**
 * Tries to raise a window with class (or resource) name equal to s.
 * If not it spawns s
 *
 * @param s the class or instance name to be raised or the program to spawn
 *
 * @return 0 iff the program was spawned
 */
static inline int raiseOrRun(const char* s) { return raiseOrRun2(s, s, MATCHES_CLASS); }
/**
 * Freezes and cycle through windows in the active master's& stack
 * @param delta
 * @param filter only cycle throught windows matching filter
 */
void cycleWindowsMatching(int delta, bool(*filter)(WindowInfo*));
static inline void cycleWindows(int delta) {cycleWindowsMatching(delta, NULL);}


/**
 * Activates a window with URGENT_MASK
 *
 * @param action
 *
 * @return 1 iff there was an urgent window
 */
bool activateNextUrgentWindow(void);
/**
 * Removes HIDDEN_MASK from a recent hidden window
 * @param action
 *
 * @return 1 a window was found and changed
 */
bool popHiddenWindow(void);

//////Run or Raise code

/**
 * Checks to see if any window in searchList matches rule ignoring any in ignoreList
 * @param rule
 * @param searchList
 * @param ignoreList can be NULL
 * @param includeNonActivatable
 * @return the first window that matches rule or NULL
 */
WindowInfo* findWindow(const FindWindowArg* rule, const ArrayList* searchList, bool includeNonActivatable);


/**
 * Send the active window to the first workspace with the given name
 * @param winInfo
 * @param name
 */
void sendWindowToWorkspaceByName(WindowInfo* winInfo, const char* name);

/**
 * Swap the active window with the window dir windows away from its current position in the worked space& stack. The stack wraps around if needed
 * @param dir
 * @param stack
 */
void swapPosition(int dir);

static inline bool matchesFocusedWindowClass(WindowInfo* winInfo) {
    return matchesClass(winInfo, getFocusedWindow()->className);
}
/**
 * Shifts the focus up or down the window& stack
 * @param index
 * @param filter ignore windows not matching filter
 * @param stack
 * @return
 */
void shiftFocus(int dir, bool(*filter)(WindowInfo*));
static inline void shiftFocusToNextClass(int dir) {shiftFocus(dir, matchesFocusedWindowClass);};


/**
 * Shifts the focused window to the top of the window& stack if it isn't already
 * If it is already at the top, then shifts the next window to the top and focuses it
 * @param stack
 */
void shiftTopAndFocus();



/**
 * activates the workspace that contains the mouse pointer
 * @see getLastKnownMasterPosition()
 */
void activateWorkspaceUnderMouse(void);
/**
 * Moves the active pointer so that is is the center of winInfo
 *
 * @param winInfo
 */
void centerMouseInWindow(WindowInfo* winInfo);

/**
 * switches to an adjacent workspace
 *
 * @param dir either UP or DOWN
 */
static inline void swapWorkspace(int dir) {
    switchToWorkspace(getNextIndex(getAllWorkspaces(), getActiveWorkspaceIndex(), dir));
}

/**
 * Swaps monitors with an adjacent workspace
 *
 * @param dir either UP or DOWN
 */
static inline void shiftWorkspace(int dir) {
    swapMonitors(getActiveWorkspaceIndex(), (getNextIndex(getAllWorkspaces(), getActiveWorkspaceIndex(), dir)));
}


/**
 * Starts a window move-resize request
 *
 * The current window position and mouse position is stored. ON subsequent calls
 * to updateWindowMoveResize the window position will be change by the delta in the mouse position.
 *
 * If this method is called twice, the previous stored value is forgotten.
 * This method does not itself affect the window position
 *
 * @param winInfo
 * @param move whether to move (change position) or resize (change width/height)
 * @param allowMoveX allow x position/width to be changed
 * @param allowMoveY allow y position/height to be changed
 * @param btn
 * @param m
 */
void startWindowMoveResize(WindowInfo* winInfo, bool move, int disallowMove);
static inline void startWindowMove(WindowInfo* winInfo) {startWindowMoveResize(winInfo, 1, 0);}
static inline void startWindowResize(WindowInfo* winInfo) {startWindowMoveResize(winInfo, 0, 0);}
/**
 * Update a window-move resize with the new mouse position.
 * If a request has not been started (@see startWindowMoveResize) this method is a no-op
 * The current master position is calculated, and the window is move/resized according to the displacement of the current position and the stored position.
 *
 * If the mouse delta is 0, this is a no-op
 * If the resize would cause dimension to be exactly 0, that dimension would have size 1
 * If the resize make a dimension negative, the window is moved and resized in a standard manner
 * @param m
 */
void updateWindowMoveResize() ;
/**
 * Commits the window-move resize
 * Subsequent calls to updateWindowMoveResize or cancelWindowMoveResize won't have any effect
 *
 * @param m
 */
void commitWindowMoveResize() ;

/**
 * A DEVICE_EVENT function that will triggered commitWindowMoveResize when the client specified button is released
 * @param m
 *
 * @return 1 iff commitWindowMoveResize was called
 */
bool detectWindowMoveResizeButtonRelease();
/**
 * Cancels the window-move resize operation
 *
 * The window is reverted to its original position/size it had when startWindowMoveResize was called
 *
 * Subsequent calls to updateWindowMoveResize or commitWindowMoveResize won't have any effect
 *
 *
 * @param m
 */
void cancelWindowMoveResize();
#endif
