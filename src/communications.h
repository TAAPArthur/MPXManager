/**
 * @file communications.h
 * Contains methods to used to remote communicate with the WM
 */
#ifndef MPXMANAGER_COMMUTNICATE_H_
#define MPXMANAGER_COMMUTNICATE_H_

#include "bindings.h"
#include "boundfunction.h"
#include <typeinfo>

/**
 *
 *
 * @param str the str to check
 * @param isNumeric if str can be converted to an int
 *
 * @return the interger if isNumeric is set to 1
 */
int isInt(std::string str, bool* isNumeric);


/**
 * Function wrapper for Option
 */
struct GenericOptionFunctionWrapper {
    virtual ~GenericOptionFunctionWrapper() = default;
    /**
     *
     * @return does this function take a string (0) or an int (1) on match
     */
    virtual bool isNumeric() const {return 0;};
    /**
     * @return  1 iff this function takes no args
     */
    virtual bool isVoid()const {return 0;}
    /**
     * Calls the underlying function.
     * p may be converted to an int
     *
     * @param p
     */
    virtual void call(std::string p)const = 0 ;
    /**
     * @return the type name
     */
    virtual std::string getType()const = 0;
    /**
     * prints string representation
     *
     * @param strm
     *
     * @return
     */
    virtual std::ostream& toString(std::ostream& strm) {
        return strm << getType();
    }
};
/**
 * Holds a function that takes an string or an int like parameter
 * If a variable pointer is passed in, it is wrapped in a setter
 *
 * @tparam P
 */
template <typename P>
struct OptionFuntionWrapper : GenericOptionFunctionWrapper {
    /// backing variable
    P* var = NULL;
    /// generic function holder
    std::function<void(P)>func;

    /**
     *
     *
     * @param p var or function pointer
     */
    /// @{
    OptionFuntionWrapper(std::function<void(P)>p): func(p) {}
    OptionFuntionWrapper(P* p): var(p), func([ = ](P v) {*var = v;}) {}
    /// @}
    void call(std::string p)const override {
        _call(p);
    }
    /**
     * @tparam U
     * @param p a string that can be converted to an int
     * @return
     */
    template<typename U = P>
    std::enable_if_t < std::is_convertible<int, U>::value, void > _call(std::string p)const {
        func(isInt(p, NULL));
    }
    /**
     * @tparam U
     * @param p a string to pass to the underlying function
     * @return
     */
    template<typename U = P>
    std::enable_if_t < std::is_same<std::string, U>::value, void > _call(std::string p)const {
        func(p);
    }
    std::ostream& toString(std::ostream& strm) override {
        if(var)
            strm << *var << " ";
        return strm << getType();
    }
    virtual std::string getType()const {
        return typeid(P).name();
    }
    bool isNumeric() const override {
        return !std::is_same<std::string, P>::value;
    }
};
/**
 * Holds a reference to a void function
 */
template<>
struct OptionFuntionWrapper<void> : GenericOptionFunctionWrapper {
    /// generic function holder
    std::function<void(void)>func;
    /**
     * @param func
     */
    OptionFuntionWrapper(std::function<void(void)>func): func(func) {}
    void call(std::string p __attribute__((unused)))const override {
        func();
    }
    virtual std::string getType()const {
        return typeid(func).name();
    }
    bool isVoid() const override {
        return 1;
    }
};
/// flags that control how options are handled
enum OptionFlags {
    /// if true and the command was sent, program will fork before executing this command and attempt redirect output to the caller's stdout
    FORK_ON_RECEIVE = 1,
    /// Send confirmation before command is run
    CONFIRM_EARLY = 4,
};
/// Holds info to map string to a given operation
struct Option {

    /// option name; --$name will trigger option
    std::string name;
    /// detail how we should respond to the option
    const std::shared_ptr<GenericOptionFunctionWrapper> func;
    /// bitmask of OptionFlags
    int flags = 0;
    /**
     * @tparam T
     * @param name
     * @param t
     * @param flags
     */
    /// @{
    template <typename T>
    Option(std::string name, T* t, int flags = 0): name(name), func(new OptionFuntionWrapper<T>(t)), flags(flags) {}
    template <typename T>
    Option(std::string name, void(*t)(T), int flags = 0): name(name), func(new OptionFuntionWrapper<T>(t)), flags(flags) {}
    Option(std::string name, void(*t)(void), int flags = 0): name(name), func(new OptionFuntionWrapper<void>(t)),
        flags(flags) {}
    /// @}

    /// @return the type of backing function
    std::string getType()const {return func->getType();}

    /// @return name
    std::string getName() const {return name;}
    /**
     * @param str
     * @param empty
     * @param isNumeric
     *
     * @return 1 if the name matches str and func matches empty and isNumeric
     */
    bool matches(std::string str, bool empty, bool isNumeric)const ;
    /// @return if func take no arguments
    bool isVoid() const {return func->isVoid();}
    /**
     * If option holds a function, calls the function bound to option with argument value.
     * Else sets name to value
     *
     * @param value the value to set/pass. If NULL, a value of '1' is used
     */
    void call(std::string value)const {(*this)(value);}
    /// @copydoc call
    void operator()(std::string value)const ;
    /// @return name
    operator std::string()const {return name;}
    /**
     *
     * @param option
     *
     * @return 1 iff the name and type match
     */
    bool operator==(const Option& option)const {
        return name == option.name && getType() == option.getType();
    }
};


/**
 * Finds a option by name that can accept value
 *
 * @param name the name of the option
 * @param value the value to be passed
 *
 * @return the first option matching name or NULL
 */
const Option* findOption(std::string name, std::string value) ;

/**
 *
 *
 * @param strm
 * @param option
 *
 * @return
 */
std::ostream& operator<<(std::ostream& strm, const Option& option) ;
/**
 * Sets up the system to receive messages from an external client
 */
void enableInterClientCommunication(void);
/**
 * Should be to a xcb_client_message_event_t event.
 *
 * If the client message was sent to us, in our custom format, we will adjust the given parameters/run methods
 */
void receiveClientMessage(void);
/**
 * Sends a message to the running WM to set name = value or call name(value) depending if name is an variable or a method
 *
 *
 * @param name
 * @param value
 */
void send(std::string name, std::string value);

/**
 *
 *
 * @return a list of options the can be triggered
 */
ArrayList<Option*>& getOptions();

/**
 * @return 1 iff there are send messages whose receipt has not been confirmed
 */
bool hasOutStandingMessages(void);
#endif
