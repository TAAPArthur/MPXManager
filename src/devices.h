/**
 * @file devices.h
 * @brief Provides XI2 helper methods for pointer/keyboard related methods like grabs and device query
 *
 */
#ifndef MPX_DEVICES_H_
#define MPX_DEVICES_H_

#include "mywm-structs.h"
#include "arraylist.h"
#include <string>
#include "monitors.h"
#include "masters.h"


/**
 * Creates a master device with the specified name
 * @param name the user set prefix of the master device pair
 */
void createMasterDevice(std::string name);
/**
 * Attaches the specified slave to the specified master
 * @param slaveId
 * @param masterId
 */
void attachSlaveToMaster(SlaveID slaveId, MasterID masterId);
void floatSlave(SlaveID slaveId) ;
void attachSlaveToMaster(Slave* slave, Master* master);
/**
 * Sends a XIChangeHierarchy request to remove the specified master.
 * The internal state will not be updated until we receive a Hierarchy change event
 * @param id
 * @param returnPointer
 * @param returnKeyboard
 */
void destroyMasterDevice(MasterID id, int returnPointer, int returnKeyboard);
/**
 * Calls destroyMasterDevice for all non default masters
 */
void destroyAllNonDefaultMasters(void);


/**
 * Add all existing masters to the list of master devices
 */
void initCurrentMasters(void);
/**
 * Sets the active master to be the device associated with deviceId
 * @param deviceId either master keyboard or slave keyboard (or master pointer)id
 * @return 1 iff keyboardSlaveId was a master keyboard device
 */
void setActiveMasterByDeviceId(MasterID deviceId);



/**
 * Gets the pointer position relative to a given window
 * @param id the id of the pointer
 * @param relativeWindow window to report position relative to
 * @param result where the x,y location will be stored
 * @return 1 if result now contains position
 */
bool getMousePosition(MasterID id, int relativeWindow, int16_t result[2]);
static inline bool getMousePosition(Master* m, int relativeWindow, int16_t result[2]) {
    return getMousePosition(m->getPointerID(), relativeWindow, result);
}

/**
 * Swap the ids of master devices backed by master1 and master2
 * There is no easy way in X to accomplish this task so every other aspect of the
 * master devices (pointer position, window focus, slaves devices etc) are switched and we update our
 * internal state as if just the ids switched
 * @param master1
 * @param master2
 */
void swapDeviceId(Master* master1, Master* master2);

/**
 * Sets the client pointer for the given window to the active master
 * @param window
 */
void setClientPointerForWindow(WindowID window, MasterID id = getActiveMasterPointerID());
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
WindowID getActiveFocus(MasterID id = getActiveMasterKeyboardID());

/**
 * Creates a new X11 monitor with the given bounds.
 *
 * This method was designed to emulate a picture-in-picture(pip) experience, but can be used to create any arbitrary monitor
 *
 * @param bounds the position of the new monitor
 */
void createFakeMonitor(Rect bounds);
/**
 * Clears all fake monitors
 */
void clearFakeMonitors();

#endif /* DEVICES_H_ */
