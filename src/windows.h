/**
 * @file windows.h
 * @brief Methods related to WindowInfo
 *
 */
#ifndef WINDOWS_H_
#define WINDOWS_H_

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>

#include "globals.h"
#include "rect.h"
#include "mywm-structs.h"
#include "window-masks.h"
#include <string>


/**
 * @return a list of windows
 */
ArrayList<WindowInfo*>& getAllWindows();

///holds data on a window
typedef struct WindowInfo : WMStruct {
private:
    /**Window id used to determine changes in window layouts */
    const WindowID effectiveId;
    /**Window mask */
    WindowMask mask = 0;
    /// set to 1 iff the window is a dock
    bool dock = 0;
    /// 1 iff override_redirect flag set
    bool overrideRedirect = 0;
    /**xcb_atom representing the window type*/
    uint32_t type = 0;

    /**string xcb_atom representing the window type*/
    std::string typeName = "";
    /**class name of window*/
    std::string className = "";
    /** instance name of window*/
    std::string instanceName = "";
    /**title of window*/
    std::string title = "";

    ///time the window was last pinged
    TimeStamp pingTimeStamp = 0;
    TimeStamp creationTime = 0;
    uint32_t eventMasks = NON_ROOT_EVENT_MASKS;


    ///Window gravity
    char gravity = 0;
    /// 5 bits each to enable 1 element of config
    unsigned char tilingOverrideEnabled = 0;
    /** Set to override tiling */
    RectWithBorder tilingOverride = {};
    /** The last know size of the window */
    RectWithBorder geometry = {};

    /**
     * The window this window is a transient for
     */
    WindowID transientFor = 0;
    /**
     * Window id of related window
     */
    WindowID groupID = 0;
    /// the parent of this window
    WindowID parent = 0;

public:
    WindowInfo(WindowID id, WindowID parent = 0, WindowID effectiveID = 0): WMStruct(id),
        effectiveId(effectiveID ? effectiveID : id), parent(parent) {}
    ~WindowInfo();
    WindowID getEffectiveID()const {return effectiveId;}
    friend void loadWindowProperties(WindowInfo* winInfo);
    void setGroup(WindowID group) {groupID = group;}
    WindowID getGroup()const {return groupID;}
    void setParent(WindowID win) {parent = win;}
    WindowID getParent()const {return parent;}
    void setTransientFor(WindowID win) {transientFor = win;}
    WindowID getTransientFor()const {return transientFor;}
    uint32_t getType()const {return type;}
    void setType(uint32_t t) {type = t;}
    void setTypeName(std::string str) {typeName = str;}
    std::string getTypeName()const {return typeName ;}
    void setClassName(std::string str) {className = str;}
    std::string getClassName()const {return className ;}
    void setInstanceName(std::string str) {instanceName = str;}
    std::string getInstanceName()const {return instanceName ;}
    void setTitle(std::string str) {title = str;}
    std::string getTitle()const {return title ;}
    /**
     *
     * @param winInfo
     *
     * @return true iff this window has the override_redirect bit set
     */
    bool isOverrideRedirectWindow()const {
        return overrideRedirect;
    };
    /**
     * Indicates that this window has the override_redirect bit set
     *
     * @param winInfo
     */
    void markAsOverrideRedirect() {
        addMask(FLOATING_MASK | STICKY_MASK);
        overrideRedirect = 1;
    }
    /**
     * @param winInfo
     * @returns the full mask of the given window
     */
    WindowMask getMask(void)const {
        return mask;
    }
    /**
     *
     * @param winInfo
     * @param mask
     * @return the intersection of mask and the window mask
     */
    WindowMask hasPartOfMask(WindowMask mask)const;

    /**
     *
     * @param mask
     * @return 1 iff the window containers the complete mask
     */
    bool hasMask(WindowMask mask)const;
    /**
     * Returns the subset of mask that refers to user set masks
     */
    WindowMask getUserMask() {
        return getMask()&USER_MASKS;
    }

    /**
     * Resets the user controlled state to the defaults
     * @param winInfo
     */
    void resetUserMask() {
        mask &= ~USER_MASKS;
    }

    /**
     * Adds the states give by mask to the window
     * @param winInfo
     * @param mask
     */
    void addMask(WindowMask mask) {
        this->mask |= mask;
    }
    /**
     * Removes the states give by mask from the window
     * @param winInfo
     * @param mask
     */
    void removeMask(WindowMask mask) {
        this->mask &= ~mask;
    }
    /**
     * Adds or removes the mask depending if the window already contains
     * the complete mask
     * @param winInfo
     * @param mask
     */
    void toggleMask(WindowMask mask);

    uint32_t getEventMasks() {return eventMasks;}
    void setEventMasks(uint32_t mask) {eventMasks = mask;}
    void addEventMasks(uint32_t mask) {eventMasks |= mask;}

    TimeStamp getCreationTime() {return creationTime;}
    void setCreationTime(TimeStamp t) {creationTime = t;}
    TimeStamp getPingTimeStamp() {return pingTimeStamp;}
    void setPingTimeStamp(TimeStamp t) {pingTimeStamp = t;}
    /**
     * Enables/Disables tiling to be overridden at the indexes corresponding to mask
     *
     * @param winInfo
     * @param mask
     * @param enable
     */
    void setTilingOverrideEnabled(unsigned int mask, bool enable = 1) {
        if(enable)
            tilingOverrideEnabled |= mask;
        else
            tilingOverrideEnabled &= ~mask;
    }
    /**
     * Checks to see if tilling has been overridden at the given index
     *
     * @param winInfo
     * @param index
     *
     * @return true iff tiling override is enabled
     */
    bool isTilingOverrideEnabledAtIndex(int index) {
        return tilingOverrideEnabled & (1 << index);
    }
    uint8_t getTilingOverrideMask()const {
        return tilingOverrideEnabled;
    }
    /**
     * Returns User set geometry that will override that generated when tiling
     * @param winInfo
     * @see WindowInfo->config
     */
    const RectWithBorder& getTilingOverride()const {
        return tilingOverride;
    }
    /**
     * Sets window geometry that will replace that which is generated when tiling
     * @param winInfo
     * @param geometry
     */
    void setTilingOverride(const RectWithBorder tileOverride) {
        tilingOverride = tileOverride;
    }
    /**
     * Get the last recorded geometry of the window
     */
    const RectWithBorder& getGeometry()const {
        return geometry;
    }
    /**
     * Set the last recorded geometry of the window
     * @param geometry
     */
    void setGeometry(const RectWithBorder geo) {
        geometry = geo;
    }

    /**
     * @param winInfo
     *
     * @return true iff the window has been marked as a dock
     */
    bool isDock() const {
        return dock;
    }
    /**
     * Marks the window as a dock. Registered windows marked as docks will be used to resize monitor viewports
     * @param winInfo
     */
    void setDock(bool value = true) {dock = value;}
    /**
     * @param winInfo
     * @return 1 iff the window is PARTIALLY_VISIBLE or VISIBLE
     */
    bool isVisible() const {
        return hasMask(PARTIALLY_VISIBLE);
    }

    /**
     *
     * @param winInfo the window whose map state to query
     * @return the window map state either UNMAPPED(0) or MAPPED(1)
     */
    bool isMappable() const {
        return hasMask(MAPPABLE_MASK);
    }
    /**
     * @param winInfo
     * @return true if the user can interact (focus,type etc)  with the window
     * For this to be true the window would have to be mapped and not hidden
     */
    bool isInteractable() const {
        return hasMask(MAPPED_MASK) && !hasMask(HIDDEN_MASK);
    }
    /**
     * Determines if a window should be tiled given its mapState and masks
     * @param winInfo
     * @return true if the window should be tiled
     */
    bool isTileable()const {
        return hasMask(MAPPABLE_MASK) && !hasPartOfMask(ALL_NO_TILE_MASKS);
    }
    /**
     * A window is activatable if it is MAPPABLE and not HIDDEN  and does not have the NO_ACTIVATE_MASK set
     * @param winInfo
     * @return true if the window can receive focus
     */
    bool isActivatable() const {
        return  hasMask(MAPPABLE_MASK | INPUT_MASK) &&
                !hasPartOfMask(HIDDEN_MASK | NO_ACTIVATE_MASK);
    }


    /**
     * @param winInfo
     * @return 1 iff external resize requests should be granted
     */
    bool isExternallyResizable() const {
        return  hasMask(EXTERNAL_RESIZE_MASK) || !isMappable();
    }

    /**
     * @param winInfo
     * @return 1 iff external move requests should be granted
     */
    bool isExternallyMoveable() const {
        return  hasMask(EXTERNAL_MOVE_MASK) || !isMappable();
    }

    /**
     * @param winInfo
     * @return 1 iff external border requests should be granted
     */
    bool isExternallyBorderConfigurable() const {
        return  hasMask(EXTERNAL_BORDER_MASK) || !isMappable();
    }
    /**
     * @param winInfo
     * @return 1 iff external raise requests should be granted
     */
    bool isExternallyRaisable() const {
        return  hasMask(EXTERNAL_RAISE_MASK) || !isMappable();
    }
    /**
     * Checks to see if the window has SRC* masks set that will allow. If not client requests with such a source will be ignored
     * @param winInfo
     * @param source
     * @return
     */
    bool allowRequestFromSource(int source) {
        return  hasMask(1 << (source + SRC_INDICATION_OFFSET));
    }
    /**
     * @param winInfo
     *
     * @return the workspace index the window is in or NO_WORKSPACE
     */
    WorkspaceID getWorkspaceIndex()const;
    /**
     * @param winInfo a window that is in a workspace
     * @return the workspace the window is in or NULL
     */
    Workspace* getWorkspace(void)const;
    bool isNotInInvisibleWorkspace()const;
    bool removeFromWorkspace();
    void moveToWorkspace(WorkspaceID destIndex);

    int* getDockProperties(bool createNew = 0);
    /**
     * Add properties to winInfo that will be used to avoid docks
     * @param winInfo the window in question
     * @param numberofProperties number of properties
     * @param properties list of properties
     * @see xcb_ewmh_wm_strut_partial_t
     * @see xcb_ewmh_get_extents_reply_t
     * @see avoidStruct
     */
    void setDockProperties(int* properties, int numberofProperties);
    bool matches(std::string str);
} WindowInfo;


static inline bool removeWindow(WindowID win) {
    WindowInfo* winInfo = getAllWindows().find(win);
    if(winInfo)
        delete getAllWindows().removeElement(winInfo);
    return winInfo ? 1 : 0;
}

/**
 * Returns a struct with stored metadata on the given window
 * @param win
 * @return pointer to struct with info on the given window
 */
static inline WindowInfo* getWindowInfo(WindowID win) {
    return getAllWindows().find(win);
}
#endif
