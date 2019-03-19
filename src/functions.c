/**
 * @file functions.c
 * @copybrief functions.h
 */

#include <assert.h>
#include <err.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>


#include "bindings.h"
#include "devices.h"
#include "functions.h"
#include "globals.h"
#include "layouts.h"
#include "logger.h"
#include "masters.h"
#include "monitors.h"
#include "mywm-util.h"
#include "windows.h"
#include "wmfunctions.h"
#include "workspaces.h"

static int getNextInteractableWindowInStack(ArrayList* stack, int pos, int delta){
    if(!isNotEmpty(stack))return -1;
    int dir = delta == 0 ? 0 : delta > 0 ? 1 : -1;
    int index;
    for(int i = 0; i < getSize(stack); i++){
        index = getNextIndex(stack, pos, delta);
        if(isActivatable(getElement(stack, index))){
            return index;
        }
        delta += dir;
    }
    return -1;
}
static int getNextIndexInStack(int dir){
    ArrayList* activeWindows = getActiveWindowStack();
    if(!isNotEmpty(activeWindows))return -1;
    int index = -1;
    if(getFocusedWindow())
        index = indexOf(activeWindows, getFocusedWindow(), sizeof(int));
    if(index == -1)
        index = 0;
    int newIndex = getNextInteractableWindowInStack(activeWindows, index, dir);
    return newIndex;
}
WindowInfo* getNextWindowInStack(int dir){
    int index = getNextIndexInStack(dir);
    return index >= 0 ? getElement(getActiveWindowStack(), index) : NULL;
}
void cycleWindows(int delta){
    setFocusStackFrozen(1);
    Master* m = getActiveMaster();
    int index = getNextInteractableWindowInStack(getWindowStackByMaster(m), m->focusedWindowIndex, delta);
    if(index >= 0)
        activateWindow(getElement(getWindowStackByMaster(m), index));
}

void endCycleWindows(void){
    setFocusStackFrozen(0);
}

//////Run or Raise code


WindowInfo* findWindow(Rule* rule, ArrayList* searchList, ArrayList* ignoreList){
    WindowInfo* winInfo = NULL;
    UNTIL_FIRST(winInfo, searchList,
                (!ignoreList || !find(ignoreList, winInfo, sizeof(int))) &&
                isActivatable(winInfo) &&
                doesWindowMatchRule(rule, winInfo));
    assert(winInfo == NULL || doesWindowMatchRule(rule, winInfo));
    return winInfo;
}
static int processFindResult(WindowInfo* target){
    if(target){
        activateWindow(target);
        updateWindowCache(target);
        return target->id;
    }
    else return 0;
}
int findAndRaiseLazy(Rule* rule){
    return processFindResult(findWindow(rule, getAllWindows(), NULL));
}
int findAndRaise(Rule* rule){
    ArrayList* topWindow = getActiveMasterWindowStack();
    if(getFocusedWindow())
        updateWindowCache(getFocusedWindow());
    ArrayList* windowsToIgnore = getWindowCache(getActiveMaster());
    WindowInfo* target = findWindow(rule, topWindow, windowsToIgnore);
    if(!target){
        target = findWindow(rule, getAllWindows(), windowsToIgnore);
        if(!target && isNotEmpty(windowsToIgnore)){
            clearWindowCache();
            target = findWindow(rule, topWindow, NULL);
        }
    }
    return processFindResult(target);
}


void setClientMasterEnvVar(void){
    char strValue[32];
    if(getActiveMaster()){
        sprintf(strValue, "%d", getActiveMasterKeyboardID());
        setenv(CLIENT[0], strValue, 1);
        sprintf(strValue, "%d", getActiveMasterPointerID());
        setenv(CLIENT[1], strValue, 1);
    }
    if(LD_PRELOAD_INJECTION)setenv("LD_PRELOAD", LD_PRELOAD_PATH, 1);
}

int spawnPipe(char* command){
    LOG(LOG_LEVEL_INFO, "running command %s\n", command);
    resetPipe();
    pipe(statusPipeFD);
    int pid = fork();
    if(pid == 0){
        setClientMasterEnvVar();
        close(statusPipeFD[1]);
        dup2(statusPipeFD[0], 0);
        execl(SHELL, SHELL, "-c", command, (char*)0);
    }
    else if(pid < 0)
        err(1, "error forking\n");
    close(statusPipeFD[0]);
    return pid;
}

int spawn(char* command){
    LOG(LOG_LEVEL_INFO, "running command %s\n", command);
    int pid = fork();
    if(pid == 0){
        setClientMasterEnvVar();
        setsid();
        execl(SHELL, SHELL, "-c", command, (char*)0);
    }
    else if(pid < 0)
        err(1, "error forking\n");
    return pid;
}
int waitForChild(int pid){
    return waitpid(pid, NULL, 0);
}


int focusBottom(void){
    return isNotEmpty(getActiveWindowStack()) ? activateWindow(getLast(getActiveWindowStack())) : 0;
}
int focusTop(void){
    return isNotEmpty(getActiveWindowStack()) ? activateWindow(getHead(getActiveWindowStack())) : 0;
}
void shiftTop(void){
    isNotEmpty(getActiveWindowStack()) ? shiftToHead(getActiveWindowStack(), getNextIndexInStack(0)) : NULL;
}
void swapWithTop(void){
    isNotEmpty(getActiveWindowStack()) ? swap(getActiveWindowStack(), 0, getNextIndexInStack(0)) : NULL;
}
void swapPosition(int dir){
    isNotEmpty(getActiveWindowStack()) ? swap(getActiveWindowStack(), getNextIndexInStack(0),
            getNextIndexInStack(dir)) : NULL;
}
int shiftFocus(int dir){
    return activateWindow(getNextWindowInStack(dir));
}

void sendWindowToWorkspaceByName(WindowInfo* winInfo, char* name){
    moveWindowToWorkspace(winInfo, getIndexFromName(name));
}





void resizeWindowWithMouse(WindowInfo* winInfo){
    assert(winInfo);
    short values[4];
    short delta[2];
    getMouseDelta(getActiveMaster(), delta);
    for(int i = 0; i < 4; i++)
        values[i] = getGeometry(winInfo)[i] + (i >= 2) * delta[i - 2];
    for(int i = 0; i < 2; i++)
        if(values[i + 2] < 0){
            values[i + 2] = -values[i];
            values[i] += delta[i];
        }
    processConfigureRequest(winInfo->id, values, 0, 0,
                            XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT);
    //TODO check success
    for(int i = 0; i < 4; i++)
        getGeometry(winInfo)[i] = values[i];
}
void moveWindowWithMouse(WindowInfo* winInfo){
    assert(winInfo);
    short values[2];
    short delta[2];
    getMouseDelta(getActiveMaster(), delta);
    for(int i = 0; i < 2; i++)
        values[i] = getGeometry(winInfo)[i] + delta[i];
    processConfigureRequest(winInfo->id, values, 0, 0, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y);
    //TODO check success
    getGeometry(winInfo)[0] = values[0];
    getGeometry(winInfo)[1] = values[1];
}

static void setMasterTarget(int id){
    getActiveMaster()->targetWindow = id;
}

void startMouseOperation(WindowInfo* winInfo){
    setMasterTarget(winInfo->id);
    lockWindow(winInfo);
}
void stopMouseOperation(WindowInfo* winInfo){
    setMasterTarget(0);
    unlockWindow(winInfo);
}

void activateWorkspaceUnderMouse(void){
    const short* pos = getLastKnownMasterPosition();
    Rect rect = {pos[0], pos[1], 1, 1};
    Monitor* monitor = NULL;
    UNTIL_FIRST(monitor, getAllMonitors(),  intersects(monitor->base, rect));
    if(monitor)
        setActiveWorkspaceIndex(getWorkspaceFromMonitor(monitor)->id);
}
