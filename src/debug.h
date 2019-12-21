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
#endif
