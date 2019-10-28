/**
 * @file
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
#ifndef MPX_SYSTEM
#define MPX_SYSTEM

/// name of env variable that (if set) should be the default pointer
#define DEFAULT_POINTER_ENV_VAR_NAME "CLIENT_POINTER"
/// name of env variable that (if set) should be the default keyboard
#define DEFAULT_KEYBOARD_ENV_VAR_NAME "CLIENT_KEYBOARD"

/// The other end of STATUS_FD
#define STATUS_FD_EXTERNAL_READ statusPipeFD[0]
///Returns the field descriptor used to communicate WM status to an external program
#define STATUS_FD statusPipeFD[1]
///Returns the field descriptor used to communicate WM status to an external program
#define STATUS_FD_READ statusPipeFD[2]

/// The other end of STATUS_FD_READ
#define STATUS_FD_EXTERNAL_WRITE statusPipeFD[3]

///Returns the field descriptors used to communicate WM status to an external program
extern int statusPipeFD[4];

/// the number of arguments passed into the main method
extern int numPassedArguments;
/// the argument list passed into the main method
extern char* const* passedArguments;
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
///copydoc lock(void)
void unlock(void);

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
void runInNewThread(void* (*method)(void*), void* arg, const char*);
/**
 * @return the number of threads that have not terminated and been cleaned up
 */
int getNumberOfThreads(void);
/**
 * exits the application
 * @param exitCode exit status of the program
 */
void quit(int exitCode)__attribute__((__noreturn__));

/**
 * restart the application
 */
void restart(void)__attribute__((__noreturn__));

/**
 * Joins all threads started via runInNewThread
 */
void waitForAllThreadsToExit(void);

/**
 * Forks and runs command in SHELL
 * The command run shall terminate independently of the parent.
 * If MPX_TESTING is defined, then a NULL value of shell will result in the child return 0 instead of running exec
 * @param command
 * @return the pid of the new process
 */
int spawn(const char* command);
/**
 * @copydoc spawn(const char*)
 *
 * @param silent if 1, suppress output
 *
 */
int spawn(const char* command, bool silent);
/**
 * wrapper for spawn(command, 1)
 *
 * @param command
 *
 * @return
 */
static inline int spawnSilent(const char* command) {return spawn(command, 1);}

/**
 * Like spawn but the child's stdin refers to our stdout
 * @return the pid of the new process
 */
int spawnPipe(const char* command);

/**
 * Dups stdout and stderror to /dev/null
 */
void suppressOutput(void) ;
/**
 * Waits for the child process given by pid to terminate
 * @param pid the child
 * @return the pid of the terminated child or -1 on error
 * @see waitpid()
 */
int waitForChild(int pid);
/**
 * Closes all pipes opend by spawnPipe and zeros out statusPipeFD
 */
void resetPipe();
/**
 * function to run in child after a fork/spawn call
 */
extern void (*onChildSpawn)(void);
/**
 * Destroys internal structs
 */
void destroyAllLists();
/**
 * Set environment vars such to help old clients know which master device to use
 */
void setClientMasterEnvVar(void);
#endif
