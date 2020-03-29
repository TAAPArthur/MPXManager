#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>

#include "../../logger.h"
#include "../../globals.h"
#include "../../wm-rules.h"
#include "../../wmfunctions.h"
#include "../../functions.h"
#include "../../functions.h"
#include "../../devices.h"
#include "../../Extensions/mpx.h"
#include "../../Extensions/gestures.h"
#include "../../Extensions/gesture-functions.h"
#include "../tester.h"
#include "../test-mpx-helper.h"
#include "../test-event-helper.h"
#include "../test-x-helper.h"

SET_ENV(createXSimpleEnv, fullCleanup);

MPX_TEST("move_and_click", {
    movePointer(0, 0);
    getGestureBindings().add(new GestureBinding{{GESTURE_TAP}, []{moveAndClick(1);requestShutdown();}, {.count = 2}});
    Point p = {10, 10};
    for(int i = 0; i < 2; i++) {
        startGesture(0, 0, p, p);
        endGesture(0, 0);
    }
    gestureEventLoop();
    short result[2];
    getMousePosition(getActiveMaster(), root, result);
    assertEquals(p.x, result[0]);
    assertEquals(p.y, result[1]);
});
