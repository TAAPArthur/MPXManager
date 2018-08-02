/**
 * @file test-functions.h
 * @brief Provides methods to simulate key/button presses
 * Wrappers around xcb_test_fake_input
 * @see xcb_test_fake_input
 *
 */
void sendDeviceAction(int id,int detail,int type);
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
 * Simulates a key prees
 * @param keycode keycode of key to simulate
 */
void sendKeyPress(int keycode);
/**
 * Simulates a key release
 * @param keycode keycode of key to simulate
 */
void sendKeyRelease(int keycode);
