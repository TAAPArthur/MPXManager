/**
 * @file xsession.h
 * @brief Create/destroy XConnection and set global X vars
 */

#ifndef MPXManager_SESSION_H_
#define MPXManager_SESSION_H_
#include "mywm-structs.h"
#include <xcb/xcb.h>
/**
 * Sync EWMH global properties with our internal state
 */
void syncState();

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
 * Sets the WM_STATE from the window masks
 */
void setXWindowStateFromMask(WindowInfo* winInfo);
/**
 * Reads the WM_STATE fields from the given window and sets the window mask to be consistent with the state
 * If the WM_STATE cannot be read, then nothing is done
 * @see setWindowStateFromAtomInfo
 */
void loadSavedAtomState(WindowInfo* winInfo);

/**
 * Sets the window state based on a list of atoms
 * @param winInfo the window whose mask will be modified
 * @param atoms the list of atoms to add,remove or toggle depending on ACTION
 * @param numberOfAtoms the number of atoms
 * @param action whether to add,remove or toggle atoms. For toggle, if all of the corresponding masks for the list of atoms is set, then they all will be removed else they all will be added
 * @see XCB_EWMH_WM_STATE_ADD, XCB_EWMH_WM_STATE_REMOVE, XCB_EWMH_WM_STATE_TOGGLE
 */
void setWindowStateFromAtomInfo(WindowInfo* winInfo, const xcb_atom_t* atoms, int numberOfAtoms, int action);
/**
 * Set environment vars such to help old clients know which master device to use
 */
void setClientMasterEnvVar(void);
void addResumeRules();
#endif
