/**
 * @file mywm-util.c
 */

/// \cond
#include <assert.h>
#include <stdlib.h>
#include <string.h>
/// \endcond

#include "globals.h"
#include "mywm-util.h"
#include "windows.h"
#include "workspaces.h"
#include "masters.h"
#include "monitors.h"
#include "xsession.h"
#include "logger.h"


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
void quit(void){
    LOG(LOG_LEVEL_INFO, "Shutting down\n");
    //shuttingDown=1;
    if(dis)
        catchError(xcb_set_selection_owner_checked(dis, XCB_NONE, WM_SELECTION_ATOM, XCB_CURRENT_TIME));
    closeConnection();
    LOG(LOG_LEVEL_INFO, "destroying context\n");
    resetContext();
    exit(0);
}

