#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <libinput.h>
#include <linux/input.h>
#include <math.h>
#include <list>
#include <stdbool.h>
#include <string.h>
#include <sys/poll.h>

#include "../logger.h"
#include "../ringbuffer.h"
#include "../threads.h"
#include "../time.h"
#include "../monitors.h"
#include "gestures.h"

#define SQUARE(X) ((X)*(X))
#define SQ_DIST(P1,P2) SQUARE(P1.x-P2.x)+SQUARE(P1.y-P2.y)

/// Used to determine if a line as a positive, 0 or negative slope
#define SIGN_THRESHOLD(X) ((X)>.5?1:(X)>=-.5?0:-1)

#define GESTURE_MERGE_DELAY_TIME 200

uint32_t GestureRegion::count = 0;

static ThreadSignaler signaler;
static RingBuffer<GestureEvent*> gestureQueue;
static RingBuffer<GestureEvent*> touchQueue;
static ArrayList<Binding*> gestureBindings;
ArrayList<Binding*>& getGestureBindings() {
    return gestureBindings;
}
static GestureEvent* lastEvent;

GestureEvent* getLastGestureEvent(void) {
    return lastEvent;
}
uint32_t GestureEvent::seqCounter = 0;

static uint32_t gestureSelectMask = -1;
void listenForGestureEvents(uint32_t mask) {
    gestureSelectMask = mask;
}
ReverseArrayList<GestureRegion> regions;

ArrayList<GestureRegion>& getGestureRegions() {
    return regions;
}

int getGestureEventQueueSize() {
    return gestureQueue.getSize();
}

static GestureEvent* enqueueEvent(GestureEvent* event) {
    VERBOSE("Generated Event: " << *event);
    assert(event);
    if(event->flags.mask & gestureSelectMask) {
        TRACE("Enqueued Event: " << *event);
        if(event->flags.mask & GestureEndMask)
            gestureQueue.push(event);
        else touchQueue.push(event);
    }
    else {
        delete event;
        return NULL;
    }
    signaler.signal();
    return event;
}

struct Gesture {
    ArrayList<Point> points;
    ArrayList<GestureType> info;
    Point lastPoint;
    TimeStamp start = getTime();
    GestureFlags flags;
    ArrayList<Point>& getPoints() {return points;}
    bool addPoint(Point point, bool first = 0);
};
struct GestureGroup {
    GestureGroupID id;
    std::string sysName = "";
    std::string name = "";
    std::unordered_map<uint32_t, Gesture>gestures;
    int activeCount = 0;
    GestureGroupID getID() { return id;}
    Gesture& getArbitraryGesture() {return gestures.begin()->second;}

    uint32_t size() {return gestures.size();}
    Gesture& operator[](int i) {
        return gestures[i];
    }
};

static std::unordered_map<GestureGroupID, GestureGroup*>gestureGroupIDToGestureGroup;
static std::unordered_map<TouchID, GestureGroup*>touchIDToGestureGroup;
static GestureGroupID generateID(ProductID id, Point startingPoint) {
    int regionID = 0;
    Rect base = {0, 0, getRootWidth(), getRootHeight()};
    for(auto& region : getGestureRegions())
        if(base.getRelativeRegion(region).contains(startingPoint)) {
            regionID = region.getID();
            break;
        }
    return (((uint64_t)regionID) << 32L) | ((uint64_t)id)  ;
}
static TouchID generateTouchID(ProductID id, uint32_t seat) {
    return ((uint64_t)id) << 32L | seat;
}
static GestureGroup& getGestureGroup(ProductID id, uint32_t seat) {
    assert(touchIDToGestureGroup[generateTouchID(id, seat)]);
    return *touchIDToGestureGroup[generateTouchID(id, seat)];
}
static void clearGesture(GestureGroup& group, uint32_t seat) {
    group.gestures.erase(seat);
}
static void clearGestureGroup(GestureGroup& group) {
    for(auto& keyValue : group.gestures)
        touchIDToGestureGroup.erase(generateTouchID(group.getID(), keyValue.first));
    auto id = group.getID();
    delete gestureGroupIDToGestureGroup[id];
    gestureGroupIDToGestureGroup.erase(id);
}
bool Gesture::addPoint(Point point, bool first) {
    if(!first) {
        Point lastPoint = getPoints().back();
        auto distance = SQ_DIST(lastPoint, point);
        if(distance < THRESHOLD_SQ)
            return 0;
        flags.totalSqDistance = flags.totalSqDistance + distance;
    }
    getPoints().add(point);
    return 1;
}

GestureType getOppositeDirection(GestureType d) {
    return (GestureType)(GESTURE_SOUTH_EAST - d);
}
GestureType getMirroredXDirection(GestureType d) {
    return (GestureType)(d - (d / 4 - 1) * GESTURE_NORTH_EAST);
}
GestureType getMirroredYDirection(GestureType d) {
    return (GestureType)((d % 4 - 1) * (-2) + d);
}

GestureType getRot90Direction(GestureType d) {
    int sign = (d / GESTURE_TAP * 2) - 1;
    int a = (d / 4 - 1);
    int odd = (d % 2);
    int b = (a * (2 + odd * 3));
    int c = (!a * -3 * sign);
    int e = (d - a * 3 == 5) * a * 6;
    GestureType result = (GestureType)(d - b - c  - e);
    return result;
}

GestureType getRot270Direction(GestureType d) {
    return getRot90Direction(getOppositeDirection(d));
}
ArrayList<KeyBinding*> getKeyBindingList(const GestureKeyBinding& keyBinding, const GestureFlags& flags) {
    ArrayList<KeyBinding*> result;
    if(!flags.replaceWithTransform)
        result.add(new GestureKeyBinding(keyBinding));
    if(flags.reflectionMask) {
        ArrayList<GestureType> mirror;
        for(auto type : keyBinding.getTypes())
            mirror.add(getReflection(flags.reflectionMask, type));
        result.add(new GestureKeyBinding(mirror));
    }
    return result;
}

const char* getGestureTypeString(GestureType t) {
    switch(t) {
        case GESTURE_NORTH_WEST:
            return "NORTH_WEST";
        case GESTURE_WEST:
            return "WEST";
        case GESTURE_SOUTH_WEST:
            return "SOUTH_WEST";
        case GESTURE_NORTH:
            return "NORTH";
        case GESTURE_SOUTH:
            return "SOUTH";
        case GESTURE_NORTH_EAST:
            return "NORTH_EAST";
        case GESTURE_EAST:
            return "EAST";
        case GESTURE_SOUTH_EAST:
            return "SOUTH_EAST";
        case GESTURE_PINCH:
            return "PINCH";
        case GESTURE_PINCH_OUT:
            return "PINCH_OUT";
        case GESTURE_TAP:
            return "TAP";
        default:
            return "UNKNOWN";
    }
}
static double getRSquared(const ArrayList<Point> points, int start = 0, int num = 0) {
    double sumX = 0, sumY = 0, sumXY = 0;
    double sumX2 = 0, sumY2 = 0;
    if(!num)
        num = points.size() - start;
    for(int i = start; i < num; i++) {
        sumX += points[i].x;
        sumY += points[i].y;
        sumXY += points[i].x * points[i].y;
        sumX2 += SQUARE(points[i].x);
        sumY2 += SQUARE(points[i].y);
    }
    double r2 = (1e-6 + SQUARE(num * sumXY - sumX * sumY)) / (1e-6 + (num * sumX2 - SQUARE(sumX)) * (num * sumY2 - SQUARE(
                    sumY)));
    TRACE("R2 " << r2 << " Num: " << num);
    return r2;
}
GestureType getLineType(Point start, Point end) {
    VERBOSE("Getting line between " << start << " and " << end);
    double dx = end.x - start.x, dy = end.y - start.y;
    double sum = sqrt(SQUARE(dx) + SQUARE(dy));
    return (GestureType)(((SIGN_THRESHOLD(dx / sum) + 1) << 2) | (SIGN_THRESHOLD(dy / sum) + 1));
}
void detectSubLines(Gesture* gesture) {
    ArrayList<GestureType>& info = gesture->info;
    ArrayList<Point>& points = gesture->points;
    GestureType lastDirection = GESTURE_TAP;
    for(int i = 1; i < points.size(); i++) {
        auto dir = getLineType(points[i - 1], points[i]);
        if(dir != GESTURE_TAP)
            if(lastDirection == dir) {
                assert(info.size());
            }
            else {
                info.add(dir);
                lastDirection = dir;
            }
    }
}
bool generatePinchEvent(GestureEvent* gestureEvent, GestureGroup& group) {
    if(group.size() > 1 && gestureEvent->flags.avgSqDisplacement > THRESHOLD_SQ) {
        Point avgEnd = {0, 0};
        Point avgStart = {0, 0};
        for(auto& keyValue : group.gestures) {
            Gesture& gesture = keyValue.second;
            avgEnd += gesture.getPoints().back();
            avgStart += gesture.getPoints()[0];
        }
        avgEnd /= group.size();
        avgStart /= group.size();
        double avgStartDis = 0;
        double avgEndDis = 0;
        for(auto& keyValue : group.gestures) {
            Gesture& gesture = keyValue.second;
            avgEndDis += SQ_DIST(gesture.getPoints().back(), avgEnd);;
            avgStartDis += SQ_DIST(gesture.getPoints()[0], avgStart);;
        }
        avgEndDis /= group.size();
        avgStartDis /= group.size();
        if(avgStartDis < PINCH_THRESHOLD && avgEndDis > PINCH_THRESHOLD)
            gestureEvent->detail = GESTURE_PINCH_OUT;
        else if(avgStartDis > PINCH_THRESHOLD && avgEndDis < PINCH_THRESHOLD)
            gestureEvent->detail = GESTURE_PINCH;
        else return 0;
        return 1;
    }
    return 0;
}
bool setReflectionMask(GestureEvent* gestureEvent, GestureGroup& group) {
    auto& gesture = group.getArbitraryGesture();
    int sameCount = 1;
    const TransformMasks reflectionMasks[] = {MirroredMask, MirroredXMask, MirroredYMask, Rotate90Mask};
    uint32_t reflectionCounts[LEN(reflectionMasks)] = {0};
    if(group.size() > 1) {
        for(auto& keyValue : group.gestures) {
            if(&keyValue.second != &gesture) {
                if(keyValue.second.info.size() != gesture.info.size())
                    break;
                else if(gesture.info == keyValue.second.info)
                    sameCount++;
                else {
                    for(int i = 0; i < gesture.info.size(); i++) {
                        reflectionCounts[0] += getOppositeDirection(gesture.info[i]) == keyValue.second.info[i];
                        reflectionCounts[1] += getMirroredXDirection(gesture.info[i]) == keyValue.second.info[i];
                        reflectionCounts[2] += getMirroredYDirection(gesture.info[i]) == keyValue.second.info[i];
                        reflectionCounts[3] += getRot90Direction(gesture.info[i]) == keyValue.second.info[i] ||
                            getRot270Direction(gesture.info[i]) == keyValue.second.info[i];
                    }
                }
            }
        }
    }
    if(sameCount == group.gestures.size()) {
        gestureEvent->detail = gesture.info;
        return 1;
    }
    for(int i = 0; i < LEN(reflectionMasks); i++) {
        if(sameCount + reflectionCounts[i] / gesture.info.size() == group.gestures.size()) {
            VERBOSE("Setting reflection masks" << reflectionMasks[i]);
            gestureEvent->flags.reflectionMask = reflectionMasks[i];
            gestureEvent->detail = gesture.info;
            return 1;
        }
    }
    return 0;
}
void setFlags(Gesture& g, GestureEvent* event) {
    g.flags.avgSqDisplacement = g.getPoints().size() > 1 ?
        SQ_DIST(g.getPoints()[g.getPoints().size() - 2], g.getPoints().back()) :
        0;
    g.flags.avgSqDistance = g.flags.totalSqDistance;
    g.flags.duration = event->time - g.start;
    event->flags.totalSqDistance = g.flags.totalSqDistance;
    event->flags.avgSqDisplacement  = g.flags.avgSqDisplacement;
    event->flags.avgSqDistance = g.flags.avgSqDistance;
    event->flags.duration = event->time - g.start;
}
void combineFlags(GestureGroup& group, GestureEvent* event) {
    uint32_t minStartTime = -1;
    for(auto& keyValue : group.gestures) {
        Gesture& g = keyValue.second;
        event->flags.avgSqDisplacement += g.flags.avgSqDisplacement;
        event->flags.avgSqDistance += g.flags.avgSqDistance;
        event->flags.totalSqDistance += g.flags.totalSqDistance;
        if(g.start < minStartTime)
            minStartTime  = g.start;
    }
    event->flags.avgSqDisplacement /= group.size();
    event->flags.avgSqDistance /= group.size();
    event->flags.duration = event->time - minStartTime;
}
GestureEvent* generateGestureEvent(GestureGroup& group, Gesture& g, uint32_t mask) {
    GestureEvent* gestureEvent = new GestureEvent({
        .id = group.getID(),
        .endPoint = g.lastPoint
    });
    gestureEvent->flags.mask = mask;
    gestureEvent->flags.fingers = group.size();
    if(mask == GestureEndMask) {
        combineFlags(group, gestureEvent);
        if(generatePinchEvent(gestureEvent, group)) {}
        else if(setReflectionMask(gestureEvent, group)) {}
        else {
            gestureEvent->detail = GESTURE_UNKNOWN;
            TRACE("Failed to find mask");
        }
        return gestureEvent;
    }
    else
        setFlags(g, gestureEvent);
    if(mask == TouchEndMask) {
        assert(g.info.size());
        gestureEvent->detail = g.info;
    }
    else if(g.getPoints().size() == 1)
        gestureEvent->detail = GESTURE_TAP;
    else
        gestureEvent->detail = getLineType(g.getPoints()[g.getPoints().size() - 2], g.getPoints().back());
    return gestureEvent;
}

void startGesture(ProductID id, uint32_t seat, Point startingPoint, Point pointInPixels, std::string sysName,
    std::string name) {
    auto gestureGroupID = generateID(id, startingPoint);
    if(!gestureGroupIDToGestureGroup[gestureGroupID]) {
        TRACE("Creating gesture group " << gestureGroupID << "(" << id << ") Seat " << seat);
        gestureGroupIDToGestureGroup[gestureGroupID] = new GestureGroup{
            .id = gestureGroupID,
            .sysName = sysName,
            .name = name,
        };
    }
    touchIDToGestureGroup[generateTouchID(id, seat)] = gestureGroupIDToGestureGroup[gestureGroupID];
    GestureGroup& group = getGestureGroup(id, seat);
    assert(!group.gestures.count(seat));
    group.activeCount++;
    assert(group[seat].getPoints().size() == 0);
    group[seat].addPoint(startingPoint, 1);
    group[seat].lastPoint = pointInPixels;
    enqueueEvent(generateGestureEvent(group, group[seat], TouchStartMask));
}
void continueGesture(ProductID id, uint32_t seat, Point point, Point pointInPixels) {
    VERBOSE("Continuing gesture" << id << " Seat " << (int)seat);
    GestureGroup& group = getGestureGroup(id, seat);
    bool newPoint = group[seat].addPoint(point, 0);
    group[seat].lastPoint = pointInPixels;
    enqueueEvent(generateGestureEvent(group, group[seat], newPoint ? TouchMotionMask : TouchHoldMask));
}

void cancelGesture(ProductID id, uint32_t seat) {
    VERBOSE("Cancelling gesture" << id << " Seat " << (int)seat);
    GestureGroup& group = getGestureGroup(id, seat);
    assert(group.gestures.count(seat));
    assert(group.size());
    group.activeCount--;
    clearGesture(group, seat);
}

const GestureEvent* endGesture(ProductID id, uint32_t seat) {
    VERBOSE("Ending gesture" << id << " Seat " << (int)seat);
    GestureGroup& group = getGestureGroup(id, seat);
    assert(group.gestures.count(seat));
    Gesture* g = &group[seat];
    assert(g->info.size() == 0 && g->getPoints().size());
    if(g->getPoints().size() == 1)
        g->info.add(GESTURE_TAP);
    else if(getRSquared(g->points) > R_SQUARED_THRESHOLD) {
        VERBOSE("Find Single straight line" << id << " Seat " << (int)seat << " " << g->getPoints().size());
        g->info.add(getLineType(g->getPoints()[0], g->getPoints().back()));
    }
    else {
        detectSubLines(g);
        TRACE(getRSquared(g->getPoints()));
    }
    enqueueEvent(generateGestureEvent(group, *g, TouchEndMask));
    assert(group.activeCount);
    if(--group.activeCount == 0) {
        auto event = enqueueEvent(generateGestureEvent(group, *g, GestureEndMask));
        clearGestureGroup(group);
        return event;
    }
    return NULL;
}

void gestureEventLoop() {
    INFO("Staring GestureProcessor");
    if(!getGestureBindings().size())
        WARN("No bindings gestures are setup");
    while(!isShuttingDown()) {
        if(gestureQueue.isEmpty() && touchQueue.isEmpty()) {
            signaler.justWait();
        }
        while(!isShuttingDown() && (!gestureQueue.isEmpty() || !touchQueue.isEmpty())) {
#ifndef NDEBUG
            lock();
#endif
            GestureEvent* event;
            if(touchQueue.isEmpty() || !gestureQueue.isEmpty() && *gestureQueue.peek() < *touchQueue.peek()) {
                event = gestureQueue.pop();
                event->wait();
                while(!gestureQueue.isEmpty()) {
                    VERBOSE("Trying to combine gesture with " << *gestureQueue.peek());
                    auto delta = gestureQueue.peek()->time - event->time;
                    if(*event == *gestureQueue.peek() && delta <  GESTURE_MERGE_DELAY_TIME) {
                        auto* temp = gestureQueue.pop();
                        TRACE("Combining event " << *event << " with " << *temp << " Delta " << delta);
                        temp->wait();
                        event->flags.count += 1;
                        delete temp;
                        continue;
                    }
                    TRACE("Did not combine events Delta " << delta);
                    break;
                }
            }
            else
                event = touchQueue.pop();
#ifndef NDEBUG
            unlock();
#endif
            lock();
            assert(event);
            lastEvent = event;
            DEBUG("GestureEvent " << *event);
            pushContext("GestureEvent");
            VERBOSE("QueueSizes Gesture:" << gestureQueue.getSize() << " Touch:" << touchQueue.getSize());
            checkBindings(*event, getGestureBindings());
            popContext();
            lastEvent = NULL;
            unlock();
            delete event;
        }
    }
    INFO("Ending Thread");
}

bool GestureFlags::matchesFlags(GestureFlags& flags) const {
    return avgSqDistance.contains(flags.avgSqDistance) &&
        count.contains(flags.count) &&
        duration.contains(flags.duration) &&
        fingers.contains(flags.fingers) &&
        totalSqDistance.contains(flags.totalSqDistance) &&
        (getMask() == 0 || (getMask() & flags.getMask()) == flags.getMask()) &&
        (getMode() == ANY_MODE || flags.getMode() == ANY_MODE || flags.getMode() == getMode()) &&
        reflectionMask == flags.reflectionMask;
}
bool GestureBinding::matches(const UserEvent& userEvent) const {
    auto& event = (GestureEvent&)userEvent;
    VERBOSE("Trying to match Event " << event << " with " << *this);
    if(gestureFlags.matchesFlags(event.flags))
        for(auto* k : getKeyBindings()) {
            auto* key = (GestureKeyBinding*)k;
            if((key->isWildcard() || *key == event.detail))
                return 1;
        }
    return 0;
}

void GestureEvent::wait() const {
    uint32_t delta = getTime() - time;
    if(delta < GESTURE_MERGE_DELAY_TIME)
        msleep(GESTURE_MERGE_DELAY_TIME - delta);
}
