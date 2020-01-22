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

/// Used to track how many times this program has been restarted
extern int RESTART_COUNTER;
///Returns the field descriptors used to communicate WM status to an external program
extern int statusPipeFD[4];

/// the number of arguments passed into the main method
extern int numPassedArguments;
/// the argument list passed into the main method
extern char* const* passedArguments;
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
