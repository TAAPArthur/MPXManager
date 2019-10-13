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
static int getDeviceIDByMask(int mask) {
    return isKeyboardMask(mask) ? getActiveMasterKeyboardID() :
           getActiveMasterPointerID();
}
ArrayList<const Chain*>& getActiveChains(Master* master) {
    static Index<ArrayList<const Chain*>> key;
    auto list = get(key, master);
    return *list;
}
const Chain* getActiveChain(Master* master) {
    auto& chain = getActiveChains(master);
    return !chain.empty() ? chain.back() : NULL;
}
void endActiveChain(Master*master){
    if(getActiveChain(master))
        getActiveChain(master)->end();
}

bool Chain::check(const UserEvent& userEvent)const {
    if(!checkBindings(userEvent, members))
        return 0;
    return shouldPassThrough(getPassThrough(), end());
}
bool Chain::start(const UserEvent& event)const {
    LOG(LOG_LEVEL_DEBUG, "starting chain; mask:%d\n", flags.chainMask);
    if(flags.chainMask)
        grabDevice(getDeviceIDByMask(flags.chainMask), flags.chainMask);
    for(Binding* member : members) {
        member->grab();
    }
    getActiveChains().add(this);
    return boundFunction(getWindowToActOn(event)) && check(event);
}
bool Chain::end()const {
    LOG(LOG_LEVEL_DEBUG, "ending chain\n");
    getActiveChains().removeElement(this);
    if(flags.chainMask)
        ungrabDevice(getDeviceIDByMask(flags.chainMask));
    for(Binding* member : members)
        member->ungrab();
    return 1;
}
bool Chain::trigger(const UserEvent& event)const {
    return start(event);
}
bool checkAllChainBindings(const UserEvent& userEvent) {
    auto& chains = getActiveChains(userEvent.getMaster());
    for(int i = chains.size() - 1; i >= 0; i--)
        if(!chains[i]->check(userEvent)) {
            LOG(LOG_LEVEL_DEBUG, "checkAllChainBindings terminated early\n");
            return 0;
        }
    return 1;
}
void addApplyChainBindingsRule(AddFlag flag) {
    getEventRules(ProcessDeviceEvent).add(DEFAULT_EVENT(+[]() {return checkAllChainBindings(getLastUserEvent());}), flag);
}
