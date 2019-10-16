/**
 * @file bindings.c
 * \copybrief bindings.h
 */

#include <assert.h>
#include <ctype.h>
#include <regex.h>
#include <string.h>

#include <xcb/xinput.h>

#include "bindings.h"
#include "device-grab.h"
#include "globals.h"
#include "logger.h"
#include "masters.h"
#include "windows.h"
#include "xsession.h"

Master* UserEvent::getMaster()const {
    return master ? master : getActiveMaster();
}
static UserEvent lastUserEvent;
UserEvent& getLastUserEvent() {return lastUserEvent;}
void setLastUserEvent(const UserEvent& event) {lastUserEvent = event;}

uint32_t Binding::getMask()const {
    return flags.mask;
}

WindowInfo* Binding::getWindowToActOn(const UserEvent& event)const {
    switch(event.getMaster()->getTargetWindow() ? TARGET_WINDOW : getWindowTarget()) {
        default:
        case DEFAULT_WINDOW:
            return isKeyboardMask(getMask()) ? event.getMaster()->getFocusedWindow() : event.winInfo;
        case FOCUSED_WINDOW:
            return event.getMaster()->getFocusedWindow();
        case TARGET_WINDOW:
            return event.getMaster()->getTargetWindow() ? getWindowInfo(event.getMaster()->getTargetWindow()) : event.winInfo;
    }
}
bool Binding::trigger(const UserEvent& event)const {
    LOG_RUN(LOG_LEVEL_DEBUG, std::cout << "Triggering " << *this << "\n");
    return shouldPassThrough(getPassThrough(), boundFunction(getWindowToActOn(event), event.getMaster()));
}
std::ostream& operator<<(std::ostream& stream, const Binding& binding) {
    return stream << "{ Name:'" << binding.getName() << "' " << binding.mod << " " << binding.detail << " Func:'" <<
        binding.boundFunction << "' " << binding.getMask()
        << "}" ;
}
bool Binding::matches(const UserEvent& event) {
    return
        (mod == WILDCARD_MODIFIER || mod == (event.mod & ~IGNORE_MASK)) &&
        (getDetail() == 0 || getDetail() == event.detail) &&
        (event.mask == 0 || getMask() & event.mask) &&
        (!event.keyRepeat || event.keyRepeat == !flags.noKeyRepeat) &&
        (event.getMaster()->allowsMode(flags.mode)) ;
}
std::ostream& operator<<(std::ostream& stream, const UserEvent& userEvent) {
    stream << "{" << userEvent.mod << " " << userEvent.detail << " " << userEvent.mask << " " << userEvent.keyRepeat;
    stream << "Master:" << (userEvent.getMaster() ? userEvent.getMaster()->getID() : 0);
    stream << "Window:" << (userEvent.winInfo ? userEvent.winInfo->getID() : 0);
    return stream << "}";
}

bool checkBindings(const UserEvent& userEvent, const ArrayList<Binding*>& bindings) {
    LOG(LOG_LEVEL_TRACE, "checking bindings %d\n", bindings.size());
    LOG_RUN(LOG_LEVEL_DEBUG, std::cout << "Event: " << userEvent << "\n");
    for(Binding* binding : bindings)
        if(binding->matches(userEvent) && !binding->trigger(userEvent)) {
            LOG(LOG_LEVEL_DEBUG, "checkBindings terminated early\n");
            return 0;
        }
    return 1;
}

ArrayList<Binding*>& getDeviceBindings() {
    static ArrayList<Binding*> deviceBindings;
    return deviceBindings;
}


int Binding::grab() {
    if(!isNotGrabbable() && getDetail())
        return grabDetail(getTargetID(), getDetail(), mod, getMask());
    return -1;
}
int Binding::ungrab() {
    if(!isNotGrabbable() && getDetail())
        return ungrabDetail(getTargetID(), getDetail(), mod, isKeyboardMask(getMask()));
    return -1;
}

uint32_t Binding::getDetail() {
    if(detail == 0 && buttonOrKey != 0) {
        detail = getButtonDetailOrKeyCode(buttonOrKey);
        assert(detail);
    }
    return detail;
}
bool Binding::operator==(const Binding& binding)const {
    return name == binding.name && mod == binding.mod && buttonOrKey == binding.buttonOrKey &&
        flags.mode == binding.flags.mode;
}
