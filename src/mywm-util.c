/**
 * @file mywm-util.c
 */

/// \cond
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
/// \endcond

#include "globals.h"
#include "mywm-util.h"
#include "windows.h"
#include "workspaces.h"
#include "masters.h"
#include "monitors.h"
#include "xsession.h"
#include "logger.h"


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
    deleteList(getListOfWorkspaces());
    for(int i = 0; i < NUMBER_OF_WORKSPACES; i++)
        addToList(getListOfWorkspaces(), createWorkspace());
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
    if(STATUS_FD)
        close(STATUS_FD);
    //shuttingDown=1;
    if(dis)
        catchError(xcb_set_selection_owner_checked(dis, XCB_NONE, WM_SELECTION_ATOM, XCB_CURRENT_TIME));
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

