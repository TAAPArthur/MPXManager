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

#define _RUN_OR_RAISE(TYPE,STR_TO_MATCH,COMMAND_STR) \
    OR(BIND(findAndRaise,(&((Rule){STR_TO_MATCH,TYPE,NULL}))), BIND(spawn,COMMAND_STR))

#define _RUN_OR_RAISE_HELPER(_1,_2,NAME,...) NAME
#define _RUN_OR_RAISE_IMPLICIT(TYPE,STR_TO_MATCH) _RUN_OR_RAISE(TYPE,STR_TO_MATCH,STR_TO_MATCH)

/**
 * Creates a rule with type TYPE that tries to find window with property STR and if it can't it will run COM
 * @param TYPE rule type
 * @param args if args has two elements then STR is the first ad COM is the 2nd else both are args
 */
#define RUN_OR_RAISE(TYPE,args...) _RUN_OR_RAISE_HELPER(args,_RUN_OR_RAISE,_RUN_OR_RAISE_IMPLICIT)(TYPE,args)

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
WindowInfo* findAndRaise(const BoundFunction& rule, WindowAction action = ACTION_RAISE, bool checkLocalFirst = 1,
                         bool cache = 1, Master* master = getActiveMaster());

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
 * Get the next window in the& stacking order in the given direction
 * @param dir
 * @return
 */
WindowInfo* getNextWindowInStack(int dir, const ArrayList<WindowInfo*>& stack = getActiveWindowStack());
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
bool activateWorkspaceUnderMouse(void);
#endif
