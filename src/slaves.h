/**
 * @file slaves.h
 */
#ifndef MPX_SLAVES_H
#define MPX_SLAVES_H
#include "mywm-structs.h"
#include <string>

ArrayList<Slave*>& getAllSlaves();
/**
 * Upon creation/master id update slaves will be automatically associated with
 * their (new) master; When the slave is destructed, the master will lose all refs to the slave
 */
struct Slave : WMStruct {
private:
    ///the id of the associated master device
    MasterID attachment = 0;
    /// Whether this is a keyboard(1) or a pointer device(0)
    bool keyboard;
    ///the name of the slave device
    std::string name;
public:
    Slave(SlaveID id, MasterID masterID, bool keyboard, std::string name = ""): WMStruct(id), keyboard(keyboard),
        name(name) {
        setMasterID(masterID);
    }
    ~Slave();
    bool isKeyboardDevice()const {return keyboard;}
    MasterID getMasterID()const {return attachment;}
    Master* getMaster()const;
    void setMasterID(MasterID id);
    std::string getName(void)const {return name;}
    /**
     * Checks to see if the device is prefixed with XTEST
     * @return 1 iff str is the name of a test slave as opposed to one backed by a physical device
     */
    static bool isTestDevice(std::string name);
};
Slave* getSlaveByName(std::string name) ;
#endif
