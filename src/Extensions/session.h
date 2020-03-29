/**
 * @file
 * @brief Create/destroy XConnection and set global X vars
 */

#ifndef MPXManager_SESSION_H_
#define MPXManager_SESSION_H_

#include <xcb/xcb.h>
#include "../mywm-structs.h"
#include "../bindings.h"


/**
 * Loads global MPX state that has been save via saveCustomState()
 */
void loadCustomState(void);
/**
 * Stores global MPX state so it can restored via loadCustomState()
 * The state is stored as properties of the root window
 */
void saveCustomState(void);
/**
 * Loads saved state on XConnection and saves after a batch of TILE_WORKSPACE
 */
void addResumeCustomStateRules(bool remove = 0);
#endif
