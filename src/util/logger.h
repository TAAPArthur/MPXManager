/**
 * @file logger.h
 * @brief Basic logging and var dumping
 */

#ifndef MPXMANAGER_LOGGER_H_
#define MPXMANAGER_LOGGER_H_

#include <stdio.h>

/// various logging levels
typedef enum LogLevel {
    /// used to extra verbose logging
    LOG_LEVEL_VERBOSE,
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
} LogLevel;

/**
 * Prints the element in arr space separated. Arr is treated like an array of ints
 *
 * @param label some prefix
 * @param arr
 * @param size the number of elements
 *
 * @return
 */
#define PRINT_ARR(label,arr,size){printf("%s  Arr: ",label);for(int _n=0;_n<size;_n++)printf("%d ",(arr)[_n]);printf("\n");}



/**
 * if i>= getLogLevel(), run code
 *
 * @param i
 * @param code
 *
 */
#define LOG_RUN(i,code...) \
    do{if(isLogging(i)){code;}}while(0)

#define _LOG(LEVEL, str...) do { \
    if(isLogging(LEVEL)) {\
        printContextStr();\
        printf(str);\
        printf("\n");\
    }\
}while(0)

/// @{ Logging macros
/// VERBOSE and TRACE logging macros will be no-ops in release builds
#ifndef NDEBUG
#define VERBOSE(X...) _LOG(LOG_LEVEL_VERBOSE, X)
#define TRACE(X...) _LOG(LOG_LEVEL_TRACE, X)
#else
#define VERBOSE(X...) do{}while(0)
#define TRACE(X...) do{}while(0)
#endif
#define DEBUG(X...) _LOG(LOG_LEVEL_DEBUG, X)
#define INFO(X...)_LOG(LOG_LEVEL_INFO, X)
#define WARN(X...)_LOG(LOG_LEVEL_WARN, X)
#define ERROR(X...)_LOG(LOG_LEVEL_ERROR, X)
/// @}


/// if false then all logging will be disabled
#ifndef LOGGING
#define LOGGING 1
#endif

/**
 * @return the current log level
 */
LogLevel getLogLevel();
/**
 * Sets the log level
 * @param level the new log level
 */
void setLogLevel(LogLevel level);
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
void pushContext(const char* context);
/**
 * Removes the last context pushed via pushContext
 */
void popContext();

/**
 * prints a string representation of everything on the context stack
 */
void printContextStr();

#endif /* LOGGER_H_ */
