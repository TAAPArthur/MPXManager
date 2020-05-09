/**
 * @file communications.h
 * Contains methods to used to remote communicate with the WM
 */
#ifndef MPXMANAGER_COMMUTNICATE_H_
#define MPXMANAGER_COMMUTNICATE_H_

#include "boundfunction.h"

/**
 *
 * @param str the str to check
 * @param isNumeric if str can be converted to an int
 *
 * @return the interger if isNumeric is set to 1
 */
int isInt(std::string str, bool* isNumeric);

/// flags that control how options are handled
enum OptionFlags {
    /// if true and the command was sent, program will fork before executing this command and attempt redirect output to the caller's stdout
    FORK_ON_RECEIVE = 1 << 0,
    /// Send confirmation before command is run
    CONFIRM_EARLY = 1 << 2,
    /// marks this function as one that only modifies a global variable
    VAR_SETTER = 1 << 4,
};
/**
 * Holds a function that takes an string or an int like parameter
 * If a variable pointer is passed in, it is wrapped in a setter
 */
struct Option {

    /// option name
    std::string name;
    /// the backing function
    const BoundFunction boundFunction;
    /// bitmask of OptionFlags
    int flags = 0;
    /**
     * @param n
     * @param boundFunction
     * @param flags
     */
    Option(std::string n, BoundFunction boundFunction, int flags = 0);
    /**
     * If option holds a function, calls the function bound to option with argument value.
     * Else sets name to value
     *
     * @param value the value to set/pass. If NULL, a value of '1' is used
     */
    int call(std::string value)const;
    /**
     * @return the boundFunction
     */
    const BoundFunction& getBoundFunction() const { return boundFunction;}

    /// @return name
    std::string getName() const {return name;}
    /// @return flags
    uint8_t getFlags() const {return flags;}
    /**
     * @param str
     * @param empty
     * @param isNumeric
     *
     * @return 1 if the name matches str and func matches empty and isNumeric
     */
    bool matches(std::string str, bool empty, bool isNumeric)const ;
    /**
     * @param value
     *
     * @return 1 if this option accepts value
     */
    bool matches(std::string value) const {
        bool isNumeric;
        isInt(value, &isNumeric);
        return matches(getName(), value.empty(), isNumeric);
    }
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
        return name == option.name && boundFunction.getType() == option.boundFunction.getType();
    }
};

/// Adds a startup mode
void addStartupMode(std::string name, void(*func)());

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
void addInterClientCommunicationRule(bool remove = 0);
/**
 * Should be to a xcb_client_message_event_t event.
 *
 * If the client message was sent to us, in our custom format, we will adjust the given parameters/run methods
 */
void receiveClientMessage(void);
/**
 * Sends a message to the running WM to set name = value or call name(value) depending if name is an variable or a method
 *
 * @param name
 * @param value
 * @param active
 */
void send(std::string name, std::string value, WindowID active = 0);

/**
 * @return a list of options the can be triggered
 */
ArrayList<Option*>& getOptions();

/**
 * @return 1 iff there are send messages whose receipt has not been confirmed
 */
bool hasOutStandingMessages(void);
/**
 * @return the number of calls to send
 */
uint32_t getNumberOfMessageSent();

/**
 * @return the return value of the last mesage sent
 */
int getLastMessageExitCode(void);
#endif
