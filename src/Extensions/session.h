/**
 * @file xsession.h
 * @brief Create/destroy XConnection and set global X vars
 */

#ifndef MPXManager_SESSION_H_
#define MPXManager_SESSION_H_
#include "../mywm-structs.h"
#include <xcb/xcb.h>

/**
 * Loads global MPX state that has been save via saveCustomState()
 */
void loadCustomState(void);
/**
 * Stores global MPX state so it can restored via loadCustomState()
 * The state is stored as properties of the root window
 */
void saveCustomState(void);
void addResumeCustomStateRules();
#endif
