/**
 * @file functions.h
 *
 * @brief functions that users may add to their config that are not used elsewhere
 */


#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_


#include "bindings.h"
#include "masters.h"
#include "wmfunctions.h"

/// Direction towards the head of the list
#define UP -1
/// Direction away from the head of the list
#define DOWN 1

/// arguments to findAndRaise
enum WindowAction {
    /// @{ performs the specified action
    ACTION_RAISE, ACTION_FOCUS, ACTION_ACTIVATE, ACTION_LOWER
    /// @}
} ;
/**
 * Attempts to find a that rule matches a managed window.
 * First the active Master's window& stack is checked ignoring the master's window cache.
 * Then all windows are checked
 * and finally the master's window complete window& stack is checked
 * @param rule
 * @param action
 * @param checkLocalFirst checks the master window stack before checking all windows
 * @param cache ignores recently chosen windows. The cache is cleared if the focused window doesn't match rule or a window cannot be found otherwise
 * @param master
 * @return 1 if a matching window was found
 */
WindowInfo* findAndRaise(const BoundFunction& rule, WindowAction action = ACTION_ACTIVATE, bool checkLocalFirst = 1,
    bool cache = 1, Master* master = getActiveMaster());

/**
 * Tries to find a window by checking all the managed windows, and if it finds one it activates it
 *
 * @param rule
 *
 * @return the window found or NULL
 */
static inline WindowInfo* findAndRaiseSimple(const BoundFunction& rule) {return findAndRaise(rule, ACTION_ACTIVATE, 0, 0);}

/// Determines what WindowInfo field to match on
enum RaiseOrRunType {
    MATCHES_TITLE,
    MATCHES_CLASS,
    MATCHES_ROLE,
} ;
/**
 * @param winInfo
 * @param str
 *
 * @return 1 iff the window class or instance name is equal to str
 */
bool matchesClass(WindowInfo* winInfo, std::string str);
/**
 * @param winInfo
 * @param str
 *
 * @return if the window title is equal to str
 */
bool matchesTitle(WindowInfo* winInfo, std::string str);
/**
 * Tries to raise a window that matches s. If it cannot find one, spawn cmd
 *
 * @param s
 * @param cmd command to spawn
 * @param matchType whether to use matchesClass or matchesTitle
 * @param silent if 1 redirect stderr and out of child process to /dev/null
 *
 * @return 1 iff a window was found
 */
bool raiseOrRun(std::string s, std::string cmd, RaiseOrRunType matchType = MATCHES_CLASS, bool silent = 1);
/**
 * Tries to raise a window with class (or resource) name equal to s.
 * If not it spawns s
 *
 * @param s the class or instance name to be raised or the program to spawn
 *
 * @return 0 iff the program was spawned
 */
static inline bool raiseOrRun(std::string s) {
    return raiseOrRun(s, s, MATCHES_CLASS);
}
/**
 * Tries to raise a window with class (or resource) name equal to s.
 * If not it spawns s
 *
 * @param s the class or instance name to be raised or the program to spawn
 * @param silent if 1 redirect stderr and out of child process to /dev/null
 *
 * @return 0 iff the program was spawned
 */
static inline bool raiseOrRun(std::string s, bool silent) {
    return raiseOrRun(s, s, MATCHES_CLASS, silent);
}
/**
 * Call to stop cycling windows
 *
 * Unfreezes the master window& stack
 */
void endCycleWindows(Master* m = getActiveMaster());
/**
 * Freezes and cycle through windows in the active master's& stack
 * @param delta
 */
void cycleWindows(int delta);



//////Run or Raise code

/**
 * Checks to see if any window in searchList matches rule ignoring any in ignoreList
 * @param rule
 * @param searchList
 * @param ignoreList can be NULL
 * @param includeNonActivatable
 * @return the first window that matches rule or NULL
 */
WindowInfo* findWindow(const BoundFunction& rule, const ArrayList<WindowInfo*>& searchList,
    ArrayList<WindowID>* ignoreList = NULL, bool includeNonActivatable = 0);


/**
 * Send the active window to the first workspace with the given name
 * @param winInfo
 * @param name
 */
void sendWindowToWorkspaceByName(WindowInfo* winInfo, std::string name);

/**
 * Swap the active window with the window dir windows away from its current position in the worked space& stack. The stack wraps around if needed
 * @param dir
 * @param stack
 */
void swapPosition(int dir, ArrayList<WindowInfo*>& stack = getActiveWindowStack());
/**
 * Shifts the focus up or down the window& stack
 * @param index
 * @param stack
 * @return
 */
int shiftFocus(int index, ArrayList<WindowInfo*>& stack = getActiveWindowStack());


/**
 * Shifts the focused window to the top of the window& stack
 * @param stack
 */
void shiftTop(ArrayList<WindowInfo*>& stack = getActiveWindowStack());
/**
 * Swaps the focused window with the top of the window& stack
 * @param stack
 */
void swapWithTop(ArrayList<WindowInfo*>& stack = getActiveWindowStack());



/**
 * Activates the window at the bottom of the Workspace& stack
 * @param stack
 * @return 1 if successful
 */
int focusBottom(const ArrayList<WindowInfo*>& stack = getActiveWindowStack());
/**
 * Activates the window at the top of the workspace& stack
 * @param stack
 * @return 1 if successful
 */
int focusTop(const ArrayList<WindowInfo*>& stack = getActiveWindowStack());



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
#endif
