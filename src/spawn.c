#include <assert.h>
#include <err.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "globals.h"
#include "logger.h"
#include "masters.h"

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
void resetPipe(){
    if(statusPipeFD[0]){
        // statusPipeFD[0]) is closed right after a call to pipe;
        close(statusPipeFD[1]);
        statusPipeFD[0] = statusPipeFD[1] = 0;
    }
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
    return waitpid(pid, NULL, 0);
}
