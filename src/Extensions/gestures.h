/**
 * @file
 */
#ifndef MPX_EXT_GESUTRES_H_
#define MPX_EXT_GESUTRES_H_
#include "../time.h"
#include "../bindings.h"

/// The consecutive points within this distance are considered the same and not double counted
#define THRESHOLD_SQ (4)
/// All seat of a gesture have to start/end within this sq distance of each other
#define PINCH_THRESHOLD (256)
/// The cutoff for when a sequence of points forms a line
#define R_SQUARED_THRESHOLD .5

/// @{ GestureMasks
/// Triggered when a gesture group ends
#define GestureEndMask      (1 << 0)
/// triggered when a seat end
#define TouchEndMask        (1 << 1)
/// triggered when a seat starts
#define TouchStartMask      (1 << 2)
/// triggered when a  new point is added to a gesture
#define TouchHoldMask       (1 << 3)
/// triggered when a new point detected but not added to a gesture
#define TouchMotionMask     (1 << 4)
/// @}

/**
 * Controls what type(s) of gestures match a binding.
 * The Mirrored*Masks allow for the specified gesture or a reflection to match
 */
enum TransformMasks {
    /// Modifier used to indicate that a gesture was reflected across the X axis
    MirroredXMask = 1,
    /// Modifier used to indicate that a gesture was reflected across the Y axis
    MirroredYMask = 2,
    /// Modifier used to indicate that a gesture was reflected across the X and Y axis
    MirroredMask = 3,
    Rotate90Mask = 4,
    Rotate270Mask = 8,

};

/// ID of touch device
typedef uint32_t ProductID;
/// Groups gestures together; based on ProductID
typedef uint64_t GestureGroupID;
/// Id of a particular touch device based on ProductID and seat number
typedef uint64_t TouchID;

/**
 * Only Gesture with mask contained by mask will able to trigger events
 * By default all gestures are considered
 * @param mask
 */
void listenForGestureEvents(uint32_t mask);
/**
 * Types of gestures
 */
enum GestureType {
    /// @{ Gesture in a straight line
    GESTURE_NORTH_WEST = 0,
    GESTURE_WEST,
    GESTURE_SOUTH_WEST,
    GESTURE_NORTH = 4,
    GESTURE_TAP,
    GESTURE_SOUTH,
    GESTURE_NORTH_EAST = 8,
    GESTURE_EAST,
    GESTURE_SOUTH_EAST,
    /// @}

    /// Represents fingers coming together
    GESTURE_PINCH,
    /// Represents fingers coming apart
    GESTURE_PINCH_OUT,
    /// A gesture not defined by any of the above
    GESTURE_UNKNOWN,
};

/**
 * Returns the opposite direction of d
 * For example North -> South, SouthWest -> NorthEast
 * @param d
 * @return the opposite direction of d
 */
GestureType getOppositeDirection(GestureType d);
/**
 * Returns d flipped across the X axis
 * For exmaple North -> North, East -> West, SouthWest -> SouthEast
 * @param d
 * @return d reflected across the x axis
 */
GestureType getMirroredXDirection(GestureType d);
/**
 * Returns d flipped across the Y axis
 * For exmaple North -> South, East -> East, SouthWest -> NorthWest
 * @param d
 * @return d reflected across the x asis
 */
GestureType getMirroredYDirection(GestureType d);

/**
 * Returns d roated 90deg counter clockwise
 * For exmaple North -> West, West-> South, SouthWest -> SouthEast
 * @param d
 * @return d roated 90 deg
 */
GestureType getRot90Direction(GestureType d);

/**
 * Returns d roated 270deg counter clockwise
 * For exmaple North -> East, West-> North, SouthWest -> NorthWest
 * @param d
 * @return d roated 270 deg
 */
GestureType getRot270Direction(GestureType d);
/**
 * Transforms type according to mask
 *
 * @param mask
 * @param type
 *
 * @return
 */
static inline GestureType getReflection(TransformMasks mask, GestureType type) {
    return mask == MirroredXMask ? getMirroredXDirection(type)
        : mask == MirroredYMask ? getMirroredYDirection(type)
        : mask == MirroredMask ? getOppositeDirection(type)
        : mask == Rotate90Mask ? getRot90Direction(type)
        : mask == Rotate270Mask ? getRot270Direction(type)
        : GESTURE_UNKNOWN;
}


/**
 * Used to sub-divide a gesture device
 * The GestureRegion will be apart of the GestureID thus events starting in different regions
 * are independent of each other
 */
struct GestureRegion {
    /// global counter
    static uint32_t count;
    /// unique id
    const int id = ++count;
    /// Dimensions of the gesture region
    Rect base;
    /// whether to treat the x/y pos/dimensions as percents or absolutes
    bool percent[2];
    /**
     * @param base
     * @param percentX
     * @param percentY
     */
    GestureRegion(const Rect& base, bool percentX = 0, bool percentY = 0): base(base)  {
        percent[0] = percentX;
        percent[1] = percentY;
    }
    /**
     * @param p
     * @param relativeBounds total gesture device/screen size
     *
     * @return 1 iff p is contains by this gesture
     */
    bool contains(Point p, Rect relativeBounds) const {
        Rect temp = base;
        for(int i = 0; i < 2; i++) {
            if(percent[i]) {
                temp[i] *= relativeBounds[i + 2] / 100.0;
                temp[i + 2] *= relativeBounds[i + 2] / 100.0;
            }
        }
        return relativeBounds.getRelativeRegion(temp).contains(Rect(p.x, p.y, 0, 0));
    }
    /// @copydoc id
    int getID() const { return id; }
};
/**
 * @return list of gestureRegions
 */
ArrayList<GestureRegion>& getGestureRegions();

/**
 * Used to bind GestureEvents to functions
 */
struct GestureKeyBinding: KeyBinding {
    /// List of detected gestures
    ArrayList<GestureType> type;

    GestureKeyBinding(): KeyBinding(0, 0) {}
    /**
     * Constructs list with a single type
     * @param t
     */
    GestureKeyBinding(GestureType t): KeyBinding(0, 0), type({t}) {}
    /**
     * @param __l the list of GestureTypes
     */
    GestureKeyBinding(std::initializer_list<GestureType> __l): KeyBinding(0, 0), type(__l) {}
    /**
     * @param list the list of GestureTypes
     */
    GestureKeyBinding(const ArrayList<GestureType>& list): KeyBinding(0, 0), type(list) {}
    /// @return copydoc GestureKeyBinding::type
    const ArrayList<GestureType>& getTypes() const {return type;}

    /**
     * @param d
     * @return if both have the same type
     */
    bool operator ==(const GestureKeyBinding& d)const {return type == d.type;}
    /// If this gesture should match all others
    bool isWildcard() const {return !type.size();}
    /// The number of types
    uint32_t size()const {return type.size();}
};

/**
 * Simple struct used to check if a value is in range
 */
struct MinMax {
    /// min value
    uint32_t min;
    /// max value
    uint32_t max;
    /**
     * Contains exactly value
     *
     * @param value the new value of both min and max
     */
    MinMax(uint32_t value): min(value), max(value) {}
    /**
     * @param minValue
     * @param maxValue
     */
    MinMax(uint32_t minValue, uint32_t maxValue): min(minValue), max(maxValue) {}
    /**
     * @param value
     * @return 1 iff value is in [min, max]
     */
    bool contains(uint32_t value) const {
        return max == 0 || min <= value && value <= max;
    }
    /// @return max
    operator uint32_t () const { return max;}
    /**
     * @param value adds value to max
     * @return
     */
    MinMax& operator +=(uint32_t value) {
        max += value;
        return *this;
    }
    /**
     * @param value the amount to divide max by
     *
     * @return
     */
    MinMax& operator /=(uint32_t value) {
        max /= value;
        return *this;
    }
};


/// special flags specific to gestures
struct GestureFlags {
    /// Determines if more bindings will be processed if this one is triggered
    PassThrough passThrough = ALWAYS_PASSTHROUGH;
    /// The number of occurrences of this gesture
    /// The subsequent, time-sensitive repeat count of this event
    /// Identical events within GESTURE_MERGE_DELAY_TIME are combined and this count is incremented
    /// Only applies to events with the same id
    MinMax count = 1;
    /// The total distance traveled
    MinMax totalSqDistance = 0;
    /// The total displacement traveled averaged across all fingers
    MinMax avgSqDisplacement = 0;
    /// The total distance traveled averaged across all fingers
    MinMax avgSqDistance = 0;
    /// The time in ms since the first to last press for this gesture
    MinMax duration = 0;
    /// The number of fingers used in this gesture
    MinMax fingers = 1;
    /// @copydoc TransformMasks
    TransformMasks reflectionMask;
    /// If reflectionMask, instead of appending the transformation, it will replace this base (for GestureBindings)
    bool replaceWithTransform = 0;
    /// @copydoc GestureRegion::id
    uint32_t regionID = 0;

    /// GestureMask; matches events with a mask contained by this value
    uint32_t mask = GestureEndMask;
    /**
     * @param flags from an event
     * @return 1 iff this GestureBinding's flags are compatible with flags
     */
    bool matchesFlags(GestureFlags& flags) const;
    /// @return @copydoc passThrough
    PassThrough getPassThrough()const { return passThrough;}
    /// @return @copydoc regionID
    uint32_t getMode()const { return regionID;}
    /// @return @copydoc mask
    uint32_t getMask() const { return mask;}
};
/**
 * Gesture specific UserEvent
 */
struct GestureEvent: UserEvent {

    /// monotonically increasing number
    static uint32_t seqCounter;
    /// identifier use to potentially combine identical gestures
    const uint32_t seq = seqCounter++;
    /// identifier use to potentially combine identical gestures
    GestureGroupID id;
    /// Describes the gesture
    GestureKeyBinding detail;
    /// Used to match GestureBindings
    GestureFlags flags;
    /// the time this event was created
    TimeStamp time = getTime();
    /// The last point of the gesture
    Point endPoint;

    ~GestureEvent() = default;

    /// @return the gesture region id or 0
    uint32_t getRegionID() const { return id >> 32; }
    /**
     * @return The libinput device id
     */
    uint32_t getDeviceID() const { return (int)id ; }

    /**
     * @param event
     * @return 1 iff this event was created before event
     */
    bool operator<(GestureEvent& event) {
        return seq < event.seq;
    }
    /**
     * @param event
     * @return
     */
    bool operator==(GestureEvent& event) {
        return id == event.id && detail == event.detail && flags.fingers == event.flags.fingers &&
            flags.mask == event.flags.mask;
    }
    /// Sleeps until GESTURE_MERGE_DELAY_TIME ms have passed since creation
    void wait() const;
};

/**
 * Generates a list of gesture keybindings based on flags
 *
 * @param keyBinding
 * @param flags
 *
 * @return
 */
ArrayList<KeyBinding*> getKeyBindingList(const GestureKeyBinding& keyBinding, const GestureFlags& flags);

/**
 * Gesture specific bindings
 */
struct GestureBinding : Binding {
    /// special flags specific to gestures
    GestureFlags gestureFlags;

    /// GestureRegion::id
    int regionID = 0;
    /**
     * @param detail
     * @param boundFunction the function to call when triggered
     * @param gestureFlags special flags specific to gestures
     * @param name
     */
    GestureBinding(GestureKeyBinding detail, const BoundFunction boundFunction = {}, const GestureFlags gestureFlags = {},
        std::string name = ""): Binding(getKeyBindingList(detail, gestureFlags),
                boundFunction, {.noGrab = 1}, name), gestureFlags(gestureFlags) {
    }
    bool matches(const UserEvent& event) const override;
    virtual PassThrough getPassThrough()const {return gestureFlags.passThrough;}
};

/**
 * Should only be accessed from a GestureBinding that is being triggered/called
 *
 * @return the current GestureEvent
 */
GestureEvent* getLastGestureEvent(void);
/**
 * @param t
 * @return string representation of t
 */
const char* getGestureTypeString(GestureType t);
/**
 * @return A list of GestureBindings
 */
ArrayList<Binding*>& getGestureBindings();

/**
 * @param stream
 * @param type
 * @return
 */
static inline std::ostream& operator<<(std::ostream& stream, const GestureType& type) {
    return stream << getGestureTypeString(type);
}
/**
 * @param stream
 * @param detail
 * @return
 */
static inline std::ostream& operator<<(std::ostream& stream, const GestureKeyBinding& detail) {
    return stream << detail.type;
}

static inline std::ostream& operator>>(std::ostream& stream, const GestureFlags& flags) {
    return stream << "{ " << "mask:" << flags.mask << ", count: " << flags.count << ", sqDis: " << flags.totalSqDistance <<
        ", avgSqDisplacement: " << flags.avgSqDisplacement << ", avgSqDis: " << flags.avgSqDistance <<
        ", duration: " << flags.duration << " fingers: " << flags.fingers << ", Reflection: " << flags.reflectionMask;
}
static inline std::ostream& operator<<(std::ostream& stream, const MinMax& minMax) {
    return minMax.min == minMax.max ? stream << "[" << minMax.max << "]" :
        stream << "[" << minMax.min << ", " << minMax.max << "]";
}
static inline std::ostream& operator<<(std::ostream& stream, const GestureFlags& flags) {
    return stream << "{ " << "count: " << flags.count << ", sqDis: " << flags.totalSqDistance <<
        ", avgSqDisplacement: " << flags.avgSqDisplacement << ", avgSqDis: " << flags.avgSqDistance <<
        ", duration: " << flags.duration << " fingers: " << flags.fingers << ", Reflection: " << flags.reflectionMask;
}
/**
 * @param stream
 * @param binding
 * @return
 */
static inline std::ostream& operator<<(std::ostream& stream, const GestureBinding& binding) {
    return stream << "Detail: " << (ArrayList<GestureKeyBinding*>&)binding.getKeyBindings() << ", Flags: " <<
        binding.gestureFlags;
}
/**
 * @param stream
 * @param event
 * @return
 */
static inline std::ostream& operator<<(std::ostream& stream, const GestureEvent& event) {
    return stream << "{ #" << event.seq << " ID: " << event.getRegionID() << "," << event.getDeviceID() << ", Detail: " <<
        event.detail <<
        "Flags: " >> event.flags << ", Time: " << event.time << " LastPoint " << event.endPoint << "}";
}

/**
 * Starts a gesture
 *
 * @param id device id
 * @param seat seat number
 * @param startingPoint
 * @param lastPoint
 * @param sysName
 * @param name
 */
void startGesture(ProductID id, uint32_t seat, Point startingPoint, Point lastPoint, std::string sysName = "",
    std::string name = "");
/**
 * Adds a point to a gesture
 *
 * @param id
 * @param seat
 * @param point
 * @param lastPoint
 */
void continueGesture(ProductID id, uint32_t seat, Point point, Point lastPoint);
/**
 * Concludes a gestures and if all gestures in the GestureGroup are done, termines the group and returns a GestureEvent.
 * The event is already added to the queue for processing
 *
 * @param id
 * @param seat
 *
 * @return NULL of a gesture event if all gestures have completed
 */
const GestureEvent* endGesture(ProductID id, uint32_t seat);
/**
 * Aborts and ongoing gesture
 *
 * Does not generate any events. Already generated events are not affected
 *
 * @param id
 * @param seat
 */
void cancelGesture(ProductID id, uint32_t seat);

/**
 * Starting listening for and processing gestures.
 * One thread is spawned to listen for libinput TouchEvents and convert them into our custom GestureEvents
 * Another thread processes these GestureEvents and triggers GestureBindings
 *
 * @param paths only listen for these paths
 * @param num length of paths array
 * @param grab rather to get an exclusive grab on the device
 */
void enableGestures(const char** paths = NULL, int num = 0, bool grab = 0);
/**
 * Process GestureEvents
 */
void gestureEventLoop();

/**
 * @return the number of queued events
 */
int getGestureEventQueueSize();
#endif
