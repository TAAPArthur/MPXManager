/**
 * @file devices.h
 * @brief Provides XI2 helper methods for pointer/keyboard related methods like grabs and device query
 *
 */
#ifndef DEVICES_H_
#define DEVICES_H_

#include "mywm-structs.h"

/**
 * The value pointed to by the Node list returned by getSlavesOfMasterByID()
 */
typedef struct {
    ///The id of the slave
    MasterID id;
    ///the id of the associated master device
    int attachment;
    ///the name of the slave device
    char name[NAME_BUFFER];
    /// Whether this is a keyboard(1) or a pointer device(0)
    char keyboard;
    ///Convience field: the index of attachment in the list passed to getSlavesOfMasterByID()
    unsigned char offset;
} SlaveDevice;

/**
 * Attaches the specifed slave to the specified master
 * @param slaveId
 * @param masterId
 */
void attachSlaveToMaster(int slaveId, int masterId);
/**
 * Creates a master device with the specified name
 * @param name the user set prefix of the master device pair
 */
void createMasterDevice(char* name);
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
 * if deviceId is a slave, will return the master device;
 * else,  deviceId is a master and the partner master id is returned
 * @param deviceId
 * @return
 */
int getAssociatedMasterDevice(int deviceId);


/**
 * Sets the active master to be the device associated with deviceId
 * @param deviceId either master keyboard or slave keyboard (or master pointer)id
 * @return 1 iff keyboardSlaveId was a master keyboard device
 */
void setActiveMasterByDeviceId(MasterID deviceId);

/**
 * Sets the client pointer for the given window to the active master
 * @param window
 */
void setClientPointerForWindow(WindowID window);
/**
 * Returns the client keyboard for the given window
 * @param win
 * @return
 */
int getClientKeyboard(WindowID win);



/**
 * Returns a list containing the slaves of the give master device
 * @param ids an array of ids of the master devices whose slaves we want
 * @param num the size of ids
 * @param numberOfSlaves if not NULL, it will be set to the number of slaves found
 * @return a Node list container where each value is a SlaveDeivce
 */
ArrayList* getSlavesOfMasterByID(MasterID* ids, int num, int* numberOfSlaves);
/**
 * Returns a node list of both types of SlaveDevices for the given master
 * @param master
 */
ArrayList* getSlavesOfMaster(Master* master);

/**
 * Checks to see if the device is prefixed with XTEST
 * @param str
 * @return 1 iff str is the name of a test slave as opposed to one backed by a physical device
 */
int isTestDevice(char* str);

/**
 * Gets the pointer position relative to a given window
 * @param id the id of the pointer
 * @param relativeWindow window to report position relative to
 * @param result where the x,y location will be stored
 * @return 1 if result now contains position
 */
int getMousePosition(MasterID id, int relativeWindow, int result[2]);

/**
 * Swap the ids of master devices backed by master1 and master2
 * There is no easy way in X to accomplish this task so every other aspect of the
 * master devices (ie pointer position, focus, slaves) are switched and we update our
 * internal state as if just the ids switched
 * @param master1
 * @param master2
 */
void swapDeviceId(Master* master1, Master* master2);

/**
 * Add all existing masters to the list of master devices
 */
void initCurrentMasters(void);


/**
 * Returns the id of the device that should be grabbed. If the mask is odd, the all master devices will be grabbed
 *
 * @param mask
 * @return the if of the device that should be grabbed based on the mask
 */
int getDeviceIDByMask(int mask);
/**
 * Wrapper around XIUngrabDevice
 * Ungrabs the keyboard or mouse
 *
 * @param id    id of the device to ungrab
 */
int ungrabDevice(MasterID id);
/**
 * Wrapper around XIGrabDevice
 *
 * Grabs the keyboard or mouse
 * @param deviceID    the device to grab
 * @param maskValue mask of the events to grab
 * @see XIGrabDevice
 */
int grabDevice(int deviceID, int maskValue);
/**
 * Grabs the specified detail/mod combination
 *
 * @param deviceID the device id to grab (supports special ids)
 * @param detail the key or button value to grab
 * @param mod
 * @param maskValue specifies what type of event we are interested in
 * @return 1 iff the grab succeeded
 */
int grabDetail(int deviceID, int detail, int mod, int maskValue);
/**
 * Ungrabs the specified detail/mod combination
 * @param deviceID the device id to grab (supports special ids)
 * @param detail the key or button value to grab
 * @param mod
 * @param isKeyboard used to tell if a key or button should be grabbed
 * @return 1 iff the grab succeeded
 */
int ungrabDetail(int deviceID, int detail, int mod, int isKeyboard);
/**
 * Grabs the active keyboard
 * @return 1 iff the grab succeeded
 */
int grabActiveKeyboard(void);
/**
 *
 * @param id
 * @return 1 iff the grab succeded
 */
int grabKeyboard(MasterID id);


/**
 *
 * @param id
 * @return 1 iff the grab succeded
 * @see grabActivePointer()
 */
int grabPointer(MasterID id);
/**
 * Grabs the active pointer
 * @return 1 iff the grab succeeded
 */
int grabActivePointer(void);

/**
 * Establishes a passive grab on the given window for events indicated by mask.
 * Multiple clients can have passive grabs on the same window
 * @param window
 * @param maskValue
 */
void passiveGrab(WindowID window, int maskValue);
/**
 * Ends a passive grab on the given window for events indicated by mask.
 * @param window
 */
void passiveUngrab(WindowID window);
/**
 * grabs all master devices
 */
int grabAllMasterDevices(void);
/**
 * ungrabs all master devices (keyboards and mice)
 */
int ungrabAllMasterDevices(void);

/**
 * If the masks should go with a keyboard device
 * @param mask
 * @return 1 iff the mask contains XCB_INPUT_XI_EVENT_MASK_KEY_PRESS or XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE
 */
int isKeyboardMask(int mask);

/**
 * Sets the current master position to x,y
 * the previous current position is saved and the delta can be got with getMouseDelta
 * @param x
 * @param y
 */
void setLastKnowMasterPosition(int x, int y);

/**
 * Gets the current master position to x,y
 * the previous current position is saved and the delta can be got with getMouseDelta
 * @return an array with the x and y position
 */
const short* getLastKnownMasterPosition(void);

/**
 * For a given mouse, store the change in position between the last two calls of setLastKnowMasterPosition() in result
 * @param master
 * @param result holds the {dx,dy}
 */
void getMouseDelta(Master* master, short result[2]);

/**
 * Queries the X Server and returns the focused window.
 * Note this method is different thatn getFocusedWindow() in that we are not looking our view of focus but the Xservers
 * @param id a keyboard master device
 * @return the the current window focused by the given keyboard device
 */
WindowID getActiveFocus(MasterID id);

#endif /* DEVICES_H_ */
