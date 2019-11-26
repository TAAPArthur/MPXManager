/**
 * @file test-functions.h
 * @brief Provides methods to simulate key/button presses
 *
 * Wrapper around Xtest
 *
 */
#ifndef MPX_TEST_FUNCTIONS_H_
#define MPX_TEST_FUNCTIONS_H_

#include "masters.h"
#include "xsession.h"


/**
 * Wrappers around xcb_test_fake_input
 * Will send detail to the active window
 *
 * @param id the id of the device to send as
 * @param detail the key or button to send
 * @param type XCB_event type to send; valid values are key/button press/release
 * @param win the window to send to
 * @see xcb_test_fake_input
 */
void sendDeviceAction(MasterID id, int detail, int type, WindowID win = root);
/**
 * Simulates a button press
 * @param button the button that was pressed
 * @param id the Master pointer id
 * @param win the window to send the event to
 */
void sendButtonPress(int button, WindowID win = root, MasterID id = getActiveMasterPointerID());
/**
 * Simulates a button released
 * @param button the button that was released
 * @param id the Master pointer id
 * @param win the window to send the event to
 */
void sendButtonRelease(int button, WindowID win = root, MasterID id = getActiveMasterPointerID());
/**
 * Sends a button press followed by a button release
 * @param button
 * @param id the Master pointer id
 * @param win the window to send the event to
 */
void clickButton(int button, WindowID win = root, MasterID id = getActiveMasterPointerID());

/**
 * Simulates a key press
 * @param keycode keycode of key to simulate
 * @param id the Master keyboard id
 * @param win the window to send the event to
 */
void sendKeyPress(int keycode, WindowID win = root, MasterID id = getActiveMasterKeyboardID());
/**
 * Simulates a key release
 * @param keycode keycode of key to simulate
 * @param id the Master keyboard id
 * @param win the window to send the event to
 */
void sendKeyRelease(int keycode, WindowID win = root, MasterID id = getActiveMasterKeyboardID());
/**
 * Sends a keypress followed by a key release
 * @param keycode
 * @param id the Master keyboard id
 * @param win the window to send the event to
 */
void typeKey(int keycode, WindowID win = root, MasterID id = getActiveMasterKeyboardID());


/**
 * Moves the mouse specified by id x,y units relative to relativeWindow. If
 * relativeWindow is none the mouse will be moved relative to its current position
 * @param x
 * @param y
 * @param relativeWindow
 * @param id
 */
void movePointer(short x, short y, WindowID relativeWindow = root, MasterID id = getActiveMasterPointerID());
/**
 * Moves the mouse specified by id x,y units relative to its current position
 * @param x horizontal displacement
 * @param y vertical displacement
 * @param id
 */
void movePointerRelative(short x, short y, MasterID id = getActiveMasterPointerID()) ;

#endif
