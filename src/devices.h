/**
 * @file devices.h
 * @brief Provides XI2 helper methods for pointer/keyboard related methods like grabs and device query
 * These methods generally don't modify our internal representation, but just make X calls.
 * The correct events should be listened for to update state
 */
#ifndef MPX_DEVICES_H_
#define MPX_DEVICES_H_

#include "masters.h"
#include "monitors.h"
#include "mywm-structs.h"
#include "util/arraylist.h"
#include "xutil/xsession.h"


/**
 * Creates a master device with the specified name
 * @param name the user set prefix of the master device pair
 */
void createMasterDevice(const char* name);
/**
 * Attaches the specified slave to the specified master
 * @param slaveID valid slaveID
 * @param masterID valid MasterID
 */
void attachSlaveToMasterDevice(SlaveID slaveID, MasterID masterID);

/**
 * Attaches slave to master
 * If master is NULL, slave becomes floating
 *
 * @param slave
 * @param master
 */
void attachSlaveToMaster(Slave* slave, Master* master);
/**
 * Disassociates a slave from its master
 *
 * @param slaveID
 */
void floatSlave(SlaveID slaveID) ;
/**
 * Sends a XIChangeHierarchy request to remove the specified master.
 * The internal state will not be updated until we receive a Hierarchy change event
 * @param id
 * @param returnPointer
 * @param returnKeyboard
 */
void destroyMasterDevice2(MasterID id, int returnPointer, int returnKeyboard);
static inline void destroyMasterDevice(MasterID id) {destroyMasterDevice2(id, DEFAULT_POINTER, DEFAULT_KEYBOARD);}

void destroyAllNonEmptyMasters(void);
/**
 * Calls destroyMasterDevice for all non default masters
 */
void destroyAllNonDefaultMasters(void);

/**
 * Add all existing masters to the list of master devices
 */
void initCurrentMasters(void);
/**
 * Sets the active master to be the device associated with deviceID
 * @param deviceID either master keyboard or slave keyboard (or master pointer)id
 */
bool setActiveMasterByDeviceID(MasterID deviceID);
/**
 * @param id a MasterID or a SlaveID
 *
 * @return the active master to be the device associated with deviceID
 */
Master* getMasterByDeviceID(MasterID id) ;

/**
 * Gets the pointer position relative to a given window
 * @param id the id of the pointer
 * @param relativeWindow window to report position relative to
 * @param result where the x,y location will be stored
 * @return 1 if result now contains position
 */
bool getMousePosition(MasterID id, int relativeWindow, int16_t result[2]);
/**
 * Wrapper for getMousePosition
 *
 * @param result where the x,y location will be stored
 *
 * @return 1 if result now contains position
 */
static inline bool getActiveMousePosition(int16_t result[2]) {
    return getMousePosition(getActiveMasterPointerID(), root, result);
}


/**
 * Sets the client pointer for the given window to the active master
 * @param window
 * @param id
 */
void setClientPointerForWindow(WindowID window, MasterID id);
/**
 *
 *
 * @param win
 *
 * @return the MasterID (pointer) associated with win
 */
MasterID getClientPointerForWindow(WindowID win) ;
/**
 * Returns the client keyboard for the given window
 * @param win
 * @return
 */
Master* getClientMaster(WindowID win) ;

/**
 * Queries the X Server and returns the focused window.
 * Note this method is different than getFocusedWindow() in that we are not looking our view of focus but the Xserver's
 * @param id a keyboard master device
 * @return the current window focused by the given keyboard device
 */
WindowID getActiveFocusOfMaster(MasterID id);
static inline WindowID getActiveFocus(void) {return getActiveFocusOfMaster(getActiveMasterKeyboardID());};

#endif /* DEVICES_H_ */
