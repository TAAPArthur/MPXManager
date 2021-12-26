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
#include <stdbool.h>

/// @{
/// Exit codes
#define NORMAL_TERMINATION  0
#define X_ERROR             1
#define SYS_CALL_FAILED     64
#define WM_ALREADY_RUNNING  65
#define WM_NOT_RESPONDING   66
#define INVALID_OPTION      67
#define FAILED_VALIDATION   68
/// @}

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

/// enum to specify how pipes are handled
typedef enum {
    /// Don't redirect
    NO_REDIRECTION = 0,
    /// Redirect child input to STATUS_FD_EXTERNAL_READ
    REDIRECT_CHILD_INPUT_ONLY,
    /// Redirect child output to STATUS_FD_EXTERNAL_WRITE
    REDIRECT_CHILD_OUTPUT_ONLY,
    /// Redirect child input/output to STATUS_FD_EXTERNAL_READ/STATUS_FD_EXTERNAL_WRITE
    REDIRECT_BOTH = 3
} ChildRedirection ;

/// Used to track how many times this program has been restarted
extern int RESTART_COUNTER;
///Returns the field descriptors used to communicate WM status to an external program
extern int statusPipeFD[4];

/// the number of arguments passed into the main method
extern int numPassedArguments;
/// the argument list passed into the main method
extern const char* const* passedArguments;
/**
 * exits the application
 * @param exitCode exit status of the program
 */
void quit(int exitCode)__attribute__((__noreturn__));

/**
 * restart the application
 * @param force don't try to safly shutdown
 */
void restart()__attribute__((__noreturn__));

/**
 * Waits for the child process given by pid to terminate
 * @param pid the child
 * @return the pid of the terminated child or -1 on error
 * @see waitpid()
 */
int waitForChild(int pid);

/**
 * Forks and, if command is not NULL, exec command in SHELL
 * The command run shall terminate independently of the parent.
 * @param command
 * @return the pid of the new process
 */
void spawn(const char* command);
/**
 * @copydoc spawn(const char*)
 *
 * @param preserveSession if true, spawn won't double fork
 * @param silent if 1, suppress output
 * @return the pid of the new process
 */
int spawnChild(const char* command);

static inline int spawnAndWait(const char* command) {return waitForChild(spawnChild(command));}

/**
 * @copydoc spawn(const char*)
 * Like spawn but the child's stdin/stdout refers to our STATUS_FD_EXTERNAL_READ/STATUS_FD_EXTERNAL_WRITE
 *
 * @param ioRedirection determines if there will be pipes setup between the parent and child process
 * @param preserveSession if true, spawn won't double fork
 * @return the pid of the new process
 */
void spawnPipe(const char* command, ChildRedirection ioRedirection);
int spawnPipeChild(const char* command, ChildRedirection ioRedirection);

/**
 * Like spawn but redirects child output to /dev/null
 */
int spawnSilent(const char* command);

/**
 * Dups stdout and stderror to /dev/null
 */
void suppressOutput(void) ;
/**
 * function to run in child after a fork/spawn call
 */
extern void (*onChildSpawn)(void);

void set_handlers();

/**
 * Used to create a signal handler for functions that don't return
 *
 * @param sig the signal to catch
 * @param handler the callback function
 *
 * @return 0 on success or the result from sigaction
 */
int createSigAction(int sig, void(*callback)(int));

#endif
