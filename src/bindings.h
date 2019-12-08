/**
 * @file bindings.h
 * @brief Provides methods related to key/mouse bindings and window matching rules.
 */
#ifndef BINDINGS_H_
#define BINDINGS_H_


#include "arraylist.h"
#include "masters.h"
#include "user-events.h"
#include "xsession.h"
#include "boundfunction.h"

/**
 * Details which window to act on for Bindings
 */
enum WindowParamType {
    /// Use the focused window for key bindings and the event window for mouse bindings
    DEFAULT_WINDOW = 0,
    /// Always use the focused window
    FOCUSED_WINDOW,
    /**
     * Always use the target window if set
     * If not set then the event window is used
     * @see getTarget(), setTarget()
     *
     */
    TARGET_WINDOW
} ;
/**
 * Compares Bindings with detail and mods to see if it should be triggered.
 * If bindings->mod ==anyModier, then it will match any modifiers.
 * If bindings->detail or keycode is 0 it will match any detail
 * Intended to be extracted form a device event to contain only the needed information to match against a binding
 */
struct UserEvent {
    /// @return Returns true if the binding should be triggered based on the combination of detail and mods
    uint32_t mod = 0;
    /// either a keycode or a button
    uint32_t detail = 0;
    /// the mask to match
    uint32_t mask = 0;
    /// @param keyRepeat if the event was a key repeat
    bool keyRepeat = 0;
    /// the master who triggered this binding; should not be null
    Master* master = getActiveMaster();
    /// the window the binding was triggered on
    WindowInfo* winInfo = NULL;
};
/**
 * Prints userEvent
 *
 * @param stream
 * @param userEvent
 *
 * @return
 */
std::ostream& operator<<(std::ostream& stream, const UserEvent& userEvent);

/**
 * Optional arguments to Bindings
 *
 * These are separated into their own class to make it easier to specify only some
 */
struct BindingFlags {
    /**Whether more bindings should be considered*/
    PassThrough passThrough = ALWAYS_PASSTHROUGH;
    /**Whether a device should be grabbed or not*/
    bool noGrab = 0;
    ///The mask to grab;
    uint32_t mask = 0 ;
    /**The target of the grab;; Default: is XIAllMasterDevices*/
    uint32_t targetID = 1;
    /// Details which window to pass to the boundFunction
    WindowParamType windowTarget = DEFAULT_WINDOW ;
    /// if true the binding won't trigger for key repeats
    bool noKeyRepeat = 0;
    /// Binding will be triggered in the active master is in a compatible mode
    unsigned int mode = 0;
};
/// holds a modifier and button/key
struct KeyBinding {
private:
    /**Modifier to match on*/
    unsigned int mod;
    /**Either Button or KeySym to match**/
    uint32_t buttonOrKey;
    /**Converted detail of key bindings. Should not be set manually*/
    int detail = 0;
public:
    /**
     * @param mod
     * @param buttonOrKey
     */
    KeyBinding(uint32_t mod, uint32_t buttonOrKey): mod(mod), buttonOrKey(buttonOrKey) {}
    /// @return the button or key code used to initiate grabs
    uint32_t getDetail();
    /// @return the button or key sym used to generate the detail
    uint32_t getButtonOrKey() const {return buttonOrKey;}
    /// @return the modifier
    uint32_t getMod() const {return mod;}
    /// @return 1 iff the bindings are
    bool operator==(const KeyBinding& binding)const;
    /**
     * @param stream
     * @param binding
     * @return
     */
    friend std::ostream& operator<<(std::ostream& stream, const KeyBinding& binding);
};

/// Wildcard mask
#define ANY_MASK (-1)
///used for key/mouse bindings
struct Binding {
protected:
    /// list of keybinds that will cause this to be triggered
    ArrayList<KeyBinding> keyBindings;
    /**Function to call on match**/
    const BoundFunction boundFunction;
    /// extra optional flags
    BindingFlags flags;
    /// user specified name to be used for debugging
    std::string name;

private:
    /// init the flags.mask if it was initially 0
    void init() {
        if(flags.mask == 0)
            this->flags.mask = keyBindings[0].getButtonOrKey() == 0 ? ANY_MASK : isButton(keyBindings[0].getButtonOrKey()) ?
                XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS :
                XCB_INPUT_XI_EVENT_MASK_KEY_PRESS;
    }
public:
    /**
     * Note that flags.mask is inferred from buttonOrKey if set to the default value of 0.
     * @param mod a valid X modifier
     * @param buttonOrKey a button detail or key sym
     * @param boundFunction the function to call when this binding is triggered
     * @param flags
     * @param name
     */
    Binding(uint32_t mod, uint32_t buttonOrKey, const BoundFunction boundFunction = {}, const BindingFlags& flags = {},
        std::string name = ""): keyBindings({{mod, buttonOrKey}}), boundFunction(boundFunction), flags(flags), name(name) {
        init();
    }

    /**
     * Note that flags.mask is inferred from buttonOrKey if set to the default value of 0.
     * @param keyBindings a list of keyBindings
     * @param boundFunction the function to call when this binding is triggered
     * @param flags
     * @param name
     */
    Binding(ArrayList<KeyBinding> keyBindings, const BoundFunction boundFunction = {}, const BindingFlags& flags = {},
        std::string name = ""): keyBindings(keyBindings), boundFunction(boundFunction), flags(flags), name(name) {
        init();
    }



    virtual ~Binding() = default;
    /**
     * 2 Bindings (A and B) are equal iff a event that triggers A would also trigger B
     *
     * @param binding
     *
     * @return 1 iff the mode, buttonOrKey, mode and mask
     */
    bool operator==(const Binding& binding)const;
    /**
     * Prints this binding
     *
     * @param stream
     * @param binding
     *
     * @return
     */
    friend std::ostream& operator<<(std::ostream& stream, const Binding& binding);
    /**
     * Returns the window that will be passed on to the BoundFunction
     * This can vary with the flags set and properties of the event.master
     * If event.master has an explicitly set target, it is chosen
     * Else the window is chosen according to flags.windowTarget
     *
     * @see WindowParamType
     *
     * @param event
     *
     * @return the window that will be passed on to the BoundFunction
     */
    WindowInfo* getWindowToActOn(const UserEvent& event)const;
    /// @copydoc BindingFlags::mask
    uint32_t getMask()const {return flags.mask;}
    /// @copydoc BindingFlags::windowTarget
    WindowParamType getWindowTarget()const {return flags.windowTarget;}
    /// @copydoc BindingFlags::passThrough
    PassThrough getPassThrough()const {return flags.passThrough;}
    /// @return if this Binding won't grap a button/key
    bool isNotGrabbable()const {return flags.noGrab;}
    /**
     * @return 1 iff the code should proceed or 0 iff it should return immediately
     */
    virtual bool trigger(const UserEvent& event)const;
    /**
     * Compares the mod, mask, keyrepeat and mode of the event and returns true if they match
     *
     * @param event
     *
     * @return 1 iff this Binding matches event
     */
    bool matches(const UserEvent& event);
    /// @return a list of keyBindings this struct should match with
    ArrayList<KeyBinding>& getKeyBindings() {return keyBindings;}
    /// @return the id of the device to grab
    int getTargetID() const {return flags.targetID;}
    /**
     * Wrapper around grabDetail()
     * @return 0 iff the grab was successful
     */
    int grab(bool ungrab = 0);
    /**
     * Wrapper around ungrab()
     * @return 0 iff the grab was successful
     */
    int ungrab() { return grab(1);}
    /// @return name
    virtual std::string getName()const {return name;}
} ;


/**
 * Returns a Node list of all registered biddings
 * @see deviceBindings;
*/

ArrayList<Binding*>& getDeviceBindings();

/**
 * Check bindings to see if they match the userEvent
 * Bindings will be checked in order a binding that matches may cause all subsequent bindings to be skipped
 * @param userEvent
 * @param bindings list of bindings to check against
 * @return 1 if a binding was matched that had passThrough == false
 */
bool checkBindings(const UserEvent& userEvent, const ArrayList<Binding*>& bindings = getDeviceBindings());


/**
 * @return the last event stored by setLastUserEvent()
 */
UserEvent& getLastUserEvent();
/**
 * Stores an event that can be retrieved by getLastUserEvent
 *
 * @param event
 */
void setLastUserEvent(const UserEvent& event);

/**
 * Binding::getWindowToActOn will return target if passed an event from this master
 *
 * @param master
 * @param target the window future bindings will operate on
 */
void setTarget(Master* master, WindowID target);
/**
 * Sets the target for the active master
 *
 * @param target
 */
static inline void setActiveTarget(WindowID target) {
    setTarget(getActiveMaster(), target);
}
/**
 *
 *
 * @param master
 *
 * @return the set target of 0 if unset
 */
WindowID getTarget(Master* master = getActiveMaster()) ;
/**
 * Clears the target
 *
 * @param master
 */
static inline void clearTarget(Master* master = getActiveMaster()) {
    setTarget(master, 0);
}

#endif
