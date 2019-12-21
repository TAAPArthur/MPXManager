/**
 * @file logger.h
 * @brief Basic logging and var dumping
 */

#ifndef MPXMANAGER_LOGGER_H_
#define MPXMANAGER_LOGGER_H_


#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include <cstdio>

#include "mywm-structs.h"
#include "window-masks.h"



/// The FD to log to
#define LOG_FD STDOUT_FILENO

/**
 * Prints the element in arr space separated. Arr is treated like an array of ints
 *
 * @param label some prefix
 * @param arr
 * @param size the number of elements
 *
 * @return
 */
#define PRINT_ARR(label,arr,size){auto& info = logger.info() <<label << " Arr: ";for(int _n=0;_n<size;_n++)info<<(arr)[_n]<< " ";info<<std::endl;}


/**
 * If i >= getLogLevel(), the print str
 *
 * @param i
 * @param str
 *
 */
#define LOG(i,str...) do{if(isLogging(i)) { \
    size_t needed = snprintf(NULL, 0, str) + 1; \
    char  *buffer = (char*)malloc(needed); \
    sprintf(buffer, str); \
    logger.log(i) << buffer; \
    free(buffer); }}while(0)\


/**
 * if i>= getLogLevel(), run code
 *
 * @param i
 * @param code
 *
 */
#define LOG_RUN(i,code...) \
    do{if(isLogging(i)){code;}}while(0)

/// various logging levels
enum LogLevel {
    /// used to debug specific problems
    LOG_LEVEL_TRACE,
    /// used for debug problems
    LOG_LEVEL_DEBUG,
    /// general information
    LOG_LEVEL_INFO,
    /// for unexpected behavior that is recoverable
    LOG_LEVEL_WARN,
    /// for error messages
    LOG_LEVEL_ERROR,
    /// no logging
    LOG_LEVEL_NONE
};


/// if false then all logging will be disabled
#ifndef LOGGING
#define LOGGING 1
#endif

/**
 * @return the current log level
 */
int getLogLevel();
/**
 * Sets the log level
 * @param level the new log level
 */
void setLogLevel(uint32_t level);
/**
 * If message at log level i will be logged
 *
 * @param i
 *
 * @return 1 if the message will be logged
 */
static inline int isLogging(int i) {
    return LOGGING && i >= getLogLevel();
}
/**
 * Adds to a list of messages that will be appending to all logging messages
 *
 * @param context name of the context
 */
void pushContext(std::string context);
/**
 * Removes the last context pushed via pushContext
 */
void popContext();
/**
 *
 *
 * @return a string representation of everything on the context stack
 */
std::string getContextString();
/**
 * Logging class
 */
class Logger {
    /// @cond
    struct : std::streambuf { int overflow(int c) { return c; } } nullBuffer;
    /// @endcond
    std::ostream nullStream = std::ostream(&nullBuffer);
    std::ostream& logStream = std::cout;

public:
    /**
     * @param level
     *
     * @return  either nullStream or logSteam depening on level
     */
    std::ostream& log(int level) {
        return (isLogging(level) ? logStream : nullStream) << getContextString().c_str();
    }

    /// @{ @see LogLevel
    std::ostream& trace() { return log(LOG_LEVEL_TRACE);}
    std::ostream& info() { return log(LOG_LEVEL_INFO);}
    std::ostream& debug() { return log(LOG_LEVEL_DEBUG);}
    std::ostream& warn() { return log(LOG_LEVEL_WARN);}
    std::ostream& err() { return log(LOG_LEVEL_ERROR);}
    std::ostream& always() { return log(LOG_LEVEL_NONE);}
    /// @}
};
/// global logger variable
extern Logger logger;

/**
 * Prints every window in the active Workspace
 */
void dumpWindowStack() ;
/**
 * Prints just win if it is registered
 * @param win
 */
void dumpSingleWindow(WindowID win) ;

/**
 * Prints the stack strace
 */
void printStackTrace(void);
/**
 * Prints a summary of the state of the WM
 */
void printSummary(void);
/**
 * Prints all set rules
 */
void dumpRules(void);
/**
 * Prints all with that have filterMask (if filterMask is 0, print all windows)
 *
 * @param filterMask
 */
void dumpWindow(WindowMask filterMask = 0);

/**
 * Prints all window whose title, class, instance of type equals match
 *
 * @param match
 */
void dumpWindow(std::string match);
/**
 * Dumps info on master. If master == null, info on the active master will be printed
 *
 * @param master
 */
void dumpMaster(Master* master) ;

#endif /* LOGGER_H_ */
