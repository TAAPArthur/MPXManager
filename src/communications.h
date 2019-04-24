/**
 * @file communications.h
 * Contains methods to used to remote communicate with the WM
 */
#ifndef MPXMANAGER_COMMUTNICATE_H_
#define MPXMANAGER_COMMUTNICATE_H_

#include "bindings.h"

/// Specifes type of option
typedef enum {
    BOOL_VAR, INT_VAR, LONG_VAR, STRING_VAR, FUNC, FUNC_STR, FUNC_INT
} OptionTypes;
/// Holds info to map string to a given operation
typedef struct {
    /// option name; --$name will trigger option
    char* name;
    /// detail how we should respond to the option
    union {
        /// set the var
        void* var;
        /// call a function
        BoundFunction boundFunction;
    } var;
    /// the type of var.var
    OptionTypes type;
    /// character that may also be used to trigger binding
    char shortName;
} Option;

/**
 * Finds a option by name
 *
 * @param name the name of the option
 * @param len the len of name
 *
 * @return the first option matching name or NULL
 */
Option* findOption(char* name, int len);
/**
 * Prints info for option
 *
 * @param option
 */
void printOption(Option* option);

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
 * @param name
 * @param value
 */
void send(char* name, char* value);
/**
 * If option holds a function, calls the function bound to option with argument value.
 * Else sets name to value
 *
 * @param option
 * @param value the value to set/pass. If NULL, a value of '1' is used
 */
void callOption(Option* option, char* value);


/**
 * Adds to the list of options that can be triggered from external commands
 *
 * @param option
 * @param len
 */
void addOption(Option* option, int len);
/**
 * Loads the default list of options
 */
void loadDefaultOptions(void);
#endif
