
#ifndef TESTS_XUNITTESTS_H_
#define TESTS_XUNITTESTS_H_

#include "UnitTests.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>

#include "../default-rules.h"
#include "../xsession.h"
#include "../windows.h"
#include "../logger.h"
#include "../events.h"
#include "../bindings.h"
#include "../globals.h"
#include "../test-functions.h"

#define ATOMIC(code...) do {lock();code;unlock();}while(0)
extern pthread_t pThread;
#define START_MY_WM \
        pThread=runInNewThread(runEventLoop,NULL,0);
#define KILL_MY_WM  requestEventLoopShutdown();pthread_join(pThread,NULL);

void destroyWindow(WindowID win);
int createUserIgnoredWindow(void);
int  createUnmappedWindow(void);
void addDummyIgnoreRule(void);
int createIgnoredWindow(void);
int createNormalWindow(void);
int createInputWindow(int input);
int createNormalSubWindow(int parent);
int mapWindow(WindowID win);
int  mapArbitraryWindow(void);
void createSimpleContext();
void createSimpleContextWthMaster();
void createContextAndSimpleConnection();
void destroyContextAndConnection();
void openXDisplay();
void triggerAllBindings(int mask);
void triggerBinding(Binding* b);
void* getNextDeviceEvent();
void waitToReceiveInput(int mask);
void consumeEvents();
int waitForNormalEvent(int mask);

int isWindowMapped(WindowID win);
Window setDock(WindowID id, int i, int size, int full);
Window createDock(int i, int size, int full);
void fullCleanup();

void loadSampleProperties(WindowInfo* winInfo);
int getExitStatusOfFork();
void waitForCleanExit();
void setProperties(WindowID win);
void checkProperties(WindowInfo* winInfo);
/**
 * checks to marking the window ids in stacking are in bottom to top order
 */
int checkStackingOrder(WindowID* stackingOrder, int num);
static inline void* isInList(ArrayList* list, int value){
    return find(list, &value, sizeof(int));
}
#endif /* TESTS_UNITTESTS_H_ */
