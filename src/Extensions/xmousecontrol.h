/**
 * @file xmousecontrol.h
 * @brief Provides methods related to controlling mouse from the keyboard.
 */

#ifndef XMOUSE_CONTROL_H
#define XMOUSE_CONTROL_H

#include <X11/X.h>

#ifndef XMOUSE_CONTROL_EXT_ENABLED
/// enables this extensions
#define XMOUSE_CONTROL_EXT_ENABLED 1
#endif
#if XMOUSE_CONTROL_EXT_ENABLED

#define SCROLL_UP    4
#define SCROLL_DOWN  5
#define SCROLL_LEFT  6
#define SCROLL_RIGHT 7
/// Masks that dictate how the mouse will be controlled every update
#define    MOVE_UP_MASK (1 << 0)
#define    MOVE_DOWN_MASK (1 << 1)
#define    MOVE_LEFT_MASK (1 << 2)
#define    MOVE_RIGHT_MASK (1 << 3)
#define    SCROLL_UP_MASK (1 << 4)
#define    SCROLL_DOWN_MASK (1 << 5)
#define    SCROLL_LEFT_MASK (1 << 6)
#define    SCROLL_RIGHT_MASK (1 << 7)

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
void addStartXMouseControlRule();

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
void addDefaultXMouseControlBindings(unsigned int mask = Mod3Mask);
void resetXMouseControl();
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
void adjustSpeed(int multiplier);
/**
 * Increases how many times the scroll button is pressed every update
 *
 * @param multiplier
 */
void adjustScrollSpeed(int multiplier);
/**
 * Applies a single update
 */
void xmousecontrolUpdate(void);
#endif
#endif
