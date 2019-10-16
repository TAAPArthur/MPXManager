/**
 * @file
 */
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

private:
    ArrayList<Binding*>members;
    /**
     * if non 0, the Master device(s) will be grabbed with this mask for the duration of the chain
     * used only for chain bindings;
     */
    uint32_t chainMask = 0 ;
public:
    Chain(unsigned int mod, int buttonOrKey, const BoundFunction boundFunction = {}, const ArrayList<Binding*>& members = {},
        const BindingFlags& flags = {}, uint32_t chainMask = 0, std::string name = ""): Binding(mod, buttonOrKey, boundFunction,
                flags, name),
        members(members), chainMask(chainMask) { }
    Chain(unsigned int mod, int buttonOrKey, const ArrayList<Binding*>& members, const BindingFlags& flags = {}, uint32_t
        chainMask = 0, std::string
        name = ""): Binding(mod, buttonOrKey, {}, flags, name), members(members), chainMask(chainMask) { }
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
void addApplyChainBindingsRule(AddFlag flag = PREPEND_UNIQUE);
void endActiveChain(Master* master = getActiveMaster());
#endif
