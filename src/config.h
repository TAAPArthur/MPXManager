/**
 * @file config.h
 * @brief user config file
 */


#ifndef CONFIG_H_
#define CONFIG_H_

#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <xcb/xinput.h>

/**
 * Will be called on startup before the XServer is initilized so the user can set global vars, run
 * arbitary commands and most commonly add device bindings and event rules
 */
void loadSettings(void);

#endif /* CONFIG_H_ */
