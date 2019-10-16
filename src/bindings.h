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

#define LEN_PASSTHROUGH 4


/**
 * Details which window to act on for Bindings
 */
typedef enum {
    /// Use the focused window for key bindings and the target/event window for mouse bindings
    DEFAULT_WINDOW = 0,
    /// Always use the focused window
    FOCUSED_WINDOW,
    /// Always use the target window (the target of the active master if set of the event window)
    TARGET_WINDOW
} WindowParamType;
struct UserEvent {
    /**
     * Compares Bindings with detail and mods to see if it should be triggered.
     * If bindings->mod ==anyModier, then it will match any modifiers.
     * If bindings->detail or keycode is 0 it will match any detail
     * @param binding
     * @param detail either a keycode or a button
     * @param mods modifiers
     * @param mask
     * @param keyRepeat if the event was a key repeat
     * @return Returns true if the binding should be triggered based on the combination of detail and mods
     */
    uint32_t mod = 0;
    uint32_t detail = 0;
    uint32_t mask = 0;
    bool keyRepeat = 0;
    Master* master = getActiveMaster();
    WindowInfo* winInfo = NULL;
    Master* getMaster() const;
};
std::ostream& operator<<(std::ostream& stream, const UserEvent& userEvent);

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
    // used only for chain bindings
    bool noGrabDevice;
    uint32_t chainMask = 0 ;
};

#define ANY_MASK (-1)
///used for key/mouse bindings
struct Binding {
    /**Modifier to match on*/
    unsigned int mod;
    /**Either Button or KeySym to match**/
    int buttonOrKey;
    /**Function to call on match**/
    const BoundFunction boundFunction;
    BindingFlags flags;
    /// user specified name to be used for debugging
    std::string name;

    Binding(uint32_t mod, uint32_t buttonOrKey, const BoundFunction boundFunction = {}, const BindingFlags& flags = {},
        std::string name = ""): mod(mod), buttonOrKey(buttonOrKey), boundFunction(boundFunction), flags(flags), name(name) {
        if(flags.mask == 0)
            this->flags.mask = buttonOrKey == 0 ? ANY_MASK : isButton(buttonOrKey) ? XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS :
                XCB_INPUT_XI_EVENT_MASK_KEY_PRESS;
    }


    /**Converted detail of key bindings. Should not be set manually*/
    int detail = 0;
    virtual ~Binding() = default;
    bool operator==(const Binding& binding)const;
    friend std::ostream& operator<<(std::ostream& stream, const Binding& binding);
    WindowInfo* getWindowToActOn(const UserEvent& event)const;
    uint32_t getMask()const;
    WindowParamType getWindowTarget()const {return flags.windowTarget;}
    PassThrough getPassThrough()const {return flags.passThrough;}
    bool isNotGrabbable()const {return flags.noGrab;}
    /**
     * @return 1 iff the code should proceed or 0 iff it should return immediately
     */
    virtual bool trigger(const UserEvent& event)const;
    bool matches(const UserEvent& event);
    uint32_t getDetail();
    /// @return the id of the device to grab
    int getTargetID() const {return flags.targetID;}
    int setTargetID(MasterID id) {return flags.targetID = id;}
    /**
     * Wrapper around grabDetail()
     * @param binding
     * @return 0 iff the grab was successful
     */
    int grab();
    /**
     * Wrapper around ungrab()
     * @param binding
     * @return 0 iff the grab was successful
     */
    int ungrab();
    std::string getName()const {return name;}
} ;


/**
 * Returns a Node list of all registered biddings
 * @see deviceBindings;
*/

ArrayList<Binding*>& getDeviceBindings();

/**
 * Check all bindings of a given type to see if they match the params
 * @param mods modifier mask
 * @param detail either a KeyCode or a mouse button
 * @param bindingType
 * @param winInfo the relevant window
 * @param keyRepeat if the event was a key repeat
 * @return 1 if a binding was matched that had passThrough == false
 */
bool checkBindings(const UserEvent& userEvent, const ArrayList<Binding*>& bindings = getDeviceBindings());


UserEvent& getLastUserEvent();
void setLastUserEvent(const UserEvent& event);


#endif
