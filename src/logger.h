/**
 * @file logger.h
 * @brief Basic logging and var dumping
 */

#ifndef MPXMANAGER_LOGGER_H_
#define MPXMANAGER_LOGGER_H_


#include <stdio.h>
#include <xcb/xcb.h>

#include "mywm-structs.h"
#include "bindings.h"

/**
 * Prints the stack trace
 */
void printStackTrace(void);

enum {LOG_LEVEL_ALL, LOG_LEVEL_VERBOSE, LOG_LEVEL_TRACE, LOG_LEVEL_DEBUG, LOG_LEVEL_INFO,
      LOG_LEVEL_WARN, LOG_LEVEL_ERROR, LOG_LEVEL_NONE
     };


/**
 * Prints the element in arr space separated. Arr is treated like an array of ints
 *
 * @param label some prefix
 * @param arr
 * @param size the number of elements
 * @param suffix some suffix
 *
 * @return
 */
#define PRINT_ARR(label,arr,size,suffix){LOG(LOG_LEVEL_INFO,label " Arr:");for(int _n=0;_n<size;_n++)LOG(LOG_LEVEL_INFO,"%ld ",(long)(arr)[_n]);LOG(LOG_LEVEL_INFO,suffix);}

/**
 * Prints the element in arr space separated. Arr is treated like a list of ints
 *
 * @param label some prefix
 * @param arr
 * @param suffix some suffix
 *
 * @return
 */
#define PRINT_LIST(label,arr,suffix){LOG(LOG_LEVEL_INFO,label " Arr:");FOR_EACH(int*,i,arr)LOG(LOG_LEVEL_INFO,"%ld ",(long)i);LOG(LOG_LEVEL_INFO,suffix);}
/// the init log level
#ifndef INIT_LOG_LEVEL
    #define INIT_LOG_LEVEL LOG_LEVEL_INFO
#endif
/// if false then all logging will be disabled
#ifndef LOGGING
    #define LOGGING 1
#endif


/**
 * If i >= getLogLevel(), the print str
 *
 * @param i
 * @param str
 *
 */
#define LOG(i,str...) \
    do{if(isLogging(i)) fprintf(stderr, str);}while(0)

/**
 * if i>= getLogLevel(), run code
 *
 * @param i
 * @param code
 *
 */
#define LOG_RUN(i,code...) \
    do{if(isLogging(i)) code;}while(0)

/**
 * @return the current log level
 */
int getLogLevel();
/**
 * Sets the log level
 * @param level the new log level
 */
void setLogLevel(int level);
/**
 * If message at log level i will be logged
 *
 * @param i
 *
 * @return 1 if the message will be logged
 */
static inline int isLogging(int i){
    return LOGGING && i >= getLogLevel();
}
/**
 * Prints a summary of the state of the WM
 */
void printSummary(void);

/**
 * Prints a summary of window stack for all workspaces
 */
void printWindowStackSummary(ArrayList* list);
/**
 * Prints a summary of all monitors
 */
void printMonitorSummary(void);
/**
 * Prints info related to m
 *
 * @param m
 */
void dumpMonitorInfo(Monitor* m);
/**
 * Prints a summary of all master devices
 */
void printMasterSummary(void);
/**
 * Prints info on all windows given that they contain filterMask
 *
 * @param filterMask
 */
void dumpAllWindowInfo(int filterMask);
/**
 * Dumps all windows whose title,class,resource or type match match
 *
 * @param match
 */
void dumpMatch(char* match);
/**
 * Prints info on window
 *
 * @param win
 */
void dumpWindowInfo(WindowInfo* win);

/**
 * Prints info on boundFunction
 *
 * @param boundFunction
 */
void dumpBoundFunction(BoundFunction* boundFunction);

/**
 * Prints the name of each element of atoms
 *
 * @param atoms
 * @param numberOfAtoms
 */
void dumpAtoms(xcb_atom_t* atoms, int numberOfAtoms);
/**
 * Stringifies type
 *
 * @param type a regular event type like XCB_CREATE_NOTIFY
 *
 * @return the string representation of type if known
 */
char* eventTypeToString(int type);

/**
 * Stringifies opcode
 * @param opcode a major code from a xcb_generic_error_t object
 * @return the string representation of type if known
 */
char* opcodeToString(int opcode);
/**
 * Prints all components of mask
 *
 * @param mask
 */
void printMask(WindowMask mask);
/**
 * Catches an error but does not nothing
 *
 * @param cookie the result of and xcb_.*_checked function
 *
 * @return the error_code if error was found or 0
 */
int catchErrorSilent(xcb_void_cookie_t cookie);
/**
 * Catches an error and log it
 *
 * @param cookie the result of and xcb_.*_checked function
 *
 * @return the error_code if error was found or 0
 * @see logError
 */
int catchError(xcb_void_cookie_t cookie);
/**
 * Prints info related to the error
 *
 * It may trigger an assert; @see CRASH_ON_ERRORS
 *
 * @param e
 */
void logError(xcb_generic_error_t* e);

#endif /* LOGGER_H_ */
