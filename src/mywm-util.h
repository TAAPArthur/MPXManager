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


/// the number of arguments passed into the main method
extern int numPassedArguments;
/// the argument list passed into the main method
extern char** passedArguments;
/**
 * execute code in an atomic manner
 * @param code
 */
#define ATOMIC(code...) do {lock();code;unlock();}while(0)
/**
 * Locks/unlocks the global mutex
 *
 * It is not safe to modify most structs from multiple threads so the main event loop lock/unlocks a
 * global mutex. Any addition thread that runs alongside the main thread of if in general, there
 * is a race, lock/unlock should be used
 */
void lock(void);
///cpoydoc lock(void)
void unlock(void);

/**
 * Destroys the resources related to our internal representation.
 * It does nothing if resources have already been cleared or never initally created
 */
void resetContext();

/**
 * Requests all threads to terminate
 */
void requestShutdown(void);
/**
 * Indicate to threads that the system is shutting down;
 */
int isShuttingDown(void);
/**
 * Runs method in a new thread
 * @param method the method to run
 * @param arg the argument to pass into method
 * @return a pthread identifier
 */
void runInNewThread(void* (*method)(void*), void* arg, char*);
/**
 * exits the application
 * @param exitCode exit status of the program
 */
void quit(int exitCode);

/**
 * restart the application
 */
void restart(void);

/**
 * Joins all threads started via runInNewThread
 */
void waitForAllThreadsToExit(void);

#endif
