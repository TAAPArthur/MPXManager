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
int getDeviceIDByMask(int mask);
int getDeviceIDByMask(int mask) {
    int global = mask & 1;
    return global ? XIAllMasterDevices :
           isKeyboardMask(mask) ? getActiveMasterKeyboardID() :
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

bool Chain::check(const UserEvent& userEvent)const {
    if(!checkBindings(userEvent, members))
        return 0;
    return shouldPassThrough(getPassThrough(), end());
}
bool Chain::start(const UserEvent& event)const {
    LOG(LOG_LEVEL_DEBUG, "starting chain; mask:%d\n", chainMask);
    if(chainMask)
        grabDevice(getDeviceIDByMask(chainMask), chainMask);
    for(Binding* member : members) {
        member->grab();
    }
    getActiveChains().add(this);
    return boundFunction(getWindowToActOn(event)) && check(event);
}
bool Chain::end()const {
    getActiveChains().removeElement(this);
    if(chainMask)
        ungrabDevice(getDeviceIDByMask(chainMask));
    for(Binding* member : members)
        member->ungrab();
    LOG(LOG_LEVEL_DEBUG, "ending chain\n");
    return 1;
}
bool Chain::trigger(const UserEvent& event)const {
    return start(event);
}
bool checkAllChainBindings(const UserEvent& userEvent) {
    auto& chains = getActiveChains(userEvent.getMaster());
    for(int i = chains.size() - 1; i >= 0; i--)
        if(!chains[i]->check(userEvent))
            return 0;
    return 1;
}
