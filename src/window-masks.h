/**
 * @file window-masks.h
 *
 */
#ifndef WINDOW_MASKS_H_
#define WINDOW_MASKS_H_

#include <string>
#include "mywm-structs.h"


/**
 * Offset for SRC_INDICATION_* masks
 * These masks specify which type of external sources are allowed to modify the window
 * Adding [0,2] and raising 2 to that power yields OTHER,APP and PAGER SRC_INDICATION_ masks respectively
 */
#define SRC_INDICATION_OFFSET 12
/**
 * Various flags that detail how a window should be treated.
 *
 */
enum WindowMasks {
    /// no special properties
    NO_MASK = 0,
    /**The window's X size will equal the size of the monitor's viewport*/
    X_MAXIMIZED_MASK =      1 << 0,
    /**The window's Y size will equal the size of the monitor's viewport*/
    Y_MAXIMIZED_MASK =      1 << 1,
    /// X & Y MAXIMIZED MASK
    MAXIMIZED_MASK =     Y_MAXIMIZED_MASK | X_MAXIMIZED_MASK,
    /**The window's size will equal the size of the monitor (not viewport)     */
    FULLSCREEN_MASK =       1 << 2,
    /**The window's size will equal the size of the screen (union of all monitors)*/
    ROOT_FULLSCREEN_MASK =  1 << 3,

    ///windows will be below normal windows unless ABOVE_MASK is set is set
    BELOW_MASK =            1 << 4,
    ///windows will be above other windows
    ABOVE_MASK =            1 << 5,
    ///Window will not be tiled
    NO_TILE_MASK =          1 << 6,


    /**The window's size will equal the size of the monitor (not viewport)     */
    TRUE_FULLSCREEN_MASK =       FULLSCREEN_MASK | ABOVE_MASK,
    /**The window's size will equal the size of the screen (union of all monitors)*/
    TRUE_ROOT_FULLSCREEN_MASK =  ROOT_FULLSCREEN_MASK | ABOVE_MASK,

    ///The window will be treated as unmapped until this mask is removed
    HIDDEN_MASK =        1 << 7,

    /// or of all masks that will cause the layout function to skip a window
    ALL_NO_TILE_MASKS = FULLSCREEN_MASK | ROOT_FULLSCREEN_MASK | BELOW_MASK | ABOVE_MASK | NO_TILE_MASK | HIDDEN_MASK,
    USER_MASKS = ((1 << 8) - 1),

    /// Window can be externally resized (configure requests are allowed)
    EXTERNAL_RESIZE_MASK =  1 << 8,
    /// Window can be externally moved (configure requests are allowed)
    EXTERNAL_MOVE_MASK =    1 << 9,
    /// Window border size can be externally changed
    EXTERNAL_BORDER_MASK =    1 << 10,
    /// Window cannot be externally raised/lowered (configure requests are blocked)
    EXTERNAL_RAISE_MASK =   1 << 11,
    /// Window won't be tiled and can be freely moved like by external programs/mouse
    EXTERNAL_CONFIGURABLE_MASK =            EXTERNAL_RESIZE_MASK | EXTERNAL_MOVE_MASK | EXTERNAL_BORDER_MASK | EXTERNAL_RAISE_MASK,
    /// Window is floating; Not tiled, above other windows and can be freely moved like by external programs/mouse and is above other windows
    FLOATING_MASK =            NO_TILE_MASK | ABOVE_MASK | EXTERNAL_CONFIGURABLE_MASK,

    ///Allow client requests from older clients who don't specify what they are
    SRC_INDICATION_OTHER =   1 << SRC_INDICATION_OFFSET,
    ///Allow client requests from normal applications
    SRC_INDICATION_APP =     1 << (SRC_INDICATION_OFFSET + 1),
    ///Allow client requests from pagers
    SRC_INDICATION_PAGER =   1 << (SRC_INDICATION_OFFSET + 2),
    /// allow from any source
    SRC_ANY =                SRC_INDICATION_OTHER | SRC_INDICATION_APP | SRC_INDICATION_PAGER,


    /**The window can receive input focus*/
    INPUT_MASK =           1 << 15,

    /**The WM send a client message after focusing the window*/
    WM_TAKE_FOCUS_MASK =    1 << 16,

    /**The WM will not forcibly delete windows immediately but request the application dies*/
    WM_DELETE_WINDOW_MASK = 1 << 17,
    /**Used in conjunction with WM_DELETE_WINDOW_MASK to kill the window */
    WM_PING_MASK =          1 << 18,

    /**
     * Best effort will be made to place all windows with this mask above any other window without it.
     * One particular flaw the implementation is that if a window with this mask is lowered, it will not automatically be re-raised
     * This mask is implemented via a onWindowMove Rule
     */
    ALWAYS_ON_TOP = 1 << 20,
    /**
     * Best effort will be made to place all windows with this mask below any other window without it.
     * One particular flaw the implementation is that if a window with this mask is raised, it will not automatically be re-lowered
     * This mask is implemented via a onWindowMove Rule
     */
    ALWAYS_ON_BOTTOM = 1 << 21,
    //
    /// Will not update our internal focus representation when a window with this mask is focused
    /// Intended for Desktop windows
    NO_RECORD_FOCUS =      1 << 22,
    /// Windows with this mask will not be activatable
    /// Intended for Desktop windows
    NO_ACTIVATE_MASK =      1 << 23,

    /**Marks the window as urgent*/
    URGENT_MASK =           1 << 24,

    /// Window is effectively associated with its monitor instead of its workspace
    /// (it is move dded between workspaces to stay on its monitor)
    STICKY_MASK =   1 << 26,

    /// Workspace masks will be ignored
    IGNORE_WORKSPACE_MASKS_MASK = 1 << 27,

    ///Keeps track on the visibility state of the window
    PARTIALLY_VISIBLE =     1 << 28,
    ///Keeps track on the visibility state of the window
    FULLY_VISIBLE =         1 << 29 | PARTIALLY_VISIBLE,
    ///Indicates the window is not withdrawn
    MAPPABLE_MASK =           1 << 30,
    ///the window is currently mapped
    MAPPED_MASK =           1 << 31,

    RETILE_MASKS =           USER_MASKS | MAPPABLE_MASK,
    /// set all masks
    ALL_MASK =              -1

} ;

/// typeof WindowInfo::mask
// typedef unsigned int WindowMask;

/**
 * Contains an bitwise or of WindowMasks
 */
struct WindowMask {
    /// | of WindowMasks
    uint32_t mask;
    /**
     * @param m the starting mask
     */
    WindowMask(uint32_t m = 0): mask(m) {}
    /**
     * @return bit mask
     */
    operator uint32_t() const {return mask;}
    /**
     * @param m the bits to and to mask
     * @return
     */
    WindowMask& operator &=(uint32_t m) {mask &= m; return *this;}
    /**
     * @param m the bits to or with mask
     * @return
     */
    WindowMask& operator |=(uint32_t m) {mask |= m; return *this;}
    /**
     * @return a string representation of mask
     */
    operator std::string() const {
#define _PRINT_MASK(str) if( (str & M)||(str==0 &&M==0)){s+=#str " ";M&=~str;}
        uint32_t M = mask;
        std::string s = std::to_string(mask ? __builtin_popcount(mask) : 0) + ":";
        _PRINT_MASK(NO_MASK);
        _PRINT_MASK(MAXIMIZED_MASK);
        _PRINT_MASK(X_MAXIMIZED_MASK);
        _PRINT_MASK(Y_MAXIMIZED_MASK);
        _PRINT_MASK(FULLSCREEN_MASK);
        _PRINT_MASK(ROOT_FULLSCREEN_MASK);
        _PRINT_MASK(BELOW_MASK);
        _PRINT_MASK(ABOVE_MASK);
        _PRINT_MASK(NO_TILE_MASK);
        _PRINT_MASK(STICKY_MASK);
        _PRINT_MASK(HIDDEN_MASK);
        _PRINT_MASK(FLOATING_MASK);
        _PRINT_MASK(EXTERNAL_CONFIGURABLE_MASK);
        _PRINT_MASK(EXTERNAL_RESIZE_MASK);
        _PRINT_MASK(EXTERNAL_MOVE_MASK);
        _PRINT_MASK(EXTERNAL_BORDER_MASK);
        _PRINT_MASK(EXTERNAL_RAISE_MASK);
        _PRINT_MASK(SRC_ANY);
        _PRINT_MASK(SRC_INDICATION_OTHER);
        _PRINT_MASK(SRC_INDICATION_APP);
        _PRINT_MASK(SRC_INDICATION_PAGER);
        _PRINT_MASK(IGNORE_WORKSPACE_MASKS_MASK);
        _PRINT_MASK(INPUT_MASK);
        _PRINT_MASK(NO_RECORD_FOCUS);
        _PRINT_MASK(NO_ACTIVATE_MASK);
        _PRINT_MASK(WM_TAKE_FOCUS_MASK);
        _PRINT_MASK(WM_DELETE_WINDOW_MASK);
        _PRINT_MASK(WM_PING_MASK);
        _PRINT_MASK(ALWAYS_ON_TOP);
        _PRINT_MASK(ALWAYS_ON_BOTTOM);
        _PRINT_MASK(FULLY_VISIBLE);
        _PRINT_MASK(PARTIALLY_VISIBLE);
        _PRINT_MASK(MAPPABLE_MASK);
        _PRINT_MASK(MAPPED_MASK);
        _PRINT_MASK(URGENT_MASK);
        return s;
    }
};
/**
 * prints WindowMask
 *
 * @param stream
 * @param mask
 *
 * @return
 */
static inline std::ostream& operator<<(std::ostream& stream, const WindowMask& mask) {
    return stream << (std::string)mask;
}

/**
 * Abstract class for Workspace and WindowInfo which have WindowMasks
 */
struct HasMask {
    /**
     * bitmap of window properties
     */
    WindowMask mask = 0;
    virtual ~HasMask() = default;
    /**
     * @returns the full mask of the given window
     */
    WindowMask getMask(void)const {
        return mask;
    }
    /**
     * @param mask
     * @return the intersection of mask and the window mask
     */
    virtual WindowMask hasPartOfMask(WindowMask mask)const {
        return getMask()& mask;
    }

    /**
     *
     * @param mask
     * @return 1 iff the window containers the complete mask
     */
    bool hasMask(WindowMask mask) const {
        return hasPartOfMask(mask) == mask;
    }
    /**
     * Returns the subset of mask that refers to user set masks
     */
    WindowMask getUserMask() const {
        return hasPartOfMask(USER_MASKS);
    }

    /**
     * Resets the user controlled state to the defaults
     */
    void resetUserMask() {
        mask &= ~USER_MASKS;
    }

    /**
     * Adds the states give by mask to the window
     * @param mask
     */
    void addMask(WindowMask mask) {
        this->mask |= mask;
    }
    /**
     * Removes the states give by mask from the window
     * @param mask
     */
    void removeMask(WindowMask mask) {
        this->mask &= ~mask;
    }
    /**
     * Adds or removes the mask depending if the window already contains
     * the complete mask
     * @param mask
     */
    void toggleMask(WindowMask mask) {
        if((getMask() & mask) == mask)
            removeMask(mask);
        else addMask(mask);
    }
};
#endif
