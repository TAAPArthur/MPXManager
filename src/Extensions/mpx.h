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
 * Creates a new master and marks it
 */
void startSplitMaster(void);
/**
 * Attaches the slave that triggered the current device event to the marked master if it is not null
 */
void attachActiveSlaveToMarkedMaster() ;
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

/**
 * Swap the ids of master devices backed by master1 and master2
 * There is no easy way in X to accomplish this task so every other aspect of the
 * master devices (pointer position, window focus, slaves devices etc) are switched and we update our
 * internal state as if just the ids switched
 * @param master1
 * @param master2
 */
void swapDeviceID(Master* master1, Master* master2);
/**
 * Swaps slaves with the master dir indexes away from the active master
 *
 * @param master
 * @param dir
 */
void swapSlaves(Master* master, int dir) ;
/**
 * Attaches the slave that triggered the current device event to the master dir away from the active master
 *
 * @param dir
 */
void cycleSlave(int dir) ;
#endif
