/**
 * @file
 * Proves methods to grab devices or buttons/keys
 */
#ifndef MPX_DEVICE_GRAB_H_
#define MPX_DEVICE_GRAB_H_

#include "mywm-structs.h"
#include "masters.h"

/**
 * Establishes a passive grab on the given window for events indicated by mask.
 * Multiple clients can have passive grabs on the same window
 * If the client already has an passive grab, it it replaced
 * @param window
 * @param maskValue
 */
void passiveGrab(WindowID window, uint32_t maskValue);
/**
 * Ends a passive grab on the given window for events
 * @param window
 */
void passiveUngrab(WindowID window);

/**
 *
 * Grabs the keyboard or mouse
 * @param deviceID a (non-special) device to grab
 * @param maskValue mask of the events to grab
 * @return 0 on success
 * @see XIGrabDevice
 */
int grabDevice(MasterID deviceID, uint32_t maskValue);
/**
 * Ungrabs the keyboard or mouse
 * Note that id has to be a real (non-special) device id
 * @param id id of the device to ungrab
 * @return 0 on success
 */
int ungrabDevice(MasterID id);
/**
 * Grabs the specified detail/mod combination
 *
 * @param deviceID the device id to grab (supports special ids)
 * @param detail the key or button value to grab
 * @param mod
 * @param maskValue specifies what type of event we are interested in
 * @return 0 iff the grab succeeded
 */
int grabDetail(MasterID deviceID, uint32_t detail, uint32_t mod, uint32_t maskValue);
/**
 * Ungrabs the specified detail/mod combination
 * @param deviceID the device id to grab (supports special ids)
 * @param detail the key or button value to grab
 * @param mod
 * @param isKeyboard used to tell if a key or button should be grabbed
 * @return 1 iff the grab succeeded
 */
int ungrabDetail(MasterID deviceID, uint32_t detail, uint32_t mod, bool isKeyboard);

/**
 *
 * @param id
 * @return 0 iff the grab succeeded
 * @see grabDevice()
 */
static inline int grabKeyboard(MasterID id = getActiveMasterKeyboardID()) {
    return grabDevice(id, KEYBOARD_MASKS);
}

/**
 *
 * @param id
 * @return 0 iff the grab succeeded
 * @see grabDevice()
 */
static inline int grabPointer(MasterID id = getActiveMasterPointerID()) {
    return grabDevice(id, POINTER_MASKS);
}


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
static inline bool isKeyboardMask(int mask) {
    return mask & (KEYBOARD_MASKS) ? 1 : 0;
}
/**
 * @param id
 *
 * @return 1 iff id is 0 or 1
 */
static inline bool isSpecialID(MasterID id) {
    return id == XIAllMasterDevices || id == XIAllDevices;
}
/**
 * Declare interest in select window events
 * @param window
 * @param mask
 * @return the error code if an error occurred
 */
int registerForWindowEvents(WindowID window, int mask);



/**
 * Request to be notified when info related to the monitor/screen changes
 * This method does nothing if compiled without XRANDR support
 */
void registerForMonitorChange();

/**
 * Undoes registerForMonitorChange
 *
 * @param window
 *
 * @return the error code if an error occurred
 */
int unregisterForWindowEvents(WindowID window) ;
#endif
