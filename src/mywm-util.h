/**
 * @file mywm-util.h
 *
 * @brief Calls to these function do not need a corresponding X11 function
 *
 *
 *
 * In general when creating an instance to a struct all values are set
 * to their defaults unless otherwise specified.
 *
 * The default for ArrayList* is an empty node (multiple values in a struct may point to the same empty node)
 * The default for points is NULL
 * everything else 0
 *
 *
 */
#ifndef MYWM_SESSION
#define MYWM_SESSION

#include "util.h"
#include "mywm-structs.h"

/// the number of arguments passed into the main method
extern int numPassedArguments;
/// the argument list passed into the main method
extern char** passedArguments;
/**
 * Destroys the resources related to our internal representation.
 * It does nothing if resources have already been cleared or never initally created
 */
void resetContext();

/**
 * exits the application
 */
void quit(void);

/**
 * restart the application
 */
void restart(void);


#endif
