/**
 * @file config.h
 * @brief user config file
 */


#ifndef CONFIG_H_
#define CONFIG_H_

#include <string.h>
#include <assert.h>

#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <xcb/xinput.h>

#include "bindings.h"
#include "default-rules.h"
#include "functions.h"
#include "globals.h"
#include "wmfunctions.h"
#include "layouts.h"
#include "xsession.h"
#include "logger.h"

void loadSettings();

void (*runOnStartup)();


#endif /* CONFIG_H_ */
