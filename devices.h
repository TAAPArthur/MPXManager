
#ifndef DEVICES_H_
#define DEVICES_H_
#include "mywm-structs.h"


void createMasterDevice(char*name);
void destroyMasterDevice(int id,int returnPointer,int returnKeyboard);

/**
 * Returns a list containing the slaves of the give master device
 * @param masterDevice
 * @param keyboard
 * @param mouse
 * @return
 */
Node* getSlavesOfMaster(Master* masterDevice,char keyboard,char mouse);



/**
 * Grab all specified keys/buttons and listen for root window events
 */
void registerForEvents();
/**
 * Add all existing masters to the list of master devices
 */
void initCurrentMasters();
/**
 * if deviceId is a slave, will return the master device;
 * else,  deviceId is a master and the parnter master id is returned
 * @param context
 * @param deviceId
 * @return
 */
int getAssociatedMasterDevice(int deviceId);

int getClientKeyboard(int win);
/**
 * Wrapper around XIUngrabDevice
 * Ungrabs the keyboard or mouse
 *
 * @param id    id of the device to ungrab
 */
int ungrabDevice(int id);
/**
 * Wrapper around XIGrabDevice
 *
 * Grabs the keyboard or mouse
 * @param id    if of the device to grab
 * @param maskValue mask of the events to grab
 * @see XIGrabDevice
 */
int grabDevice(int deviceID,int maskValue);

int grabActiveDetail(int detail,int mod,int mouse);
int grabDetail(int deviceID,int detail,int mod,int maskValue,int mouse);
int ungrabDetail(int deviceID,int detail,int mod,int mouse);
int grabDevice(int deviceID,int maskValue);
int grabActiveKeyboard();
int grabActivePointer();
int ungrabDevice(int id);
int grabDetails(Binding*binding,int numKeys,int mask,int mouse);


/**
 * Sets the active master to be the device associated with  mouseSlaveId
 * @param mouseSlaveId either master pointer or slave pointerid
 * @return 1 iff mouseSlaveId was a master pointer device
 */
int setActiveMasterByMouseId(int mouseSlaveId);
/**
 * Sets the active master to be the device associated with keyboardSlaveId
 * @param keyboardSlaveId either master keyboard or slave keyboard (or master pointer)id
 * @return 1 iff keyboardSlaveId was a master keyboard device
 */
int setActiveMasterByKeyboardId(int keyboardSlaveId);

/**
 * Grab all the buttons modifer combo specified by binding
 * @param binding the list of binding
 * @param numKeys   the number of bindings
 * @param mask  XI_BUTTON_PRESS or RELEASE
 */
void grabButtons(Binding*binding,int num,int mask);
/**
 * Grab all the keys/modifer combo specified by binding
 * @param binding the list of binding
 * @param numKeys   the number of bindings
 * @param mask  XI_KEY_PRESS or RELEASE
 */
void grabKeys(Binding*Binding,int numKeys,int mask);

void pushBinding(Binding*chain);
void popActiveBinding();
Node* getActiveBindingNode();
Binding* getActiveBinding();


#endif /* DEVICES_H_ */
