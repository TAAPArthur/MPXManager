/**
 * @file debug.h
 * @brief Validation invariants and helper functions
 */
#ifndef MPXMANAGER_DEBUG_H_
#define MPXMANAGER_DEBUG_H_
#include "../mywm-structs.h"
#include "rect.h"
#include <stdbool.h>


void dumpRect(Rect rect);
/**
 * Prints just win if it is registered
 * @param win
 */
void dumpWindow(WindowID win) ;

void dumpWindowInfo(WindowInfo* winInfo);
/**
 * Prints all with that have filterMask (if filterMask is 0, print all windows)
 *
 * @param filterMask
 */
void dumpWindowFilter(WindowMask filterMask);

/**
 * Prints a summary of the state of the WM
 */
void printSummary(void);
/**
 * Prints all set rules
 */
void dumpRules(void);
static inline void dumpWindowSingleWindow() {dumpWindowFilter(0);}

/**
 * Prints all window whose title, class, instance of type equals match
 *
 * @param match
 */
void dumpWindowByClass(const char* match);
/**
 * Dumps info on master. If master == null, info on the active master will be printed
 *
 * @param master
 */
void dumpMaster(Master* master) ;
/**
 * Stringifies type
 *
 * @param type a regular event type
 *
 * @return the string representation of type if known
 */
const char* eventTypeToString(int type);

const char* getMaskAsString(WindowMask mask, char* buff);
#endif
