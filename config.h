/**
 * @file config.h
 * @breif user config file
 */


#ifndef CONFIG_H_
#define CONFIG_H_

#include <string.h>
#include <assert.h>

#include "bindings.h"
#include "default-rules.h"
#include "functions.h"
#include "wmfunctions.h"
#include "layouts.h"

extern int DEVICE_EVENT_MASKS;
extern int IGNORE_MASK;
extern int NUMBER_OF_WORKSPACES;
extern char* workspaceNames[];

#define ADD_BINDINGS \
        bindings[0]=(Binding*)onKeyPressBindings; \
        bindings[1]=(Binding*)onKeyReleaseBindings; \
        bindings[2]=(Binding*)onMousePressBindings; \
        bindings[3]=(Binding*)onMouseReleaseBindings; \
        bindingLengths[0]=LEN(onKeyPressBindings);\
        bindingLengths[1]=LEN(onKeyReleaseBindings);\
        bindingLengths[2]=LEN(onMousePressBindings);\
        bindingLengths[3]=LEN(onMouseReleaseBindings);



void loadSettings();

void (*runOnStartup)();


#endif /* CONFIG_H_ */
