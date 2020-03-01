/**
 * @file
 */
#ifndef MPX_CHAIN_BINDINGS_H_
#define MPX_CHAIN_BINDINGS_H_

#include "arraylist.h"
#include "bindings.h"

/**
 * Helper class to group the onStart and onEnd boundFunctions
 */
struct BoundFunctionTuple {
    /// function to run on chain start
    BoundFunction onStart = {};
    /// function to run on chain end
    BoundFunction onAbort = {};
    /// function to run on chain end
    BoundFunction onEnd = {};
};

/// Can be added to a chain mask to indicate that this chain should be applied to all masters
#define GLOBAL_CHAIN 1
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
    /// functions to on start, end and abort
    BoundFunctionTuple hooks;
    /**
     * if 0, no device will be grabbed
     * if 1, no device will be grabbed and the chain will be global
     * else, the Master device(s) will be grabbed with this mask for the duration of the chain
     * used only for chain bindings;
     */
    uint32_t chainMask = 0 ;
public:
    /**
     *
     * @param mod
     * @param buttonOrKey
     * @param hooks
     * @param members
     * @param flags
     * @param chainMask
     * @param name
     */
    Chain(unsigned int mod, int buttonOrKey, const BoundFunctionTuple& hooks = {}, const ArrayList<Binding*>& members = {},
        const BindingFlags& flags = {}, uint32_t chainMask = 0, std::string name = ""): Binding(mod, buttonOrKey, {},
                flags, name),
        members(members), hooks(hooks), chainMask(chainMask) { }
    /**
     *
     * @param keyBindings
     * @param hooks
     * @param members
     * @param flags
     * @param chainMask
     * @param name
     */
    Chain(const std::initializer_list<KeyBinding> keyBindings, const BoundFunctionTuple hooks = {}, const
        ArrayList<Binding*>&
        members = {},
        const BindingFlags& flags = {}, uint32_t chainMask = 0, std::string name = ""): Binding(keyBindings, {},
                flags, name),
        members(members), hooks(hooks), chainMask(chainMask) { }
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
     * @return 1 iff this chain will be active for all masters instead of a just the one that triggered it
     */
    bool isGlobalChain() const { return chainMask & 1;}
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
     * @param abort if the chain aborted due to being idle
     * @return hooks.onEnd()
     */
    bool end(bool abort = false)const;
    virtual bool trigger(const UserEvent& event)const override;
    std::string getName()const override {return Binding::getName() + (isGlobalChain() ? "*" : "");}

    /**
     * Calls checkBindings with its members
     *
     * @param userEvent
     *
     * @return 1 if the caller shouldn't abort operation
     */
    bool check(const UserEvent& userEvent)const;

    ///@return true iff the chain is allowed to be aborted when idle
    bool allowAbortionOnIdle() const { return hooks.onAbort;}
};
/**
 * @return the last Chain added the active chain stack
 */
const Chain* getActiveChain(Master* m = getActiveMaster());
/**
 * @param master
 *
 * @return the number of global active chains and chains active for just this master
 */
uint32_t getNumberOfActiveChains(Master* master = getActiveMaster());
/**
 * Iterates over the active chain stack and calls check.
 * This method is short-circuitable
 *
 * @param userEvent
 *
 * @return
 */
bool checkAllChainBindings(const UserEvent& userEvent = getLastUserEvent())__attribute__((warn_unused_result)) ;
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
