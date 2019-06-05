/**
 * @file xmousecontrol.h
 * @brief Provides methods related to controlling mouse from the keyboard.
 */

#ifndef XMOUSE_CONTROL_H
#define XMOUSE_CONTROL_H

#ifndef XMOUSE_CONTROL_EXT_ENABLED
    /// enables this extensions
    #define XMOUSE_CONTROL_EXT_ENABLED 1
#endif
#if XMOUSE_CONTROL_EXT_ENABLED
/// default modifier for this extension
extern int XMOUSE_CONTROL_DEFAULT_MASK;
enum {
    SCROLL_UP = 4,
    SCROLL_DOWN = 5,
    SCROLL_LEFT = 6,
    SCROLL_RIGHT = 7,
};
/// Masks that dictate how the mouse will be controlled every update
enum {
    MOVE_UP_MASK = 1 << 0,
    MOVE_DOWN_MASK = 1 << 1,
    MOVE_LEFT_MASK = 1 << 2,
    MOVE_RIGHT_MASK = 1 << 3,
    SCROLL_UP_MASK = 1 << 4,
    SCROLL_DOWN_MASK = 1 << 5,
    SCROLL_LEFT_MASK = 1 << 6,
    SCROLL_RIGHT_MASK = 1 << 7,
} XMouseControlMasks;

/**
 * How frequently move/scroll masks are applied
 * XMOUSE_CONTROL_UPDATER_INTERVAL * (BASE_MOUSE_SPEED or BASE_SCROLL_SPEED) will give the mouse displacement/scroll distance
 */
extern unsigned int XMOUSE_CONTROL_UPDATER_INTERVAL;
/**
 * How much the mouse moves every update
 */
extern unsigned int BASE_MOUSE_SPEED;
/**
 * How many times the 'scroll button' is pressed ever update
 */
extern unsigned int BASE_SCROLL_SPEED;

/**
 * Will apply an update every XMOUSE_CONTROL_UPDATER_INTERVAL
 *
 * @param c
 *
 * @return NULL
 */
void* runXMouseControl(void* c __attribute__((unused)));
/**
 * Add rules/bindings to get the XMousecontrol experience
 */
void enableXMouseControl(void);
/**
 * Removes a masks
 *
 * @param mask some combination of XMouseControlMasks
 *
 */
void removeXMouseControlMask(int mask);
/**
 * Adds a mask
 *
 * @param mask some combination of XMouseControlMasks
 */
void addXMouseControlMask(int mask);
/**
 * Increases how much the mouses moves every update
 *
 * @param multiplier
 */
void adjustSpeed(double multiplier);
/**
 * Increases how many times the scroll button is pressed every update
 *
 * @param multiplier
 */
void adjustScrollSpeed(double multiplier);
/**
 * Applies a single update
 */
void xmousecontrolUpdate(void);
#endif
#endif
