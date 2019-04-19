#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>

#include "../UnitTests.h"
#include "../TestX11Helper.h"
#include "../../layouts.h"
#include "../../bindings.h"
#include "../../settings.h"
#define clock() getTime();
#define TIME(label,code...)do{\
    double start, end;\
    double deltaT;\
    start = clock();code;end = clock();deltaT = ((double) (end - start))/1000.0; printf("%s%d: %.03f\n",label,NUM,deltaT);\
}while(0);

static int NUM = 1 << 10;
void createManyWindows(void){
    for(int i = 0; i < NUM; i++)
        createNormalWindow();
    flush();
}
void removeAll(void){
    FOR_EACH(WindowInfo*, winInfo, getAllWindows()){
        xcb_destroy_window(dis, winInfo->id);
    }
    flush();
}
volatile int startEventLoop = 0;
void* profile(void* v __attribute__((unused))){
    setLogLevel(LOG_LEVEL_NONE);
    resetContext();
    openXDisplay();
    createManyWindows();
    TIME("Scan", scan(root));
    int idle = getIdleCount();
    startUpMethod = loadNormalSettings;
    TIME("Init",
         TIME("StartupTime", onStartup());
         startEventLoop = 1;
         WAIT_UNTIL_TRUE(idle != getIdleCount())
        );
    consumeEvents();
    ATOMIC(setActiveLayout(NULL));
    lock();
    createManyWindows();
    idle = getIdleCount();
    unlock();
    TIME("Register", WAIT_UNTIL_TRUE(idle != getIdleCount()));
    idle = getIdleCount();
    ATOMIC(removeAll());
    TIME("Remove", WAIT_UNTIL_TRUE(!isNotEmpty(getAllWindows())));
    createNormalWindow();
    flush();
    requestShutdown();
    return NULL;
}
int main(int argc, char**argv){
    if(argc >1)
        NUM=atoi(argv[1]);
    runInNewThread(profile, NULL, 1);
    WAIT_UNTIL_TRUE(startEventLoop);
    TIME("Profile", runEventLoop(NULL));
}
