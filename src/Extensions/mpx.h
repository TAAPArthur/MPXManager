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
void splitMaster(void);


/**
 * Swap the ids of master devices backed by master1 and master2
 * There is no easy way in X to accomplish this task so every other aspect of the
 * master devices (pointer position, window focus, slaves devices etc) are switched and we update our
 * internal state as if just the ids switched
 * @param master1
 * @param master2
 */
void swapXDevices(Master* master1, Master* master2);


/**
 * Attach all registered slaves of Master m to m
 */
bool attachRegisteredSlavesOfMaster(Master* m);
/**
 * Swap all slaves attached to master1 with master2
 */
void swapSlaves(Master* master1, Master* master2);
/**
 * Move all slaves attached to the active master that are registered to the same master
 * as the active slave to the next master
 */
void cycleSlaves(int dir);
/**
 * Move the active slave to the next master
 */
void cycleActiveSlave(int dir);
/**
 * Create a new master and attach all unregistered slaves of the active master to it
 */
void moveUnregisteredSlavesOfActiveMasterToNew(void);

void attachToLastMaster();

void shiftSlaves(int dir);

#define SPLIT_MASTER_BINDING(M, K) {M,K, {splitMaster}, .flags = {.grabDevice = 1}, .chainMembers = CHAIN_MEM( \
        {WILDCARD_MODIFIER, XK_Escape, .flags={.popChain=1, .shortCircuit=1}}, \
        {WILDCARD_MODIFIER, 0, {attachToLastMaster}} \
        ) \
}
#endif
