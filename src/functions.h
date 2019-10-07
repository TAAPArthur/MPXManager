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

#define RAISE_OR_RUN(MATCH) +[]{raiseOrRun(MATCH);}

enum WindowAction {
    ACTION_RAISE, ACTION_FOCUS, ACTION_ACTIVATE, ACTION_LOWER
} ;
/**
 * Attempts to find a that rule matches a managed window.
 * First the active Master's window& stack is checked ignoring the master's window cache.
 * Then all windows are checked
 * and finally the master's window complete window& stack is checked
 * @param rule
 * @return 1 if a matching window was found
 */
WindowInfo* findAndRaise(const BoundFunction& rule, WindowAction action = ACTION_ACTIVATE, bool checkLocalFirst = 1,
                         bool cache = 1, Master* master = getActiveMaster());

static inline WindowInfo* findAndRaiseSimple(const BoundFunction& rule) {return findAndRaise(rule, ACTION_ACTIVATE, 0, 0);}
bool matchesClass(WindowInfo* winInfo, std::string str);
bool matchesTitle(WindowInfo* winInfo, std::string str);
bool raiseOrRun(std::string s, std::string cmd, bool matchOnClass = 0);
static inline bool raiseOrRun(std::string s) {
    return raiseOrRun(s, s, 1);
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
void cycleWindows(int delta, Master* m = getActiveMaster());



//////Run or Raise code

/**
 * Checks to see if any window in searchList matches rule ignoring any in ignoreList
 * @param rule
 * @param searchList
 * @param ignoreList can be NULL
 * @return the first window that matches rule or NULL
 */
WindowInfo* findWindow(const BoundFunction& rule, ArrayList<WindowInfo*>& searchList,
                       ArrayList<WindowID>* ignoreList = NULL);


/**
 * Send the active window to the first workspace with the given name
 * @param winInfo
 * @param name
 */
void sendWindowToWorkspaceByName(WindowInfo* winInfo, std::string name);

/**
 * Swap the active window with the window dir windows away from its current position in the worked space& stack. The stack wraps around if needed
 * @param dir
 */
void swapPosition(int dir, ArrayList<WindowInfo*>& stack = getActiveWindowStack());
/**
 * Shifts the focus up or down the window& stack
 * @param index
 * @return
 */
int shiftFocus(int index, ArrayList<WindowInfo*>& stack = getActiveWindowStack());


/**
 * Shifts the focused window to the top of the window& stack
 */
void shiftTop(ArrayList<WindowInfo*>& stack = getActiveWindowStack());
/**
 * Swaps the focused window with the top of the window& stack
 */
void swapWithTop(ArrayList<WindowInfo*>& stack = getActiveWindowStack());



/**
 * Activates the window at the bottom of the Workspace& stack
 * @return 1 if successful
 */
int focusBottom(const ArrayList<WindowInfo*>& stack = getActiveWindowStack());
/**
 * Activates the window at the top of the workspace& stack
 * @return 1 if successful
 */
int focusTop(const ArrayList<WindowInfo*>& stack = getActiveWindowStack());



/**
 * activates the workspace that contains the mouse pointer
 * @see getLastKnownMasterPosition()
 */
void activateWorkspaceUnderMouse(void);
#endif
