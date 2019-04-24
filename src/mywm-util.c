/**
 * @file mywm-util.c
 * @copybrief mywm-util.h
 */


#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include "globals.h"
#include "logger.h"
#include "masters.h"
#include "monitors.h"
#include "mywm-util.h"
#include "windows.h"
#include "workspaces.h"
#include "xsession.h"


int numPassedArguments;
char** passedArguments;

void resetContext(void){
    deleteList(getAllMonitors());
    while(isNotEmpty(getAllMasters()))
        removeMaster(((Master*)getLast(getAllMasters()))->id);
    while(isNotEmpty(getAllWindows()))
        removeWindow(((WindowInfo*)getLast(getAllWindows()))->id);
    for(int i = 0; i < getNumberOfWorkspaces(); i++){
        clearList(getWindowStack(getWorkspaceByIndex(i)));
        clearList(getLayouts(getWorkspaceByIndex(i)));
    }
    resetWorkspaces();
    addWorkspaces(DEFAULT_NUMBER_OF_WORKSPACES);
}
void resetPipe(){
    if(statusPipeFD[0]){
        close(statusPipeFD[0]);
        close(statusPipeFD[1]);
        statusPipeFD[0] = statusPipeFD[1] = 0;
    }
}
static void stop(void){
    LOG(LOG_LEVEL_INFO, "Shutting down\n");
    resetPipe();
    closeConnection();
    LOG(LOG_LEVEL_INFO, "destroying context\n");
    resetContext();
}

void restart(void){
    stop();
    execv(passedArguments[0], passedArguments);
}
void quit(void){
    stop();
    exit(0);
}

