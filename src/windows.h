/**
 * @file windows.h
 * @brief Methods related to WindowInfo
 *
 */
#ifndef MPX_WINDOWS_H_
#define MPX_WINDOWS_H_

#include "globals.h"
#include "mywm-structs.h"
#include "util/rect.h"
#include "window-masks.h"
#include "workspaces.h"


///Types of docks
typedef enum DockType {DOCK_LEFT, DOCK_RIGHT, DOCK_TOP, DOCK_BOTTOM} DockType;
/// Holder for dock metadata
typedef struct DockProperties {
    /// Determines which edge of the screen the dock is for
    DockType type;
    /// How far the dock extends from the edge. If 0, the dock isn't active
    uint16_t thickness;
    /// offset into the specified edge where the dock starts
    uint16_t start;
    /// offset into the specified edge where the dock ends
    uint16_t end;
} DockProperties;

///holds data on a window
struct WindowInfo {
    const WindowID id;
    /// the parent of this window
    WindowID parent;
    /**
     * The window this window is a transient for
     */
    WindowID transientFor;
    /**
     * Window id of related window
     */
    WindowID groupID;
    /**
     * bitmap of window properties
     */
    WindowMask mask;
    WindowMask savedMask;
    /// set to 1 iff the window is a dock
    bool dock;
    /// 1 iff override_redirect flag set
    bool overrideRedirect;
    /// i iff input only window
    bool inputOnly;
    /// the window type was not set explicitly
    bool implicitType;
    /// the window won't/cannot be fully managed
    bool notManageable;
    /**xcb_atom representing the window type*/
    uint32_t type;

    /**string xcb_atom representing the window type*/
    char typeName[MAX_NAME_LEN];
    /**class name of window*/
    char className[MAX_NAME_LEN];
    /** instance name of window*/
    char instanceName[MAX_NAME_LEN];
    /**title of window*/
    char title[MAX_NAME_LEN];
    /** Application specified role of the window*/
    char role[MAX_NAME_LEN];

    ///time the window was last pinged
    TimeStamp pingTimeStamp;
    /// the time the window was created or 0, if it existed before us
    TimeStamp creationTime;
    /// events to listen for
    uint32_t eventMasks;
    /// device events to listen for
    uint32_t deviceEventMasks;


    /// Window gravity (TODO implemented)
    // char gravity;

    /// 5 bits each to enable 1 element of config and 5 bits for toggle absolute vs relative
    uint16_t tilingOverrideEnabled;
    /** Set to override tiling */
    Rect tilingOverride;
    uint16_t tilingOverrideBorder;
    uint8_t tilingOverridePercent;
    /** The last know size of the window */
    Rect geometry;
    DockProperties dockProperties;
};
static inline void setGeometry(WindowInfo* winInfo, const short* s) { winInfo->geometry = *(Rect*)s;}

/**
 * @param id unique X11 id the id of the window
 * @param parent WindowID of the parent (or 0 if you don't care)
 * @param effectiveID the id used to detect state changes; if 0, it will be id
 */
WindowInfo* newWindowInfo(WindowID id, WindowID parent);
/**
 * Removes this window from Workspace & Master stack(s)
 */
void freeWindowInfo(WindowInfo* winInfo);

static inline bool isOverrideRedirectWindow(const WindowInfo* winInfo) {return winInfo->overrideRedirect;};
static inline bool isInputOnlyWindow(const WindowInfo* winInfo) {return winInfo->inputOnly;};

/**
 * Enables/Disables tiling to be overridden at the indexes corresponding to mask
 *
 * @param mask
 * @param enable
 */
static inline void setTilingOverrideEnabled(WindowInfo* winInfo, unsigned int mask) {
    winInfo->tilingOverrideEnabled |= mask;
}
static inline void setTilingOverrideDisabled(WindowInfo* winInfo, unsigned int mask) {
    winInfo->tilingOverrideEnabled &= ~mask;
}
/**
 * Checks to see if tilling has been overridden at the given index
 *
 * @param index
 *
 * @return true iff tiling override is enabled
 */
static inline bool isTilingOverrideEnabledAtIndex(const WindowInfo* winInfo, int index) {
    return winInfo->tilingOverrideEnabled & (1 << index);
}

/**
 * @param index
 * @return true iff tiling override should be treated as relative instead of absolute
 */
static inline bool isTilingOverrideRelativeAtIndex(const WindowInfo* winInfo, int index) {
    return winInfo->tilingOverrideEnabled & (1 << (index + 5));
}
/**
 * @return a bitmap indicating which parts of the geometry will be overridden
 */
static inline uint8_t getTilingOverrideMask(const WindowInfo* winInfo) {
    return winInfo->tilingOverrideEnabled & 31;
}
/**
 * Returns User set geometry that will override that generated when tiling
 * @see WindowInfo->config
 */
static inline const Rect* getTilingOverride(const WindowInfo* winInfo) {
    return &winInfo->tilingOverride;
}
static inline uint8_t getTilingOverridePercent(const WindowInfo* winInfo) {
    return winInfo->tilingOverridePercent;
}
static inline uint16_t getTilingOverrideBorder(const WindowInfo* winInfo) {
    return winInfo->tilingOverrideBorder;
}
/**
 * Sets window geometry that will replace that which is generated when tiling
 * @param tileOverride
 */
static inline void setTilingOverride(WindowInfo* winInfo, const Rect tileOverride) {
    winInfo->tilingOverride = tileOverride;
}

static inline void setTilingOverrideBorder(WindowInfo* winInfo, uint16_t border) {
    winInfo->tilingOverrideBorder = border;
}
static inline void setTilingOverridePercent(WindowInfo* winInfo, uint8_t percent) {
    winInfo->tilingOverridePercent = percent;
}
/**
 * @return true if the window is not in a workspace or is in a visible one
 */
bool isNotInInvisibleWorkspace(const WindowInfo* winInfo);
/**
 * Removes the window from its Workspace if it is in one
 */
void removeFromWorkspace(WindowInfo* winInfo);
/**
 * Moves the window to the specified Workspace.
 *
 * The window is first removed from any other Workspace.
 * If destIndex is NO_WORKPACE, the window gets STICK_MASK and is moved to the active workspace
 *
 * @param destIndex
 */
void moveToWorkspace(WindowInfo* winInfo, WorkspaceID destIndex);

/**
 * @return size 12 array representing dock properties
 */
const DockProperties* getDockProperties(const WindowInfo* winInfo);
/**
 * Add properties to winInfo that will be used to avoid docks
 * @param properties list of properties
 * @param full whether properties is of size 4 of 12
 * @see xcb_ewmh_wm_strut_partial_t
 * @see xcb_ewmh_get_extents_reply_t
 * @see avoidStruct
 */
void setDockProperties(WindowInfo* winInfo, int* properties, bool partial);

WindowMask getEffectiveMask(const WindowInfo* winInfo);

/**
 * @param mask
 * @return the intersection of mask and the window mask
 */
static inline WindowMask hasPartOfMask(const WindowInfo* winInfo, WindowMask mask) {
    return getEffectiveMask(winInfo) & mask;
}

/**
 *
 * @param mask
 * @return 1 iff the window containers the complete mask
 */
static inline bool hasMask(const WindowInfo* winInfo, WindowMask mask) {
    return hasPartOfMask(winInfo, mask) == mask;
}

static inline bool hasOrHasNotMasks(const WindowInfo* winInfo, WindowMask has, WindowMask hasNot) {
    return !winInfo || hasMask(winInfo, has) || !hasPartOfMask(winInfo, hasNot);
}
static inline bool hasAndHasNotMasks(const WindowInfo* winInfo, WindowMask has, WindowMask hasNot) {
    return hasMask(winInfo, has) && !hasPartOfMask(winInfo, hasNot);
}

/**
 * Adds the states give by mask to the window
 * @param mask
 */
static inline void addMask(WindowInfo* winInfo, WindowMask mask) {
    winInfo->mask |= mask;
}
/**
 * Removes the states give by mask from the window
 * @param mask
 */
static inline void removeMask(WindowInfo* winInfo, WindowMask mask) {
    winInfo->mask &= ~mask;
}
/**
 * Adds or removes the mask depending if the window already contains
 * the complete mask
 * @param mask
 */
static inline void toggleMask(WindowInfo* winInfo, WindowMask mask) {
    if((winInfo->mask & mask) == mask)
        removeMask(winInfo, mask);
    else addMask(winInfo, mask);
}

/**
 * Determines if a window should be tiled given its mapState and masks
 * @return true if the window should be tiled
 */
static inline bool isTileable(const WindowInfo* winInfo) {
    return hasMask(winInfo, MAPPABLE_MASK) && !hasPartOfMask(winInfo, ALL_NO_TILE_MASKS);
}

static inline bool isVisible(const WindowInfo* winInfo) {
    return hasMask(winInfo, VISIBLE_MASK);
}

/**
 *
 * @return the window map state either UNMAPPED(0) or MAPPED(1)
 */
static inline bool isMappable(const WindowInfo* winInfo) {
    return hasMask(winInfo, MAPPABLE_MASK) && !hasMask(winInfo, HIDDEN_MASK);
}

static inline bool isActivatable(const WindowInfo* winInfo) {
    return hasAndHasNotMasks(winInfo, MAPPABLE_MASK | INPUT_MASK, NO_ACTIVATE_MASK);
}

/**
 * @return If the window can be focused
 */
static inline bool isFocusable(const WindowInfo* winInfo) {
    return hasPartOfMask(winInfo, FOCUSABLE_MASK);
}

/// @return the masks this window will sync with X
WindowMask getMasksToSync(WindowInfo* winInfo);

void saveAllWindowMasks();
#endif
