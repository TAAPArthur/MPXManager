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
#include "../tester.h"
#include "../test-mpx-helper.h"
#include "../test-event-helper.h"
#include "../test-x-helper.h"

static int SCALE_FACTOR = 100 * THRESHOLD_SQ;
SET_ENV(NULL, NULL);

MPX_TEST_ITER("gesture_types_string", GESTURE_UNKNOWN + 1, {
    getGestureTypeString((GestureType)_i);
});
MPX_TEST("gesture_detail_eq", {
    assertEquals(GestureKeyBinding(GESTURE_TAP), GestureKeyBinding(GESTURE_TAP));
    assertEquals(GestureKeyBinding({GESTURE_TAP, GESTURE_TAP}), GestureKeyBinding({GESTURE_TAP, GESTURE_TAP}));
    assert(!(GestureKeyBinding(GESTURE_TAP) == GestureKeyBinding(GESTURE_NORTH_WEST)));
    assert(!(GestureKeyBinding({GESTURE_TAP, GESTURE_TAP}) == GestureKeyBinding({GESTURE_TAP})));
});
TransformMasks MASKS[] = {MirroredXMask, MirroredYMask, MirroredMask, Rotate90Mask};
const struct GestureChecker {
    GestureType type;
    GestureType typeMirrorX;
    GestureType typeMirrorY;
    GestureType typeOpposite;
    GestureType rot90;
} gestureTypeTuples[] = {
    {GESTURE_NORTH_WEST, GESTURE_NORTH_EAST, GESTURE_SOUTH_WEST, GESTURE_SOUTH_EAST, GESTURE_SOUTH_WEST},
    {GESTURE_SOUTH, GESTURE_SOUTH, GESTURE_NORTH, GESTURE_NORTH, GESTURE_EAST},
    {GESTURE_SOUTH_EAST, GESTURE_SOUTH_WEST, GESTURE_NORTH_EAST, GESTURE_NORTH_WEST, GESTURE_NORTH_EAST},
};
MPX_TEST_ITER("validate_dir", LEN(gestureTypeTuples), {
    auto values = gestureTypeTuples[_i];
    assertEquals(values.type, getOppositeDirection(values.typeOpposite));
    assertEquals(values.type, getOppositeDirection(getOppositeDirection(values.type)));
    assertEquals(values.type, getMirroredXDirection(values.typeMirrorX));
    assertEquals(values.type, getMirroredXDirection(getMirroredXDirection(values.type)));
    assertEquals(values.type, getMirroredYDirection(values.typeMirrorY));
    assertEquals(values.type, getMirroredYDirection(getMirroredYDirection(values.type)));
    assertEquals(values.type, getRot270Direction(values.rot90));
    assertEquals(values.type, getRot90Direction(getRot90Direction(values.typeOpposite)));
    assertEquals(values.type, getRot90Direction(getRot270Direction(values.type)));
    assertEquals(values.type, getRot270Direction(getRot270Direction(values.typeOpposite)));
});
static int FAKE_DEVICE_ID = 0;
static void continueGesture(ProductID id, uint32_t seat, Point point) {
    continueGesture(id, seat, point, point);
}
static void startGesture(ProductID id, uint32_t seat, Point point) {
    startGesture(id, seat, point, point);
}
static void startGestureHelper(const ArrayList<Point>& points, int seat = 0, int steps = 0, Point offset = {0, 0}) {
    assert(points.size());
    startGesture(FAKE_DEVICE_ID, seat, points[0]*SCALE_FACTOR + offset);
    for(int i = 1; i < points.size(); i++) {
        if(steps) {
            auto delta = (points[i] - points[i - 1]);
            delta /= steps;
            Point p = points[i - 1];
            for(int s = 0; s < steps - 1; s++) {
                p += delta;
                continueGesture(FAKE_DEVICE_ID, seat, p * SCALE_FACTOR + offset);
            }
        }
        continueGesture(FAKE_DEVICE_ID, seat, points[i]*SCALE_FACTOR + offset);
    }
}
static const GestureEvent* endGestureHelper(int fingers = 1) {
    for(int n = 1; n < fingers; n++)
        assert(!endGesture(FAKE_DEVICE_ID, n));
    auto* event = endGesture(FAKE_DEVICE_ID, 0);
    return event;
}

Point lineDelta[] = {
    [GESTURE_NORTH_WEST] = {-1, -1},
    [GESTURE_WEST] = {-1, 0},
    [GESTURE_SOUTH_WEST] = {-1, 1},
    [3] = {},
    [GESTURE_NORTH] = {0, -1},
    [GESTURE_TAP] = {0, 0},
    [GESTURE_SOUTH] = {0, 1},
    [7] = {},
    [GESTURE_NORTH_EAST] = {1, -1},
    [GESTURE_EAST] = {1, 0},
    [GESTURE_SOUTH_EAST] = {1, 1},
};

struct GestureEventChecker {
    GestureKeyBinding detail;
    ArrayList<Point> points;
} gestureEventTuples[] = {
    {{GESTURE_NORTH_WEST, GESTURE_SOUTH_WEST}, {{2, 2}, {1, 1}, {0, 2}}},
    {{GESTURE_WEST}, {{1, 1}, {0, 1}}},
    {{GESTURE_SOUTH_WEST, GESTURE_SOUTH_EAST}, {{1, 1}, {0, 2}, {1, 3}}},
    {{GESTURE_NORTH}, {{1, 1}, {1, 0}}},
    {{GESTURE_TAP}, {{1, 1}}},
    {{GESTURE_TAP}, {{1, 1}, {1, 1}}},
    {{GESTURE_SOUTH}, {{1, 1}, {1, 2}}},
    {{GESTURE_NORTH_EAST, GESTURE_NORTH_WEST}, {{2, 2}, {3, 1}, {2, 0}}},
    {{GESTURE_EAST}, {{1, 1}, {2, 1}}},
    {{GESTURE_SOUTH_EAST, GESTURE_NORTH_EAST}, {{1, 1}, {2, 2}, {3, 1}}},
    {{GESTURE_EAST}, {{0, 0}, {1, 0}, {2, 0}, {3, 0}}},
};
MPX_TEST_ITER("validate_line_dir", LEN(gestureEventTuples) * 4, {
    int i = _i / 4;
    int fingers = _i % 2 + 1;
    bool cancel = _i % 4 > 2;
    auto& values = gestureEventTuples[i];
    assert(fingers);
    for(int n = 0; n < fingers; n++)
        startGestureHelper(values.points, n);
    if(cancel) {
        startGestureHelper(values.points, -1);
        cancelGesture(FAKE_DEVICE_ID, -1);
    }
    auto event = endGestureHelper(fingers);
    assert(event);
    assert(event->detail.size());
    assertEquals(event->detail, values.detail);
    assertEquals(event->flags.fingers, fingers);
    assert(!event->flags.reflectionMask);
});
GestureType getLineType(Point start, Point end);
static const ArrayList<Point> getPointsThatMakeLine(const GestureKeyBinding* detail) {
    assert(detail->size());
    Point p = {8, 8};
    ArrayList<Point>points = {p};
    for(auto type : detail->getTypes()) {
        points.add(p += lineDelta[type]);
        if(type != GESTURE_TAP)
            assertEquals(getLineType(points[points.size() - 2], p), type);
    }
    return points;
}
MPX_TEST_ITER("validate_masks", LEN(gestureEventTuples)*LEN(MASKS), {
    int index = _i / LEN(MASKS);
    auto mask = MASKS[_i % LEN(MASKS)];
    auto values = gestureEventTuples[index];
    GestureBinding binding = GestureBinding({values.detail}, {}, {.reflectionMask = mask});
    assertEquals(binding.getKeyBindings().size(), 2);
    startGestureHelper(getPointsThatMakeLine((GestureKeyBinding*)binding.getKeyBindings()[0]));
    startGestureHelper(getPointsThatMakeLine((GestureKeyBinding*)binding.getKeyBindings().back()), 1, 0, {500, 500});
    auto event = endGestureHelper(2);
    assert(!event->flags.reflectionMask || event->flags.reflectionMask & mask);
});

struct GestureMultiLineCheck {
    ArrayList<Point>points;
    GestureKeyBinding detail;
    int fingers = 1;
    int steps = 0;
} multiLines[] = {
    {
        {{0, 0}, {1, 0}, {1, 1}, {0, 1}, {0, 0}},
        {GESTURE_EAST, GESTURE_SOUTH, GESTURE_WEST, GESTURE_NORTH},
    },
    {
        {{1, 1}, {2, 2}, {3, 1}, {2, 0}, {1, 1}},
        {GESTURE_SOUTH_EAST, GESTURE_NORTH_EAST, GESTURE_NORTH_WEST, GESTURE_SOUTH_WEST},
    },
    {
        {{0, 0}, {1, 0}, {1, 1}},
        {GESTURE_EAST, GESTURE_SOUTH},
        .fingers = 2
    },
    {
        {{0, 0}, {1, 0}, {1, 1}, {0, 1}, {0, 0}},
        {GESTURE_EAST, GESTURE_SOUTH, GESTURE_WEST, GESTURE_NORTH},
        .fingers = 2
    },
    {
        {{0, 0}, {1, 0}, {1, 1}, {0, 1}, {0, 0}},
        {GESTURE_EAST, GESTURE_SOUTH, GESTURE_WEST, GESTURE_NORTH},
        .steps = 10
    },
    {
        {{0, 0}, {1, 0}, {1, 1}, {0, 1}, {0, 0}},
        {GESTURE_EAST, GESTURE_SOUTH, GESTURE_WEST, GESTURE_NORTH},
        10, 10
    },
    {
        {{0, 0}, {1, 0}, {2, 0}, {3, 0}, {3, 3}, {3, 9}},
        {GESTURE_EAST, GESTURE_SOUTH},
        10, 10
    },
};
MPX_TEST_ITER("multi_lines", LEN(multiLines) * 2, {
    auto& values = multiLines[_i / 2];
    int step = (_i % 2) * 10;
    for(int i = 0; i < values.fingers; i++)
        startGestureHelper(values.points, i, step);
    auto event = endGestureHelper(values.fingers);
    assert(!event->flags.reflectionMask);
    assertEquals(event->detail, values.detail);
});
struct GenericGestureCheck {
    ArrayList<ArrayList<Point>>points;
    GestureKeyBinding detail;

    int getFingers() {return points.size();}
    int steps = 0;
} genericGesture[] = {
    {
        {
            {{1, 1}, {1, 0}},
            {{1, 1}, {1, 2}},
            {{1, 1}, {2, 1}},
            {{1, 1}, {0, 1}},
        },
        {GESTURE_PINCH_OUT},
    },
    {
        {
            {{1, 0}, {1, 1}},
            {{1, 2}, {1, 1}},
            {{2, 1}, {1, 1}},
            {{0, 1}, {1, 1}},
        },
        {GESTURE_PINCH},
    },
    {
        {
            {{0, 1}, {1, 2}},
            {{2, 3}, {3, 4}},
            {{4, 5}, {5, 6}},
            {{6, 7}, {7, 0}},
        },
        {GESTURE_UNKNOWN},
    },
    {
        {
            {{1, 1}},
            {{0, 1}, {0, 2}, {2, 2}},
        },
        {GESTURE_UNKNOWN},
    },
};
MPX_TEST_ITER("generic_gesture", LEN(genericGesture), {
    auto& values = genericGesture[_i];
    for(int i = 0; i < values.getFingers(); i++)
        startGestureHelper(values.points[i], i);
    auto event = endGestureHelper(values.getFingers());
    assert(!event->flags.reflectionMask);
    assertEquals(event->detail, values.detail);
});

struct GenericGestureBindingCheck {
    ArrayList<ArrayList<Point>>points;
    ArrayList<GestureBinding> bindings;
    int getFingers() {return points.size();}
    int steps = 0;
} genericGestureBinding[] = {
    {
        {
            {{1, 1}, {2, 2}},
            {{2, 2}, {1, 1}},
        },
        {
            {GESTURE_NORTH_WEST, {}, {.fingers = 2, .reflectionMask = MirroredMask}},
            {GESTURE_SOUTH_EAST, {}, {.fingers = 2, .reflectionMask = MirroredMask}},
        },
    },
    {
        {
            {{0, 0}, {1, 0}, {1, 1}},
            {{8, 8}, {7, 8}, {7, 9}},
        },
        {
            {{GESTURE_EAST, GESTURE_SOUTH}, {}, {.fingers = 2, .reflectionMask = MirroredXMask}},
            {{GESTURE_WEST, GESTURE_SOUTH}, {}, {.fingers = 2, .reflectionMask = MirroredXMask}},
        },
    },
    {
        {
            {{1, 1}, {1, 0}, {0, 0}},
            {{7, 9}, {7, 8}, {8, 8}},
        },
        {
            {{GESTURE_NORTH, GESTURE_EAST}, {}, {.fingers = 2, .reflectionMask = MirroredXMask}},
            {{GESTURE_NORTH, GESTURE_WEST}, {}, {.fingers = 2, .reflectionMask = MirroredXMask}},
        },
    },
    {
        {
            {{0, 0}, {0, 1}, {1, 1}},
            {{8, 8}, {8, 7}, {9, 7}},
        },
        {
            {{GESTURE_SOUTH, GESTURE_EAST}, {}, {.fingers = 2, .reflectionMask = MirroredYMask}},
            {{GESTURE_NORTH, GESTURE_EAST}, {}, {.fingers = 2, .reflectionMask = MirroredYMask}},
        },
    },
};
MPX_TEST_ITER("generic_gesture_binding", LEN(genericGestureBinding), {
    auto& values = genericGestureBinding[_i];
    for(int i = 0; i < values.getFingers(); i++)
        startGestureHelper(values.points[i], i);
    auto event = endGestureHelper(values.getFingers());
    assert(event);
    INFO(values.bindings);
    for(auto binding : values.bindings)
        assert(binding.matches(*event));
});

SET_ENV(onSimpleStartup, fullCleanup);
GestureEvent event = {.detail = {GESTURE_TAP}};
struct GestureBindingEventMatching {
    GestureEvent event;
    GestureBinding binding;
    int count = 1;
} gestureBindingEventMatching [] {
    {{.detail = {GESTURE_TAP}}, {GESTURE_TAP, incrementCount}},
    {{.detail = {GESTURE_TAP}, .flags = {.count = 2}}, {GESTURE_TAP, incrementCount, {.count = 1}}, 0},
    {{.detail = {GESTURE_TAP}, .flags = {.count = 2}}, {GESTURE_TAP, incrementCount, {.count = 2}}},
    {{.detail = {GESTURE_TAP}}, {GESTURE_UNKNOWN, incrementCount}, 0},
};
MPX_TEST_ITER("gesture_matching", LEN(gestureBindingEventMatching), {
    auto& values = gestureBindingEventMatching[_i];
    const ArrayList<Binding*> list = {&values.binding};
    checkBindings(values.event, list);
    assertEquals(getCount(), values.count);
});

static void setup() {
    spawnThread(gestureEventLoop, "checkGestureEvents");
}

SET_ENV(setup, fullCleanup);
MPX_TEST("receive_shutdown", {
    GestureBinding wildcard({}, []{incrementCount();}, {.count = 1});
    getGestureBindings().add(&wildcard);
    startGestureHelper({{0, 0}}, 0);
    endGestureHelper(1);
    WAIT_UNTIL_TRUE(getCount() == 1);
    requestShutdown();
    waitForAllThreadsToExit();
});

MPX_TEST("double_tap", {
    GestureBinding wildcard({}, []{assert(getLastGestureEvent()); incrementCount(); requestShutdown();}, {.count = 2});
    GestureBinding badMatch({}, DEFAULT_EVENT(exitFailure), {.count = 1});
    getGestureBindings().add(&wildcard);
    getGestureBindings().add(&badMatch);

    for(int i = 0; i < 2; i++) {
        startGestureHelper({{0, 0}}, 0);
        endGestureHelper(1);
    }
    waitForAllThreadsToExit();
    assertEquals(getCount(), 1);
    getGestureBindings().clear();
});
MPX_TEST("differentiate_devices", {
    GestureBinding wildcard({}, incrementCount, {.count = 1});
    GestureBinding badMatch({}, DEFAULT_EVENT(exitFailure), {.count = 2});
    getGestureBindings().add(&wildcard);
    getGestureBindings().add(&badMatch);

    for(int i = 0; i < 2; i++) {
        startGesture(i, 0, {0, 0});
        endGesture(i, 0);
    }
    WAIT_UNTIL_TRUE(getCount() == 2);
    requestShutdown();
    waitForAllThreadsToExit();
    getGestureBindings().clear();
});


SET_ENV(NULL, fullCleanup);
MPX_TEST("gesture_regions", {

    static uint16_t dims[] = {100, 100};
    Rect bounds = {0, 0, dims[0], dims[1]};
    setRootDims(dims);

    GestureRegion leftNormal = GestureRegion({0, 0, 40, 20}, 0, 1);
    GestureRegion rightNegative = GestureRegion({-20, 0, 20, 0});
    GestureRegion topAbs = GestureRegion({0, 0, 0, 20});
    GestureRegion bottomSplit[] = {
        GestureRegion({0, -20, 33, 20}, 1, 0),
        GestureRegion({33, -20, 33, 20}, 1, 0),
        GestureRegion({66, -20, 33, 20}, 1, 0)
    };
    Point points[] = {{50, 0}, {0, 20}, {dims[0] - 10, 40}, {0, 99}, {50, 99}, {99, 99},
        {dims[0] / 2, dims[1] / 2}
    };
    assert(topAbs.contains(points[0], bounds));
    assert(leftNormal.contains(points[1], bounds));
    assert(rightNegative.contains(points[2], bounds));
    assert(bottomSplit[0].contains(points[3], bounds));
    assert(bottomSplit[1].contains(points[4], bounds));
    assert(bottomSplit[2].contains(points[5], bounds));

    getGestureRegions().add(topAbs);
    getGestureRegions().add(leftNormal);
    getGestureRegions().add(rightNegative);
    for(int i = 0; i < LEN(bottomSplit); i++)
        getGestureRegions().add(bottomSplit[i]);

    GestureBinding wildcard(GESTURE_TAP, incrementCount, {.count = 1});
    GestureBinding badMatch({}, DEFAULT_EVENT(exitFailure), {.count = 2});
    getGestureBindings().add(&wildcard);
    getGestureBindings().add(&badMatch);

    spawnThread(gestureEventLoop, "checkGestureEvents");

    listenForGestureEvents(GestureEndMask);
    for(int i = 0; i < LEN(points); i++)
        startGesture(0, i, points[i]);
    for(int i = 0; i < LEN(points); i++)
        endGesture(0, i);
    WAIT_UNTIL_TRUE(getGestureEventQueueSize() == 0);
    assertEquals(getCount(), LEN(points));
    requestShutdown();
    waitForAllThreadsToExit();
    getGestureBindings().clear();
});


MPX_TEST("bad-path", {
    setLogLevel(LOG_LEVEL_NONE);
    const char* path = "/dev/null";
    enableGestures(&path, 1, 1);
});
MPX_TEST("udev", {
    enableGestures();
});
