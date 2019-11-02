/**
 * @file slaves.h
 */
#ifndef MPX_SLAVES_H
#define MPX_SLAVES_H
#include "mywm-structs.h"
#include <string>

/**
 * @return a list of all slaves
 */
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
    /**
     * @param id unique id
     * @param masterID id of associated master or 0 for a floating slave
     * @param keyboard if the slave is a keyboard device
     * @param name the name of the device
     */
    Slave(SlaveID id, MasterID masterID, bool keyboard, std::string name = ""): WMStruct(id), keyboard(keyboard),
        name(name) {
        setMasterID(masterID);
    }
    /**
     * Disassociates itself from its master;
     */
    ~Slave();
    /**
     * @return if this is a keyboard device
     */
    bool isKeyboardDevice()const {return keyboard;}
    /**
     * @return the id of the associated master or 0;
     */
    MasterID getMasterID()const {return attachment;}
    /**
     * @return the master associated with this slave or NULL
     */
    Master* getMaster()const;
    /**
     * If id, then this slave is assosiated with the master with this id.
     * Else this slave is disassociaated with its current master.
     * @param id of the new master or 0
     */
    void setMasterID(MasterID id);
    /**
      @return the name of the slave
     */
    std::string getName(void)const {return name;}
    /**
     * Checks to see if the device is prefixed with XTEST
     * @return 1 iff str is the name of a test slave as opposed to one backed by a physical device
     */
    static bool isTestDevice(std::string name);
};
#endif
