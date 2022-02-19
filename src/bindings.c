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
#include "xutil/xsession.h"

static ArrayList globalBindings;
static ArrayList globalMasterChainBindings;
void addBindings(Binding* b, int N) {
    for(int i = 0; i < N; i++)
        addElement(&globalBindings, &b[i]);
}

const ArrayList* getBindings() {
    return &globalBindings;
}
void clearBindings() {
    clearArray(&globalBindings);
}

static inline bool matchesFlags(const BindingFlags* flags, const BindingEvent* event) {
    return ((flags->mask & event->mask) == event->mask) &&
        (!flags->noKeyRepeat || !event->keyRepeat) &&
        (flags->mode == getActiveMode() || (getActiveMode() & flags->mode) == flags->mode);
}
bool matches(const Binding* binding, const BindingEvent* event) {
    if(matchesFlags(&binding->flags, event))
        if((binding->mod == WILDCARD_MODIFIER || binding->mod == (event->mod & ~IGNORE_MASK & ~binding->flags.ignoreMod))
            && (!binding->buttonOrKey || binding->detail == event->detail))
            return 1;
    return 0;
}


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
void callBindingWithWindow(const BindingFunc* bindingFunc, EventWindow windowToPass, const BindingEvent* event) {
    switch(windowToPass) {
        case NO_WINDOW:
            bindingFunc->func(bindingFunc->arg, bindingFunc->arg2);
            break;
        case DEFAULT_WINDOW:
        case FOCUSED_WINDOW:
            if(getFocusedWindow())
                bindingFunc->func(getFocusedWindow(), bindingFunc->arg, bindingFunc->arg2);
            else
                INFO("Not calling matching binding because windowInfo is NULL");
            break;
        case EVENT_WINDOW:
            if(event->winInfo)
                bindingFunc->func(event->winInfo, bindingFunc->arg, bindingFunc->arg2);
            else
                INFO("Not calling matching binding because windowInfo is NULL");
            break;
    }
}
void enterChain(Binding* binding, ArrayList* masterBindings) {
    INFO("Starting chain; Size %d, Global: %d", binding->chainMembers.size, binding->flags.mask & 1);
    grabChain(binding, 0);
    grabAllBindings(binding->chainMembers.bindings, binding->chainMembers.size, 0);
    if(binding->flags.mask & 1)
        push(&globalMasterChainBindings, binding);
    else
        push(masterBindings, binding);
}

void dumpBinding(Binding* b) {
    if(isButton(b->buttonOrKey))
        DEBUG("Mod: %d Button: %d\n", b->mod, b->buttonOrKey);
    else
        DEBUG("Mod: %d Sym : %d Detail: %d Mask: %d\n", b->mod, b->buttonOrKey, b->detail, b->flags.mask);
}
bool checkBindings(const BindingEvent* event) {
    ArrayList* masterBindings = globalMasterChainBindings.size ? &globalMasterChainBindings : &
        getActiveMaster()->bindings;
    int numBindings = masterBindings->size ? ((Binding*)peek(masterBindings))->chainMembers.size :
        globalBindings.size;
    TRACE("checking %d bindings", numBindings);
    //DEBUG("Event: " << userEvent);
    for(int i = 0; i < numBindings; i++) {
        Binding* binding = masterBindings->size ? & ((Binding*)peek(masterBindings))->chainMembers.bindings[i] : getElement(
                &globalBindings, i);
        if(matches(binding, event)) {
            TRACE("Found match");
            LOG_RUN(LOG_LEVEL_TRACE, dumpBinding(binding));
            callBindingWithWindow(&binding->func, binding->flags.windowToPass, event);
            if(binding->flags.popChain) {
                assert(masterBindings->size);
                Binding* parent = pop(masterBindings);
                assert(parent->chainMembers.size);
                grabAllBindings(parent->chainMembers.bindings, parent->chainMembers.size, 1);
                grabChain(parent, 1);
                INFO("Chain ended; Size %d, Global: %d", parent->chainMembers.size, parent->flags.mask & 1);
            }
            if(binding->chainMembers.size) {
                enterChain(binding, masterBindings);
                if(binding->flags.noShortCircuit) {
                    INFO("Entering into chain immediately");
                    return checkBindings(event);
                }
            }
            if(!binding->flags.noShortCircuit) {
                break;
            } else {
                DEBUG("Binding not short circuited");
            }
        }
    }
    return 1;
}

int grabBinding(const Binding* binding, bool ungrab) {
    if(binding->detail && !binding->flags.noGrab) {
        if(!ungrab)
            return grabDetail(binding->flags.targetID, binding->detail, binding->mod, binding->flags.mask,
                    binding->flags.ignoreMod);
        else
            return ungrabDetail(binding->flags.targetID, binding->detail, binding->mod, binding->flags.ignoreMod,
                    getKeyboardMask(binding->flags.mask));
    }
    return 0;
}

int grabGlobalBindings() {
    int errors = 0;
    INFO("Grabing all global bindings");
    FOR_EACH(Binding*, b, &globalBindings) {
        errors += grabBinding(b, 0);
    }
    return errors;
}
int grabAllBindings(const Binding* bindings, int numBindings, bool ungrab) {
    int errors = 0;
    for(int i = 0; i < numBindings; i++)
        errors += grabBinding(bindings + i, ungrab);
    return errors;
}

void initBinding(Binding* binding, void* symbols) {
    bool freeSymbols = 0;
    if(!symbols) {
        symbols = startBatchKeyCodeLookup();
        freeSymbols = 1;
    }
    if(binding->flags.mask == 0)
        binding->flags.mask = binding->buttonOrKey == 0 ? ANY_MASK : isButton(binding->buttonOrKey) ?
            XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS :
            XCB_INPUT_XI_EVENT_MASK_KEY_PRESS;
    if(binding->flags.targetID == 0) {
        binding->flags.targetID = XCB_INPUT_DEVICE_ALL_MASTER;
    }
    if(binding->detail == 0 && binding->buttonOrKey != 0) {

        if(isButton(binding->buttonOrKey))
            binding->detail = binding->buttonOrKey;
        else {
            binding->detail = getBatchedKeyCode(symbols, binding->buttonOrKey);
        }
        assert(binding->detail);
    }

    for(int i = 0; i < binding->chainMembers.size; i++) {
        initBinding(binding->chainMembers.bindings + i, symbols);
    }
    if(freeSymbols) {
        endBatchKeyCodeLookup(symbols);
    }
}

void initGlobalBindings() {
    void* symbols = startBatchKeyCodeLookup();
    FOR_EACH(Binding*, b, &globalBindings) {
        initBinding(b, symbols);
    }
    endBatchKeyCodeLookup(symbols);
}
