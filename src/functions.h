/**
 * @file functions.h
 *
 * @brief functions that users may add to their config that are not used elsewhere
 */


#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_


#include "bindings.h"

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


/**
 * Call to stop cycling windows
 *
 * Unfreezes the master window stack
 */
void endCycleWindows(void);
/**
 * Freezes and cycle through windows in the active master's stack
 * @param delta
 */
void cycleWindows(int delta);



//////Run or Raise code

/**
 * Attempts to find a that rule matches a managed window.
 * First the active Master's window stack is checked ignoring the master's window cache.
 * Then all windows are checked
 * and finally the master's window complete window stack is checked
 * @param rule
 * @return 1 if a matching window was found
 */
int findAndRaise(Rule* rule);
/**
 * Checks all managed windows to see if any match the given rule; the first match is raised
 * @param rule
 * @return
 */
int findAndRaiseLazy(Rule* rule);
/**
 * Checks all managed windows to see if any match the given rule; the first match is lowered
 * @param rule
 * @return
 */
int findAndLowerLazy(Rule* rule);
/**
 * Checks to see if any window in searchList matches rule ignoring any in ignoreList
 * @param rule
 * @param searchList
 * @param ignoreList can be NULL
 * @return the first window that matches rule or NULL
 */
WindowInfo* findWindow(Rule* rule, ArrayList* searchList, ArrayList* ignoreList);



/**
 * Send the active window to the first workspace with the given name
 * @param winInfo
 * @param name
 */
void sendWindowToWorkspaceByName(WindowInfo* winInfo, char* name);

/**
 * Swapped the active window with the dir-th window from its current position
 * @param dir
 */
void swapPosition(int dir);
/**
 * Shifts the focus up or down the window stack
 * @param index
 * @return
 */
int shiftFocus(int index);


/**
 * Shifts the focused window to the top of the window stack
 */
void shiftTop(void);
/**
 * Swaps the focused window with the top of the window stack
 */
void swapWithTop(void);


/**
 * Get the next window in the stacking order in the given direction
 * @param dir
 * @return
 */
WindowInfo* getNextWindowInStack(int dir);

/**
 * Activates the window at the bottom of the Workspace stack
 * @return 1 if successful
 */
int focusBottom(void);
/**
 * Activates the window at the top of the workspace stack
 * @return 1 if successful
 */
int focusTop(void);


/**
 * Will shift the position of the window in response the change in mouse movements
 * @param winInfo
 */
void moveWindowWithMouse(WindowInfo* winInfo);
/**
 * Will change the size of the window in response the change in mouse movements
 * @param winInfo
 */
void resizeWindowWithMouse(WindowInfo* winInfo);

/**
 * Prepares for operations on a fixed window. Multiple operations can be performed on a given window at once.
 *
 * Window geometry won't be updated until all operations finish
 * @param winInfo
 */
void startMouseOperation(WindowInfo* winInfo);
/// @copydoc startMouseOperation
void stopMouseOperation(WindowInfo* winInfo);

/**
 * activates the workspace that contains the mouse pointer
 * @see getLastKnownMasterPosition()
 */
void activateWorkspaceUnderMouse(void);
#endif
