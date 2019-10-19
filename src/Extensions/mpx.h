/**
 * @file mpx.h
 * Contains methods to enable/setup multiple master devices
 */
#ifndef MPX_H
#define MPX_H

#include "../devices.h"
#include "../mywm-structs.h"

#ifndef MPX_EXT_ENABLED
/// Enables the extension
#define MPX_EXT_ENABLED 1
#endif
/**
 * Adds hierarchy rules that will detect master/slave addition/removal and make the correct pairings
 */
void addAutoMPXRules(void);
/**
 * Attaches all slaves to their proper masters
 */
void restoreMPX(void);
/**
 * Creates master devices according to saved masterInfo.
 * Note that a master device won't be created if one with the same name already exists
 * loadMPXMasterInfo should be called prior.
 */
void startMPX(void);
/**
 * Removes all masters devices except the default
 */
void stopMPX(void);
/**
 * Calls stopMPX() and startMPX()
 */
void restartMPX(void);
/**
 * Saves the current master/slave configurations for later use
 */
int saveMPXMasterInfo(void);
/**
 * loads saved master/slave configuration
 */
int loadMPXMasterInfo(void);
/**
 * Creates a new master and moves all active slaves (slaves that are firing events) to the newly
 * created master
 * This method adds a Idle event that will serve as a callback to stop moving active slaves
 * @return the id of the newly created master
 */
int splitMaster(void);
/**
 * Removes all the event rules created by splitMaster()
 */
void endSplitMaster(void);

/**
 * @param slaveName
 *
 * @return the master or NULL
 */
Master* getMasterForSlave(const char* slaveName);

#endif
