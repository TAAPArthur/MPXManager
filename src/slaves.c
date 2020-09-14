#include <string.h>
#include <stdlib.h>
#include "slaves.h"

static ArrayList slaveList;
const ArrayList* getAllSlaves(void) {
    return &slaveList;
}
__DEFINE_GET_X_BY_NAME(Slave)

void setMasterForSlave(Slave* slave, MasterID master);
Slave* newSlave(const MasterID id, MasterID attachment, bool keyboard, const char* name) {
    Slave* slave = malloc(sizeof(Slave));
    Slave temp = {.id = id, .keyboard = keyboard};
    memmove(slave, &temp, sizeof(Slave));
    strncpy(slave->name, name, MAX_NAME_LEN - 1);
    addElement(&slaveList, slave);
    setMasterForSlave(slave, attachment);
    return slave;
}
void freeSlave(Slave* slave) {
    setMasterForSlave(slave, 0);
    removeElement(&slaveList, slave, sizeof(SlaveID));
    free(slave);
}

static int endsWith(const char* str, const char* suffix) {
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if(lensuffix > lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}
bool isTestDevice(const char* name) {
    return endsWith(name, "XTEST pointer") || endsWith(name, "XTEST keyboard");
}
