/**
 * @file debug.h
 * @brief Validated invariants
 */

#ifndef MPXMANAGER_DEBUG_H_
#define MPXMANAGER_DEBUG_H_
#include "windows.h"
bool validate();
void validateX();
void resetUserMask(WindowInfo* winInfo);
bool isWindowMapped(WindowID win) ;
void addDieOnIntegratyCheckFailRule();
#endif
