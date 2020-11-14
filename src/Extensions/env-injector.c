#include <assert.h>
#include <stdlib.h>
#include "../util/logger.h"
#include "../monitors.h"
#include "../mywm-structs.h"
#include "../system.h"
#include "../windows.h"

static inline void setEnvRect(const char* name, const Rect rect) {
    const char var[4][32] = {"_%s_X", "_%s_Y", "_%s_WIDTH", "_%s_HEIGHT"};
    char strName[32];
    char strValue[16];
    for(int n = 0; n < 4; n++) {
        sprintf(strName, var[n], name);
        if(n<2)
            sprintf(strValue, "%u", ((short*)&rect)[n]);
        else
            sprintf(strValue, "%u", ((unsigned short*)&rect)[n]);
        setenv(strName, strValue, 1);
    }
}
void setClientMasterEnvVar(void) {
    static char strValue[32];
    if(getActiveMaster()) {
        sprintf(strValue, "%u", getActiveMasterKeyboardID());
        setenv(DEFAULT_KEYBOARD_ENV_VAR_NAME, strValue, 1);
        sprintf(strValue, "%u", getActiveMasterPointerID());
        setenv(DEFAULT_POINTER_ENV_VAR_NAME, strValue, 1);
        if(getFocusedWindow()) {
            sprintf(strValue, "%u", getFocusedWindow()->id);
            setenv("_WIN_ID", strValue, 1);
            setenv("_WIN_TITLE", getFocusedWindow()->title, 1);
            setenv("_WIN_CLASS", getFocusedWindow()->className, 1);
            setenv("_WIN_INSTANCE", getFocusedWindow()->instanceName, 1);
            setEnvRect("WIN", getFocusedWindow()->geometry);
        }
        Monitor* m = getActiveWorkspace() ? getMonitor(getActiveWorkspace()) : NULL;
        if(m) {
            setEnvRect("MON", m->base);
            setEnvRect("VIEW", m->view);
            setenv("MONITOR_NAME", m->name, 1);
        }
        const Rect rootBounds = {0, 0, getRootWidth(), getRootHeight()};
        setEnvRect("ROOT", rootBounds);
        if(LD_PRELOAD_INJECTION) {
            const char* previousPreload = getenv("LD_PRELOAD");
            const char* preload_str = LD_PRELOAD_PATH;
            char* buffer = NULL;
            if(previousPreload && previousPreload[0]) {
                buffer = calloc(1, strlen(previousPreload) + strlen(LD_PRELOAD_PATH) + 1);
                strcat(buffer, LD_PRELOAD_PATH);
                strcat(buffer, ":");
                strcat(buffer, previousPreload);
                preload_str = buffer;
            }
            setenv("LD_PRELOAD", preload_str, 1);
            if(buffer)
                free(buffer);
        }
    }
}
