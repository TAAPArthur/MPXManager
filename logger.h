/**
 * @file logger.h
 *
 * @brief Basic logging and var dumping
 */

#ifndef LOGGER_H_
#define LOGGER_H_
#include <stdio.h>
#include "mywm-structs.h"

enum {LOG_LEVEL_ALL,LOG_LEVEL_TRACE,LOG_LEVEL_DEBUG,LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,LOG_LEVEL_ERROR,LOG_LEVEL_NONE};

#define LOGGING 1
#define LOG(i,...) \
            do { if (LOGGING) if(i>=getLogLevel()) fprintf(stderr,  __VA_ARGS__); } while (0)

int getLogLevel();
void setLogLevel(int level);
void dumpWindowInfo(WindowInfo* win);
void dumpAtoms(xcb_atom_t*atoms,int numberOfAtoms);
char *genericEventTypeToString(int type);
char *eventTypeToString(int type);

#endif /* LOGGER_H_ */
