
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
#include "../logger.h"
#include "../events.h"
#include "../bindings.h"
#include "../globals.h"
#include "../test-functions.h"

extern pthread_t pThread;
#define START_MY_WM \
        pThread=runInNewThread(runEventLoop,NULL,0);
#define KILL_MY_WM  requestEventLoopShutdown();pthread_join(pThread,NULL);

void destroyWindow(int win);
int createUserIgnoredWindow(void);
int  createUnmappedWindow(void);
int createIgnoredWindow(void);
int createNormalWindow(void);
int mapWindow(int win);
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

int isWindowMapped(int win);
Window createDock(int i,int size,int full);
void fullCleanup();

int getExitStatusOfFork();
void waitForCleanExit();
void setProperties(int win);
void checkProperties(WindowInfo*winInfo);
int getActiveFocus();
#endif /* TESTS_UNITTESTS_H_ */
