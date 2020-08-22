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

#include <string>

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
enum ChildRedirection {
    /// Don't redirect
    NO_REDIRECTION = 0,
    /// Redirect child input to STATUS_FD_EXTERNAL_READ
    REDIRECT_CHILD_INPUT_ONLY,
    /// Redirect child output to STATUS_FD_EXTERNAL_WRITE
    REDIRECT_CHILD_OUTPUT_ONLY,
    /// Redirect child input/output to STATUS_FD_EXTERNAL_READ/STATUS_FD_EXTERNAL_WRITE
    REDIRECT_BOTH = 3
};

/// Used to track how many times this program has been restarted
extern const int RESTART_COUNTER;
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
 * @param force don't try to safly shutdown
 */
void restart()__attribute__((__noreturn__));

/**
 * Forks and, if command is not NULL, exec command in SHELL
 * The command run shall terminate independently of the parent.
 * @param command
 * @return the pid of the new process
 */
int spawn(const char* command);
/**
 * @copydoc spawn(const char*)
 *
 * @param silent if 1, suppress output
 * @return the pid of the new process
 */
int spawn(const char* command, bool silent);
/**
 * wrapper for spawn(command, 1)
 * @param command
 */
static inline int spawnSilent(const char* command) {return spawn(command, 1);}

/**
 * @copydoc spawn(const char*)
 * Like spawn but the child's stdin/stdout refers to our STATUS_FD_EXTERNAL_READ/STATUS_FD_EXTERNAL_WRITE
 *
 * @param ioRedirection determines if there will be pipes setup between the parent and child process
 * @return the pid of the new process
 */
int spawnPipe(const char* command, ChildRedirection ioRedirection = REDIRECT_BOTH);

/**
 * Calls NOTIFY_CMD with summary and body
 *
 * NOTIFY_CMD is a cmd like notify-send to give the user notifications
 * @param summary
 * @param body
 */
void notify(std::string summary, std::string body);

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
 * function to run in child after a fork/spawn call
 */
extern void (*onChildSpawn)(void);
/**
 * Set environment vars such to help old clients know which master device to use
 */
void setClientMasterEnvVar(void);

#endif
