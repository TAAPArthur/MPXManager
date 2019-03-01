/**
 * @file logger.h
 *
 * @brief Basic logging and var dumping
 */

#ifndef LOGGER_H_
#define LOGGER_H_


#include <stdio.h>
#include <err.h>

#include <xcb/xcb.h>
#include "mywm-util.h"


enum {LOG_LEVEL_ALL, LOG_LEVEL_TRACE, LOG_LEVEL_DEBUG, LOG_LEVEL_INFO,
      LOG_LEVEL_WARN, LOG_LEVEL_ERROR, LOG_LEVEL_NONE
     };

#define PRINT_ARR(label,arr,size,suffix){LOG(LOG_LEVEL_DEBUG,label " Arr:");for(int _n=0;_n<size;_n++)LOG(LOG_LEVEL_DEBUG,"%d ",(arr)[_n]);LOG(LOG_LEVEL_DEBUG,suffix);}
#define PRINT_ARR_PTR(arr,size){for(int _n=0;_n<size;_n++)LOG(LOG_LEVEL_DEBUG,"%d ",*(int*)(arr)[_n]);LOG(LOG_LEVEL_DEBUG,"\n");}

#define LOGGING 1
#define LOG(i,...) \
            do { if (LOGGING) if(i>=getLogLevel()) fprintf(stderr, __VA_ARGS__); } while (0)

int getLogLevel();
void setLogLevel(int level);
void printArrayList(ArrayList* list);
void printSummary(void);
void printMonitorSummary(void);
void printMasterSummary(void);
void dumpAllWindowInfo(void);
void dumpWindowInfo(WindowInfo* win);
void dumpAtoms(xcb_atom_t* atoms, int numberOfAtoms);
char* genericEventTypeToString(int type);
char* eventTypeToString(int type);

int catchError(xcb_void_cookie_t cookie);
void logError(xcb_generic_error_t* e);

#endif /* LOGGER_H_ */
