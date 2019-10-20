/**
 * @file
 */
#ifndef MPX_CHAIN_BINDINGS_H_
#define MPX_CHAIN_BINDINGS_H_

#include "bindings.h"
#include "arraylist.h"
/**
 * When triggered, the members are checked to see if they can also be triggered
 * The chain will also be added to the stack of active chains.
 * While a chain is active, a master device can be grabbed (determined by chainMask)
 * and grab is called on all members. These grabs are released when the chain ends
 * If a chain doesn't short ciruit, it ends.
 *
 * @see addApplyChainBindingsRule
 */
struct Chain : Binding {

private:
    /// members of the chain
    ArrayList<Binding*>members;
    /**
     * if non 0, the Master device(s) will be grabbed with this mask for the duration of the chain
     * used only for chain bindings;
     */
    uint32_t chainMask = 0 ;
public:
    /**
     *
     *
     * @param mod
     * @param buttonOrKey
     * @param boundFunction
     * @param members
     * @param flags
     * @param chainMask
     * @param name
     */
    Chain(unsigned int mod, int buttonOrKey, const BoundFunction boundFunction = {}, const ArrayList<Binding*>& members = {},
        const BindingFlags& flags = {}, uint32_t chainMask = 0, std::string name = ""): Binding(mod, buttonOrKey, boundFunction,
                flags, name),
        members(members), chainMask(chainMask) { }
    /**
     *
     *
     * @param mod
     * @param buttonOrKey
     * @param members
     * @param flags
     * @param chainMask
     * @param name
     */
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
     * @param event
     */
    bool start(const UserEvent& event)const;
    /**
     * Ends the active chain for the active master
     * Undoes all of startChain()
     */
    bool end()const;
    virtual bool trigger(const UserEvent& event)const override;

    /**
     * Calls checkBindings with its members
     *
     * @param userEvent
     *
     * @return 1 if the caller shouldn't abort operation
     */
    bool check(const UserEvent& userEvent)const;
};
/**
 * @return the last Chain added the active chain stack
 */
const Chain* getActiveChain(Master* m = getActiveMaster());
/**
 * Returns the list of chains the active master is currently triggering
 */
ArrayList<const Chain*>& getActiveChains(Master* m = getActiveMaster());
/**
 * Iterates over the active chain stack and calls check.
 * This method is short-circuitable
 *
 * @param userEvent
 *
 * @return
 */
bool checkAllChainBindings(const UserEvent& userEvent);
/**
 * Adds a ProcessDevice Rule that will process active chains before normal bindings
 *
 * @param flag
 */
void addApplyChainBindingsRule(AddFlag flag = PREPEND_UNIQUE);
/**
 * Ends the currently active chain for master.
 * This is equivalent to having a chain not shortcutting
 *
 * @param master
 */
void endActiveChain(Master* master = getActiveMaster());
#endif
