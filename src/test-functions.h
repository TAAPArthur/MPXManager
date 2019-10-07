/**
 * @file test-functions.h
 * @brief Provides methods to simulate key/button presses
 *
 * Wrapper around Xtest
 *
 */
#ifndef TEST_FUNCTIONS_H_
#define TEST_FUNCTIONS_H_

#include "masters.h"
#include "xsession.h"


/**
 * Wrappers around xcb_test_fake_input
 * Will send detail to the active window
 *
 * @param id the id of the device to send as
 * @param detail the key or button to send
 * @param type XCB_event type to send; valid values are key/button press/release
 * @param window the window to send to
 * @see xcb_test_fake_input
 */
void sendDeviceAction(MasterID id, int detail, int type, WindowID window = 0);
/**
 * Simulates a button press
 * @param button the button that was pressed
 */
void sendButtonPress(int button, MasterID id = 0, WindowID win = 0);
/**
 * Simulates a button released
 * @param button the button that was released
 */
void sendButtonRelease(int button, MasterID id = 0, WindowID win = 0);
/**
 * Simulates a key press
 * @param keycode keycode of key to simulate
 */
void sendKeyPress(int keycode, MasterID id = 0, WindowID win = 0);
/**
 * Simulates a key release
 * @param keycode keycode of key to simulate
 */
void sendKeyRelease(int keycode, MasterID id = 0, WindowID win = 0);
/**
 * Sends a keypress followed by a key release
 * @param keycode
 */
void typeKey(int keycode, MasterID id = 0, WindowID win = 0);
/**
 * Sends a button press followed by a button release
 * @param button
 */
void clickButton(int button, MasterID id, WindowID win = 0);
static inline void clickButton(int button) {return clickButton(button, 0);}


/**
 * Moves the mouse specified by id x,y units relative to relativeWindow. If
 * relativeWindow is none the mouse will be moved relative to its current position
 * @param x
 * @param y
 * @param id
 * @param relativeWindow
 */
void movePointer(short x, short y, MasterID = getActiveMasterPointerID(), WindowID relativeWindow = root);

#endif
