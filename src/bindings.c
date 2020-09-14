#include <assert.h>
#include <ctype.h>
#include <regex.h>
#include <string.h>

#include <xcb/xinput.h>

#include "bindings.h"
#include "globals.h"
#include "masters.h"
#include "util/logger.h"
#include "windows.h"
#include "xutil/device-grab.h"
#include "xutil/xdebug.h"
#include "xutil/xsession.h"

static ArrayList globalBindings;
ArrayList globalMasterChainBindings;
void addBindings(Binding* b, int N) {
    for(int i = 0; i < N; i++)
        addElement(&globalBindings, &b[i]);
}
void clearBindings() {
    clearArray(&globalBindings);
}

Detail getDetail(Binding* binding) {
    if(binding->detail == 0 && binding->buttonOrKey != 0) {
        binding->detail = getButtonDetailOrKeyCode(binding->buttonOrKey);
        assert(binding->detail);
    }
    return binding->detail;
}
static inline bool matchesFlags(const BindingFlags* flags, const BindingEvent* event) {
    return ((flags->mask & event->mask) == event->mask) &&
        (!flags->noKeyRepeat || !event->keyRepeat) &&
        (flags->mode == ANY_MODE || event->mode == ANY_MODE || event->mode == flags->mode);
}
bool matches(Binding* binding, const BindingEvent* event) {
    if(matchesFlags(&binding->flags, event))
        if(binding->mod == WILDCARD_MODIFIER || binding->mod == (event->mod & ~IGNORE_MASK)
            && (!binding->buttonOrKey || getDetail(binding) == event->detail))
            return 1;
    return 0;
}

/*
WindowInfo* getWindowToActOn(Binding*binding, const BindingEvent* event){
    switch(target ? TARGET_WINDOW : getWindowTarget()) {
        default:
        case DEFAULT_WINDOW:
            return getKeyboardMask(getMask()) ? event.master ? event.master->getFocusedWindow() : NULL : event.winInfo;
        case FOCUSED_WINDOW:
            return event.master ? event.master->getFocusedWindow() : NULL;
        case TARGET_WINDOW:
            return target ? getWindowInfo(*target) : event.winInfo;
    }
}
*/

void grabChain(Binding* binding, bool ungrab) {
    if(ungrab) {
        if(binding->flags.grabDevice)
            if(isButton(binding->buttonOrKey))
                ungrabDevice(getActiveMasterPointerID());
            else
                ungrabDevice(getActiveMasterKeyboardID());
    }
    else {
        if(binding->flags.grabDevice)
            if(isButton(binding->buttonOrKey))
                grabDevice(getActiveMasterPointerID(), POINTER_MASKS);
            else
                grabDevice(getActiveMasterKeyboardID(), KEYBOARD_MASKS);
    }
}

bool checkBindings(const BindingEvent* event) {
    ArrayList* masterBindings = globalMasterChainBindings.size ? &globalMasterChainBindings : &
        (event->master ? event->master :
            getActiveMaster())->bindings;
    int numBindings = masterBindings->size ? ((Binding*)peek(masterBindings))->chainMembers.size :
        globalBindings.size;
    TRACE("checking %d bindings", numBindings);
    //DEBUG("Event: " << userEvent);
    for(int i = 0; i < numBindings; i++) {
        Binding* binding = masterBindings->size ? & ((Binding*)peek(masterBindings))->chainMembers.bindings[i] : getElement(
                &globalBindings, i);
        if(matches(binding, event)) {
            TRACE("Found match");
            LOG_RUN(LOG_LEVEL_TRACE, dumbBinding(binding));
            (void)binding->func.func(binding->arg, binding->arg2);
            if(binding->flags.popChain) {
                INFO("Chain ended");
                pop(masterBindings);
                grabAllBindings(binding->chainMembers.bindings, binding->chainMembers.size, 1);
                grabChain(binding, 1);
            }
            if(binding->chainMembers.size) {
                INFO("Starting chain Global: %d", binding->flags.mask & 1);
                if(binding->flags.mask & 1)
                    push(&globalMasterChainBindings, binding);
                else
                    push(masterBindings, binding);
                grabChain(binding, 0);
                grabAllBindings(binding->chainMembers.bindings, binding->chainMembers.size, 0);
                if(!binding->flags.shortCircuit) {
                    INFO("Entering into chain immediately");
                    return checkBindings(event);
                }
            }
            if(binding->flags.shortCircuit) {
                INFO("Chain shot circuited");
                break;
            }
        }
    }
    return 1;
}

int grabBinding(Binding* binding, bool ungrab) {
    if(binding->flags.mask == 0)
        binding->flags.mask = binding->buttonOrKey == 0 ? ANY_MASK : isButton(binding->buttonOrKey) ?
            XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS :
            XCB_INPUT_XI_EVENT_MASK_KEY_PRESS;
    if(binding->flags.targetID == 0) {
        binding->flags.targetID = XIAllMasterDevices;
    }
    if(getDetail(binding) && !binding->flags.noGrab) {
        if(!ungrab)
            return grabDetail(binding->flags.targetID, getDetail(binding), binding->mod, binding->flags.mask);
        else
            return ungrabDetail(binding->flags.targetID, getDetail(binding), binding->mod, binding->flags.mask);
    }
    return 0;
}

int grabAllBindings(Binding* bindings, int numBindings, bool ungrab) {
    int errors = 0;
    if(!bindings) {
        FOR_EACH(Binding*, b, &globalBindings) {
            errors += grabBinding(b, ungrab);
        }
    }
    else {
        for(int i = 0; i < numBindings; i++)
            errors += grabBinding(bindings + i, ungrab);
    }
    return errors;
}
