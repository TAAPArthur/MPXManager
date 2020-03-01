#include <assert.h>
#include "chain.h"
#include "device-grab.h"
#include "ext.h"
#include "logger.h"
#include "masters.h"
#include "masters.h"

/**
 * Returns the id of the device that should be grabbed. If the mask is odd, the all master devices will be grabbed
 *
 * @param mask
 * @return the if of the device that should be grabbed based on the mask
 */
ArrayList<const Chain*>globalChain;
static ArrayList<const Chain*>& getActiveChains(Master* master = getActiveMaster()) {
    static Index<ArrayList<const Chain*>> key;
    auto list = get(key, master);
    return *list;
}
const Chain* getActiveChain(Master* master) {
    auto& chain = getActiveChains(master);
    return !globalChain.empty() ? globalChain.back() : !chain.empty() ? chain.back() : NULL;
}
uint32_t getNumberOfActiveChains(Master* master) {
    return globalChain.size() + getActiveChains(master).size();
}
void endActiveChain(Master* master) {
    INFO("External call to end chain");
    if(getActiveChain(master))
        getActiveChain(master)->end();
}

bool Chain::check(const UserEvent& userEvent)const {
    if(!checkBindings(userEvent, members))
        return 0;
    return shouldPassThrough(getPassThrough(), end());
}
bool Chain::start(const UserEvent& event)const {
    INFO("starting chain; mask: " << chainMask);
    if(chainMask) {
        if(getKeyboardMask(chainMask))
            grabDevice(getActiveMasterKeyboardID(), getKeyboardMask(chainMask));
        if(getPointerMask(chainMask))
            grabDevice(getActiveMasterPointerID(), getPointerMask(chainMask));
    }
    for(Binding* member : members)
        member->grab();
    if(isGlobalChain())
        globalChain.add(this);
    else
        getActiveChains().add(this);
    return hooks.onStart({getWindowToActOn(event)}) && check(event);
}
bool Chain::end(bool abort)const {
    LOG(LOG_LEVEL_INFO, "Ending chain; Global: %d; Abort: %d", isGlobalChain(), abort);
    if(isGlobalChain())
        globalChain.removeElement(this);
    else
        getActiveChains().removeElement(this);
    if(chainMask) {
        if(getKeyboardMask(chainMask))
            ungrabDevice(getActiveMasterKeyboardID());
        if(getPointerMask(chainMask))
            ungrabDevice(getActiveMasterPointerID());
    }
    for(Binding* member : members)
        member->ungrab();

    if(abort)
        hooks.onAbort();

    return hooks.onEnd();
}
bool Chain::trigger(const UserEvent& event)const {
    return start(event);
}
bool checkAllChainBindings(const UserEvent& userEvent) {
    const Chain* chain;
    while(chain = getActiveChain(userEvent.master))
        if(!chain->check(userEvent)) {
            INFO("checkAllChainBindings terminated early");
            return 0;
        }
    return 1;
}
void maybeAbortActiveChains() {
    for(Master* master : getAllMasters()) {
        const Chain* chain = getActiveChain(master);
        if(chain && chain->allowAbortionOnIdle()) {
            INFO("Aborting chain: "<< chain);
            chain->end(1);
        }
    }
}
void addApplyChainBindingsRule(AddFlag flag) {
    getEventRules(DEVICE_EVENT).add(BoundFunction{+[]{return checkAllChainBindings(getLastUserEvent());}, DEFAULT_EVENT_NAME(checkAllChainBindings), PASSTHROUGH_IF_TRUE},
        flag);
    getBatchEventRules(DEVICE_EVENT).add(DEFAULT_EVENT(maybeAbortActiveChains));
}
