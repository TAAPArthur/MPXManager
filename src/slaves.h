/**
 * @file slaves.h
 */
#ifndef MPX_SLAVES_H
#define MPX_SLAVES_H
#include "mywm-structs.h"
#include "util/arraylist.h"
#include <stdbool.h>

/**
 * @return a list of all slaves
 */
const ArrayList* getAllSlaves();
/**
 * Upon creation/master id update slaves will be automatically associated with
 * their (new) master; When the slave is destructed, the master will lose all refs to the slave
 */
typedef struct Slave {
    const MasterID id;
    ///the id of the associated master device
    MasterID attachment;
    /// Whether this is a keyboard(1) or a pointer device(0)
    const bool keyboard;
    ///the name of the slave device
    char name[MAX_NAME_LEN];
} Slave ;
/**
 * Allocates a new Slave on the heap and registers it
 *
 * @param id
 * @param attachment
 * @param keyboard
 * @param name
 *
 * @return
 */
Slave* newSlave(const MasterID id, MasterID attachment, bool keyboard, const char* name);
/**
 * Unregisters the slaves and removes the struct and internal memory
 * @param slave
 */
void freeSlave(Slave* slave);
/**
 * Checks to see if the device is prefixed with XTEST
 * @return 1 iff str is the name of a test slave as opposed to one backed by a physical device
 */
bool isTestDevice(const char* name);

#endif
