/**
 * @file logger.h
 * @brief Basic logging and var dumping
 */

#ifndef MPXMANAGER_LOGGER_H_
#define MPXMANAGER_LOGGER_H_


#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include "mywm-structs.h"
#include <cstdio>



/**
 * Pritns the element in arr space seperated. Arr is treated like an array of ints
 *
 * @param label some prefix
 * @param arr
 * @param size the number of elements
 * @param suffix some suffix
 *
 * @return
 */
#define PRINT_ARR(label,arr,size,suffix){LOG(LOG_LEVEL_INFO,label " Arr:");for(int _n=0;_n<size;_n++)LOG(LOG_LEVEL_INFO,"%d ",(arr)[_n]);LOG(LOG_LEVEL_INFO,suffix);}



enum {LOG_LEVEL_ALL, LOG_LEVEL_VERBOSE, LOG_LEVEL_TRACE, LOG_LEVEL_DEBUG, LOG_LEVEL_INFO,
    LOG_LEVEL_WARN, LOG_LEVEL_ERROR, LOG_LEVEL_NONE
};


/// the init log level
#ifndef INIT_LOG_LEVEL
#define INIT_LOG_LEVEL LOG_LEVEL_INFO
#endif
/// if false then all logging will be disabled
#ifndef LOGGING
#define LOGGING 1
#endif

/// The FD to log to
#define LOG_FD STDOUT_FILENO

/**
 * If i >= getLogLevel(), the print str
 *
 * @param i
 * @param str
 *
 */
#define LOG(i,str...) LOG_RUN(i, std::printf(str))

/**
 * if i>= getLogLevel(), run code
 *
 * @param i
 * @param code
 *
 */
#define LOG_RUN(i,code...) \
    do{if(isLogging(i)){code;}}while(0)

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
void dumpWindowStack() ;

/**
 * Prints the stack strace
 */
void printStackTrace(void);
/**
 * Prints a summary of the state of the WM
 */
void printSummary(void);
/**
 * Prints all with that have filterMask (if filterMask is 0, print all windows)
 *
 * @param filterMask
 */
void dumpWindow(WindowMask filterMask);

/**
 * Prints all window whose title, class, instance of type equals match
 *
 * @param match
 */
void dumpWindow(std::string match);
#endif /* LOGGER_H_ */
