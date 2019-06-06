#include <assert.h>
#include <err.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "globals.h"
#include "logger.h"
#include "masters.h"
#include "windows.h"
#include "monitors.h"
#include "workspaces.h"

void setClientMasterEnvVar(void){
    char strValue[8];
    if(SPAWN_ENV && getActiveMaster()){
        sprintf(strValue, "%d", getActiveMasterKeyboardID());
        setenv(CLIENT[0], strValue, 1);
        sprintf(strValue, "%d", getActiveMasterPointerID());
        setenv(CLIENT[1], strValue, 1);
        if(getFocusedWindow()){
            sprintf(strValue, "%d", getFocusedWindow()->id);
            setenv("_WIN_ID", strValue, 1);
        }
        Monitor* m = getMonitorFromWorkspace(getActiveWorkspace());
        struct {char* name; const short* bounds;} rect[] = {
            {"WIN", getFocusedWindow() ? getGeometry(getFocusedWindow()) : 0},
            {"VIEW", m ? &m->view.x : 0},
            {"MON", m ? &m->base.x : 0},
            {"ROOT", getRootBounds()},
        };
        char* name[4][32] = {"_%s_X", "_%s_Y", "_%s_WIDTH", "_%s_HEIGHT"};
        char strName[32];
        for(int i = 0; i < LEN(rect); i++){
            if(rect[i].bounds){
                for(int n = 0; n < 4; n++){
                    sprintf(strName, name[i][n], rect[i].name);
                    sprintf(strValue, "%d", rect[i].bounds[n]);
                    setenv(strName, strValue, 1);
                }
            }
        }
    }
    if(LD_PRELOAD_INJECTION)
        setenv("LD_PRELOAD", LD_PRELOAD_PATH, 1);
}
void resetPipe(){
    if(statusPipeFD[0]){
        // statusPipeFD[0]) is closed right after a call to pipe;
        close(statusPipeFD[1]);
        close(statusPipeFD[2]);
        statusPipeFD[0] = statusPipeFD[1] = 0;
    }
}

int spawnPipe(char* command){
    LOG(LOG_LEVEL_INFO, "running command %s\n", command);
    resetPipe();
    pipe(statusPipeFD);
    pipe(statusPipeFD + 2);
    for(int i = 0; i < LEN(statusPipeFD); i++)
        assert(statusPipeFD[i]);
    int pid = fork();
    if(pid == 0){
        setClientMasterEnvVar();
        close(statusPipeFD[1]);
        close(statusPipeFD[2]);
        dup2(statusPipeFD[0], STDIN_FILENO);
        dup2(statusPipeFD[3], STDOUT_FILENO);
        execl(SHELL, SHELL, "-c", command, (char*)0);
    }
    else if(pid < 0)
        err(1, "error forking\n");
    close(statusPipeFD[0]);
    close(statusPipeFD[3]);
    return pid;
}

int spawn(char* command){
    LOG(LOG_LEVEL_INFO, "running command %s\n", command);
    int pid = fork();
    if(pid == 0){
        resetPipe();
        setClientMasterEnvVar();
        setsid();
        execl(SHELL, SHELL, "-c", command, (char*)0);
    }
    else if(pid < 0)
        err(1, "error forking\n");
    return pid;
}
int waitForChild(int pid){
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}
