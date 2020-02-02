/**
 * @file debug.h
 * @brief Validation invariants and helper functions
 */

#ifndef MPXMANAGER_DEBUG_H_
#define MPXMANAGER_DEBUG_H_
#include "windows.h"
/**
 * Validates everything is internally consistent.
 *
 * Checks that are all associates are bidirectional.
 * IE if a window is in a Workspace then that Workspace contains that window
 * If asserts are enabled, the program will crash if this method fails
 *
 * @return 1 if everything is valid
 */
bool validate();
/**
 * Validates are state is consistent with with X11
 * IE if a window has a MappedMask, then it is actually Mapped
 * If asserts are enabled, the program will crash if this method fails
 *
 * @return 1 if everything is valid
 */
bool validateX();
/**
 * @param win
 * @return 1 iff the window is currently mapped
 */
bool isWindowMapped(WindowID win);
/**
 * Adds a TRUE_IDLE rule that will crash the program if an invariant check fails
 * @param flag
 */
void addDieOnIntegrityCheckFailRule(AddFlag flag = ADD_UNIQUE);
/**
 * Adds a series of rules to make detecting and debugging problems easier
 *
 * @param flag
 */
void addAllDebugRules(AddFlag flag = ADD_UNIQUE);


/**
 * Prints every window in the active Workspace
 */
void dumpWindowStack() ;
/**
 * Prints just win if it is registered
 * @param win
 */
void dumpSingleWindow(WindowID win) ;

/**
 * Prints a summary of the state of the WM
 */
void printSummary(void);
/**
 * Prints all set rules
 */
void dumpRules(void);
/**
 * Prints all with that have filterMask (if filterMask is 0, print all windows)
 *
 * @param filterMask
 */
void dumpWindow(WindowMask filterMask = 0);

/**
 * Prints all window whose title, class, instance of type equals match
 *
 * @param match
 */
void dumpWindow(std::string match);
/**
 * Dumps info on master. If master == null, info on the active master will be printed
 *
 * @param master
 */
void dumpMaster(Master* master) ;
#endif
