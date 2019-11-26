/**
 * @file windows.h
 * @brief Methods related to WindowInfo
 *
 */
#ifndef MPX_WINDOWS_H_
#define MPX_WINDOWS_H_

#include <string>

#include "globals.h"
#include "rect.h"
#include "mywm-structs.h"
#include "window-masks.h"


/**
 * @return a list of windows
 */
ArrayList<WindowInfo*>& getAllWindows();

///holds data on a window
struct WindowInfo : WMStruct, HasMask {
private:
    /**Window id used to determine changes in window layouts */
    const WindowID effectiveId;
    /// set to 1 iff the window is a dock
    bool dock = 0;
    /// 1 iff override_redirect flag set
    bool overrideRedirect = 0;
    /// i iff input only window
    bool inputOnly = 0;
    /// the window type was not set explicitly
    bool implicitType = 0;
    /// the window won't/cannot be fully managed
    bool notManageable = 0;
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
    /// the time the window was created or 0, if it existed before us
    TimeStamp creationTime = 0;
    /// events to listen for
    uint32_t eventMasks = NON_ROOT_EVENT_MASKS;
    /// device events to listen for
    uint32_t deviceEventMasks = NON_ROOT_DEVICE_EVENT_MASKS;


    /// Window gravity (TODO implemented)
    // char gravity = 0;

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
    /**
     * @param id unique X11 id the id of the window
     * @param parent WindowID of the parent (or 0 if you don't care)
     * @param effectiveID the id used to detect state changes; if 0, it will be id
     */
    WindowInfo(WindowID id, WindowID parent = 0, WindowID effectiveID = 0): WMStruct(id),
        effectiveId(effectiveID ? effectiveID : id), parent(parent) {}
    /**
     * Removes this window from Workspace & Master stack(s)
     */
    ~WindowInfo();
    /// the ID that should be used to detect tiling changes
    WindowID getEffectiveID()const {return effectiveId;}
    friend void loadWindowProperties(WindowInfo* winInfo);
    /// @param group sets the new window group
    void setGroup(WindowID group) {groupID = group;}
    /// @return the window group
    WindowID getGroup()const {return groupID;}
    /// @param win sets the new parent
    void setParent(WindowID win) {parent = win;}
    /// @return the parent of the window
    WindowID getParent()const {return parent;}


    /// @param win the window this is transient for
    void setTransientFor(WindowID win) {transientFor = win;}
    /// @return the window this is transient for or 0
    WindowID getTransientFor()const {return transientFor;}
    /// @return the type of the window
    uint32_t getType()const {return type;}
    /// @param t sets the new type of the window
    void setType(uint32_t t) {type = t;}
    /// @param str the name of the type of the window
    void setTypeName(std::string str) {typeName = str;}
    /// @return sets the new name of the type of the window
    std::string getTypeName()const {return typeName ;}

    /// @return the class of the window
    std::string getClassName()const {return className ;}
    /// @return the resource (instance) name of the window
    std::string getInstanceName()const {return instanceName ;}
    /// @param str sets the new title of the window
    void setTitle(std::string str) {title = str;}
    /// @return the title of the window
    std::string getTitle()const {return title ;}
    /**
     * @return true iff this window has the override_redirect bit set
     */
    bool isOverrideRedirectWindow()const {
        return overrideRedirect;
    };
    /**
     * Indicates that this window has the override_redirect bit set
     *
     */
    void markAsOverrideRedirect() {
        addMask(FLOATING_MASK | STICKY_MASK);
        overrideRedirect = 1;
    }
    /// @return 1 iff this is an input only window
    bool isInputOnly()const {
        return inputOnly;
    };
    /**
     * Marks the window as input only.
     * InputOnly windows cannot have a border
     */
    void markAsInputOnly() {
        inputOnly = 1;
        setTilingOverrideEnabled(1 << 4, 1);
        tilingOverride.border = 0;
    }
    /// sets a flag indicating the window cannot be managed
    void setNotManageable(bool b = 1) {notManageable = b;}
    /// returns whether or not a window can/should be managed
    /// If this returns false, then nothing should automatically happen to this window
    bool isNotManageable()const {return notManageable;}

    /// @return 1 if the window has any flags sets
    bool isSpecial() const {return isNotManageable() || isInputOnly() || isOverrideRedirectWindow() || isDock();}

    /// sets that the window type was not explicitly set
    void setImplicitType(bool b) {implicitType = b;}
    /// @return 1 iff window type was not explicitly set
    bool isImplicitType() {return implicitType;}

    /// @return the masks to grab
    uint32_t getEventMasks() {return eventMasks;}
    /// @param mask the new masks to grab
    void setEventMasks(uint32_t mask) {eventMasks = mask;}
    /// @param mask the new masks to grab in addition to the current masks
    void addEventMasks(uint32_t mask) {eventMasks |= mask;}
    /// @return the masks to grab
    uint32_t getDeviceEventMasks() {return deviceEventMasks;}
    /// @param mask the new masks to grab
    void setDeviceEventMasks(uint32_t mask) {deviceEventMasks = mask;}
    /// @param mask the new masks to grab in addition to the current masks
    void addDeviceEventMasks(uint32_t mask) {deviceEventMasks |= mask;}

    /**
     *
     *
     * @return the time the window was detected or 0 if it was detected before we started
     */
    TimeStamp getCreationTime() {return creationTime;}
    /**
     * Sets the creation time. Should only be done if we received a createEvent for this window
     *
     * @param t the detection time
     */
    void setCreationTime(TimeStamp t) {creationTime = t;}
    /// @return the last time a ping was received
    TimeStamp getPingTimeStamp() {return pingTimeStamp;}
    /// @param t sets the last time a ping was received
    void setPingTimeStamp(TimeStamp t) {pingTimeStamp = t;}
    /**
     * Enables/Disables tiling to be overridden at the indexes corresponding to mask
     *
     * @param mask
     * @param enable
     */
    void setTilingOverrideEnabled(unsigned int mask, bool enable = 1) {
        mask &= (1 << 5) - 1;
        if(enable)
            tilingOverrideEnabled |= mask;
        else
            tilingOverrideEnabled &= ~mask;
    }
    /**
     * Checks to see if tilling has been overridden at the given index
     *
     * @param index
     *
     * @return true iff tiling override is enabled
     */
    bool isTilingOverrideEnabledAtIndex(int index) const {
        return tilingOverrideEnabled & (1 << index);
    }
    /**
     * @return a bitmap indicating which parts of the geometry will be overridden
     */
    uint8_t getTilingOverrideMask()const {
        return tilingOverrideEnabled;
    }
    /**
     * Returns User set geometry that will override that generated when tiling
     * @see WindowInfo->config
     */
    const RectWithBorder& getTilingOverride()const {
        return tilingOverride;
    }
    /**
     * Sets window geometry that will replace that which is generated when tiling
     * @param tileOverride
     */
    void setTilingOverride(const RectWithBorder& tileOverride) {
        tilingOverride = tileOverride;
        if(inputOnly)
            tilingOverride.border = 0;
    }
    /**
     * Get the last recorded geometry of the window
     */
    const RectWithBorder& getGeometry()const {
        return geometry;
    }
    /**
     * Set the last recorded geometry of the window
     * @param geo
     */
    void setGeometry(const RectWithBorder geo) {
        geometry = geo;
    }

    /**
     *
     * @return true iff the window has been marked as a dock
     */
    bool isDock() const {
        return dock;
    }
    /**
     * Marks the window as a dock. Registered windows marked as docks will be used to resize monitor viewports
     */
    void setDock(bool value = true) {dock = value;}

    WindowMask hasPartOfMask(WindowMask mask)const override ;
    /**
     * @return 1 iff the window is PARTIALLY_VISIBLE or VISIBLE
     */
    bool isVisible() const {
        return hasMask(PARTIALLY_VISIBLE);
    }
    /**
     *
     * @return 1 iff the window supports being focused
     */
    bool isFocusAllowed() const {
        return !isNotManageable() && hasPartOfMask(INPUT_MASK | WM_TAKE_FOCUS_MASK) ? 1 : 0;
    }

    /**
     *
     * @return the window map state either UNMAPPED(0) or MAPPED(1)
     */
    bool isMappable() const {
        return hasMask(MAPPABLE_MASK);
    }
    /**
     * @return true if the user can interact (focus, type etc) with the window
     * For this to be true the window would have to be mapped and not hidden
     */
    bool isInteractable() const {
        return hasMask(MAPPED_MASK) && !hasMask(HIDDEN_MASK);
    }
    /**
     * Determines if a window should be tiled given its mapState and masks
     * @return true if the window should be tiled
     */
    bool isTileable()const {
        return hasMask(MAPPABLE_MASK) && !hasPartOfMask(ALL_NO_TILE_MASKS);
    }
    /**
     * A window is activatable if it is MAPPABLE and not HIDDEN and does not have the NO_ACTIVATE_MASK set
     * @return true if the window can receive focus
     */
    bool isActivatable() const {
        return !isNotManageable() && hasMask(MAPPABLE_MASK) && hasMask(INPUT_MASK) &&
            !hasPartOfMask(HIDDEN_MASK | NO_ACTIVATE_MASK);
    }


    /**
     * @return 1 iff external resize requests should be granted
     */
    bool isExternallyResizable() const {
        return hasMask(EXTERNAL_RESIZE_MASK) || !isMappable();
    }

    /**
     * @return 1 iff external move requests should be granted
     */
    bool isExternallyMoveable() const {
        return hasMask(EXTERNAL_MOVE_MASK) || !isMappable();
    }

    /**
     * @return 1 iff external border requests should be granted
     */
    bool isExternallyBorderConfigurable() const {
        return hasMask(EXTERNAL_BORDER_MASK) || !isMappable();
    }
    /**
     * @return 1 iff external raise requests should be granted
     */
    bool isExternallyRaisable() const {
        return hasMask(EXTERNAL_RAISE_MASK) || !isMappable();
    }
    /**
     * Checks to see if the window has SRC* masks set that will allow. If not client requests with such a source will be ignored
     * @param source
     * @return
     */
    bool allowRequestFromSource(int source) {
        return hasMask(1 << (source + SRC_INDICATION_OFFSET));
    }
    /**
     *
     * @return the workspace index the window is in or NO_WORKSPACE
     */
    WorkspaceID getWorkspaceIndex()const;
    /**
     * @return the workspace the window is in or NULL
     */
    Workspace* getWorkspace(void)const;
    /**
     * @return true if the window is not in a workspace or is in a visible one
     */
    bool isNotInInvisibleWorkspace()const;
    /**
     * Removes the window from its Workspace if it is in one
     */
    void removeFromWorkspace();
    /**
     * Moves the window to the specified Workspace.
     *
     * The window is first removed from any other Workspace.
     * If destIndex is NO_WORKPACE, the window gets STICK_MASK and is moved to the active workspace
     *
     * @param destIndex
     */
    void moveToWorkspace(WorkspaceID destIndex);

    /**
     *
     *
     * @param createNew if true a new array will be created if it doesn't exist
     *
     * @return size 12 array representing dock properties
     */
    int* getDockProperties(bool createNew = 0);
    /**
     * Add properties to winInfo that will be used to avoid docks
     * @param numberOfProperties number of properties
     * @param properties list of properties
     * @see xcb_ewmh_wm_strut_partial_t
     * @see xcb_ewmh_get_extents_reply_t
     * @see avoidStruct
     */
    void setDockProperties(int* properties, int numberOfProperties);
} ;

/**
 * Returns a struct with stored metadata on the given window
 * @param win
 * @return pointer to struct with info on the given window
 */
static inline WindowInfo* getWindowInfo(WindowID win) {
    return getAllWindows().find(win);
}
#endif
