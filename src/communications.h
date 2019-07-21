/**
 * @file communications.h
 * Contains methods to used to remote communicate with the WM
 */
#ifndef MPXMANAGER_COMMUTNICATE_H_
#define MPXMANAGER_COMMUTNICATE_H_

#include "bindings.h"

/// Specifies type of option
enum OptionType {
    VAR_BOOL, VAR_INT, VAR_STRING, FUNC_VOID, FUNC_INT, FUNC_STRING
};
struct VarFunc {
    union {
        void* var;
        void(*func)();
    } var;
    OptionType type;
    VarFunc(unsigned int* i): type(VAR_INT) {
        var.var = i;
    }
    VarFunc(bool* i): type(VAR_BOOL) {
        var.var = i;
    }
    VarFunc(std::string* s): type(VAR_STRING) {
        var.var = s;
    }
    VarFunc(void(*f)()): type(FUNC_VOID) {
        var.func = f;
    }
    VarFunc& operator=(void(*f)()) {
        var.func = f;
        return *this;
    }
    VarFunc(void (*f)(WindowMask)): type(FUNC_INT) {
        var.func = (void(*)())f;
    }
    VarFunc(void (*f)(std::string)): type(FUNC_STRING) {
        var.func = (void(*)())f;
    }
};
typedef enum OptionFlags {
    /// if true and the command was sent, program will fork before executing this command and attempt redirect output to the caller's stdout
    FORK_ON_RECEIVE = 1,
    REMOTE = 2,
    CONFIRM_EARLY = 4,
} OPTION_FLAGS;
/// Holds info to map string to a given operation
struct Option {

    /// option name; --$name will trigger option
    std::string name;
    /// detail how we should respond to the option
    struct VarFunc varFunc;
    int flags = 0;
    void call(std::string value) {(*this)(value);}

    OptionType getType()const {return varFunc.type;}
    void* getVar()const {return varFunc.var.var;}

    bool matches(std::string, bool);
    /**
     * If option holds a function, calls the function bound to option with argument value.
     * Else sets name to value
     *
     * @param value the value to set/pass. If NULL, a value of '1' is used
     */
    void operator()(std::string value);
};


/**
 * Finds a option by name
 *
 * @param name the name of the option
 * @param len the len of name
 *
 * @return the first option matching name or NULL
 */
Option* findOption(std::string name, std::string value) ;

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

ArrayList<Option*>& getOptions();

/**
 * @return 1 iff there are send messages whose receipt has not been confirmed
 */
bool hasOutStandingMessages(void);
#endif
