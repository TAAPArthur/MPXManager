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
 * Simulates a button press
 * @param button the button that was pressed
 * @param id the Master pointer id
 */
void sendButtonPress(int button, MasterID id = getActiveMasterPointerID());
/**
 * Simulates a button released
 * @param button the button that was released
 * @param id the Master pointer id
 */
void sendButtonRelease(int button, MasterID id = getActiveMasterPointerID());
/**
 * Sends a button press followed by a button release
 * @param button
 * @param id the Master pointer id
 */
void clickButton(int button, MasterID id = getActiveMasterPointerID());

/**
 * Simulates a key press
 * @param keyCode keyCode of key to simulate
 * @param id the Master keyboard id
 */
void sendKeyPress(int keyCode, MasterID id = getActiveMasterKeyboardID());
/**
 * Simulates a key release
 * @param keyCode keyCode of key to simulate
 * @param id the Master keyboard id
 */
void sendKeyRelease(int keyCode, MasterID id = getActiveMasterKeyboardID());
/**
 * Sends a keyPress followed by a key release
 * @param keyCode
 * @param id the Master keyboard id
 */
void typeKey(int keyCode, MasterID id = getActiveMasterKeyboardID());


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
