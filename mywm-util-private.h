/**
 * @file mywm-util-private.h
 *
 * @brief "Private" functions of mywm-util
 *
 * These are functions that user should never need to call directly,
 * but can if they want
 */

#ifndef MYWM_XUTIL_PRIVATE
#define MYWM_XUTIL_PRIVATE

#include "util.h"
#include "mywm-structs.h"


/**
 * Creates a pointer to a Master instance and sets its id to id.
 * @return pointer to object
 */
Master *createMaster(int id,int pointerId);

/**
 * Init a an array of workspaces to default values.
 * @param numberOfWorkspaces total number of workspace to create
 * @return pointer to the first workspace
 */
Workspace*createWorkSpaces(int size);

Node* isWindowInWorkspace(WindowInfo* winInfo,int workspaceIndex);
int isWindowInVisibleWorkspace(WindowInfo* winInfo);
int removeWindowFromWorkspace(WindowInfo* winInfo,int workspaceIndex);
int addWindowToWorkspaceAtLayer(WindowInfo* winInfo,int workspaceIndex,int layer);
int addWindowToWorkspace(WindowInfo*info,int workspaceIndex);
/**
 * Removes window from context and master(s)/workspace lists.
 * The Nodes containing the window and the windows value are freeded also
 * @param context
 * @param winToRemove
 */
int removeWindow(unsigned int winToRemove);

void setWorkspaceNames(char*names[],int numberOfNames);

int resizeMonitorToAvoidStruct(Monitor*m,WindowInfo*winInfo);
void resizeAllMonitorsToAvoidAllStructs();
void resizeAllMonitorsToAvoidStruct(WindowInfo*winInfo);

void resizeMonitor(Monitor*monitor,int x,int y,int width, int height);

int removeMonitor(unsigned int id);


#endif
