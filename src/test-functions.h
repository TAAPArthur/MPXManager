/**
 * @file test-functions.h
 * @brief Provides methods to simulate key/button presses
 *
 * Wrapper around Xtest
 *
 */
#ifndef TEST_FUNCTIONS_H_
#define TEST_FUNCTIONS_H_

#include "mywm-structs.h"

/**
 * \copydoc sendDeviceAction()
 * @param window to send an action to
 */
void sendDeviceActionToWindow(MasterID id, int detail, int type, WindowID window);

/**
 * Wrappers around xcb_test_fake_input
 * Will send detail to the active window
 *
 * @param id the id of the device to send as
 * @param detail the key or button to send
 * @param type XCB_event type to send; valid values are key/button press/release
 * @see xcb_test_fake_input
 */
void sendDeviceAction(MasterID id, int detail, int type);
/**
 * Simulates a button press
 * @param button the button that was pressed
 */
void sendButtonPress(int button);
/**
 * Simulates a button released
 * @param button the button that was released
 */
void sendButtonRelease(int button);
/**
 * Simulates a key press
 * @param keycode keycode of key to simulate
 */
void sendKeyPress(int keycode);
/**
 * Simulates a key release
 * @param keycode keycode of key to simulate
 */
void sendKeyRelease(int keycode);
/**
 * Sends a keypress followed by a key release
 * @param keycode
 */
void typeKey(int keycode);
/**
 * Sends a button press followed by a button release
 * @param button
 */
void clickButton(int button);


/**
 * Moves the mouse specified by id x,y units relative to relativeWindow. If
 * relativeWindow is none the mouse will be moved relative to its current position
 * @param id
 * @param relativeWindow
 * @param x
 * @param y
 */
void movePointer(MasterID id, WindowID relativeWindow, int x, int y);

#endif
