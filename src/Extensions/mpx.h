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
 * loadMasterInfo should be called prior.
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
int saveMasterInfo(void);
/**
 * loads saved master/slave configuration
 */
int loadMasterInfo(void);
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
 * Given a slaveName, scans the config to find which master the slave should be belong to
 *
 * @param slaveName
 *
 * @return the name of the master or NULL
 */
char* getMasterNameForSlave(char* slaveName);

/**
 * @param name
 *
 * @return the first master whose name is name or NULL
 */
Master* getMasterByName(char* name);
/**
 * @copybrief getMasterNameForSlave
 *
 * @param slaveName
 *
 * @return the master or NULL
 */
Master* getMasterForSlave(char* slaveName);

/**
 * Give a slaveDevice, find which master device it belongs to
 *
 * @param slaveDevice
 *
 * @return the master keyboard/pointer or 0 if not found
 */
int getMasterIdForSlave(SlaveDevice* slaveDevice);

/**
 * Looks at the config to determine which master device slaveDevice should be attached to and attaches it
 *
 * @param slaveDevice
 */
void attachSlaveToPropperMaster(SlaveDevice* slaveDevice);
/**
 * Sets the focusColor for master based on the config file
 *
 * @param master
 */
void restoreFocusColor(Master* master);
#endif
