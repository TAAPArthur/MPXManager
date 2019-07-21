#ifndef MPX_CHAIN_BINDINGS_H_
#define MPX_CHAIN_BINDINGS_H_

#include "bindings.h"
#include "arraylist.h"
struct Chain;
/**
 * @return the last Chain added the active chain stack
 */
const Chain* getActiveChain(Master* m = getActiveMaster());
/**
 * Returns the list of chains the active master is currently triggering
 */
ArrayList<const Chain*>& getActiveChains(Master* m = getActiveMaster());
struct Chain : Binding {

    ArrayList<Binding*>members;
    int chainMask = 0;
    Chain(unsigned int mod, int buttonOrKey, const BoundFunction boundFunction = {}, const ArrayList<Binding*>& members = {},
          const
          BindingFlags& flags = {}, int mask  = 0): Binding(mod, buttonOrKey, boundFunction, flags),
        members(members), chainMask(mask) {
    }
    Chain(unsigned int mod, int buttonOrKey, const ArrayList<Binding*>& members = {}, const BindingFlags& flags = {}, int
          mask  = 0): Binding(mod, buttonOrKey, {}, flags), members(members), chainMask(mask) {
    }
    ~Chain() {
        members.deleteElements();
    }
    /**
     * Starts a chain binding
     * The chain binding indicated by the boundFunction is add the stack
     * of active bindings for the active master.
     * The device is grabbed corresponding to the int arg in the boundFunction
     * All members of the chain are grabbed unless noGrab is true
     * @param boundFunction
     */
    bool start(const UserEvent& event)const;
    /**
     * Ends the active chain for the active master
     * Undoes all of startChain()
     */
    bool end()const;
    virtual bool trigger(const UserEvent& event)const override;

    bool check(const UserEvent& userEvent)const;
    static bool hasChainExtended(Chain* ref) {
        return getActiveChain() != ref && (ref == NULL || getActiveChains().find(ref));
    }
};
bool checkAllChainBindings(const UserEvent& userEvent);
#endif
