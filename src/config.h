/**
 * @file config.h
 * @brief user config file
 */


#ifndef CONFIG_H_
#define CONFIG_H_

#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <xcb/xinput.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <xcb/xinput.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <mpxmanager/logger.h>
#include <mpxmanager/settings.h>
#include <mpxmanager/functions.h>
#include <mpxmanager/mpx.h>
#include <mpxmanager/windows.h>
#include <mpxmanager/bindings.h>
#include <mpxmanager/globals.h>
#include <mpxmanager/args.h>

/**
 * Will be called on startup before the XServer is initilized so the user can set global vars, run
 * arbitary commands and most commonly add device bindings and event rules
 */
void loadSettings(void);

#endif /* CONFIG_H_ */
