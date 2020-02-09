/**
 * @file window-masks.h
 *
 */
#ifndef WINDOW_MASKS_H_
#define WINDOW_MASKS_H_

#include <string>
#include <assert.h>
#include "mywm-structs.h"

/// Max number of window masks
#define NUM_WINDOW_MASKS  32

/// no special properties
#define NO_MASK 	(0)
/**The window's X size will equal the size of the monitor's viewport*/
#define X_MAXIMIZED_MASK 	(1U << 0)
/**The window's Y size will equal the size of the monitor's viewport*/
#define Y_MAXIMIZED_MASK 	(1U << 1)
/// X & Y MAXIMIZED MASK
#define MAXIMIZED_MASK 	(Y_MAXIMIZED_MASK | X_MAXIMIZED_MASK)
/**The window's size will equal the size of the monitor (not viewport)     */
#define FULLSCREEN_MASK 	(1U << 2)
/**The window's size will equal the size of the screen (union of all monitors)*/
#define ROOT_FULLSCREEN_MASK 	(1U << 3)

///windows will be below normal windows unless ABOVE_MASK is set is set
#define BELOW_MASK 	(1U << 4)
///windows will be above other windows
#define ABOVE_MASK 	(1U << 5)
///Window will not be tiled
#define NO_TILE_MASK 	(1U << 6)

///The window will be treated as unmapped until this mask is removed
#define HIDDEN_MASK 	(1U << 7)

/// or of all masks that will cause the layout function to skip a window
#define ALL_NO_TILE_MASKS 	(FULLSCREEN_MASK | ROOT_FULLSCREEN_MASK | BELOW_MASK | ABOVE_MASK | NO_TILE_MASK | HIDDEN_MASK)


/**Marks the window as urgent*/
#define URGENT_MASK 	(1U << 8)
/**
 * Window is effectively associated with its monitor instead of its workspace
 * (it is move added between workspaces to stay on its monitor)
 */
#define STICKY_MASK 	(1U << 9)

/**
 * Best effort will be made to place all windows with this mask above any other window without it.
 * One particular flaw the implementation is that if a window with this mask is lowered, it will not automatically be re-raised
 * This mask is implemented via a WINDOW_MOVE Rule
 */
#define ALWAYS_ON_TOP_MASK 	(1U << 10)
/**
 * Best effort will be made to place all windows with this mask below any other window without it.
 * One particular flaw the implementation is that if a window with this mask is raised, it will not automatically be re-lowered
 * This mask is implemented via a WINDOW_MOVE Rule
 */
#define ALWAYS_ON_BOTTOM_MASK 	(1U << 11)
/// will cause WindowInfo::isSpecial to return 1
#define SPECIAL_MASK 	(1U << 12)

/// Will not update our internal focus representation when a window with this mask is focused
/// Intended for Desktop windows
#define NO_RECORD_FOCUS_MASK 	(1U << 16)
/// Windows with this mask will not be activatable
/// Intended for Desktop windows
#define NO_ACTIVATE_MASK 	(1U << 17)

/// if the window is not in a workspace it should be positioned relative to the primary monitor if anny
/// if the window is in a workspace it will be move should be moved to always be in the workspace of the primary monitor if any
#define PRIMARY_MONITOR_MASK 	(1U << 18)

/// Workspace masks will be ignored
#define IGNORE_WORKSPACE_MASKS_MASK 	(1U << 19)

/// Window can be externally resized (configure requests are allowed)
#define EXTERNAL_RESIZE_MASK 	(1U << 20)
/// Window can be externally moved (configure requests are allowed)
#define EXTERNAL_MOVE_MASK 	(1U << 21)
/// Window border size can be externally changed
#define EXTERNAL_BORDER_MASK 	(1U << 22)
/// Window cannot be externally raised/lowered (configure requests are blocked)
#define EXTERNAL_RAISE_MASK 	(1U << 23)
/// Window won't be tiled and can be freely moved like by external programs/mouse
#define EXTERNAL_CONFIGURABLE_MASK 	(EXTERNAL_RESIZE_MASK | EXTERNAL_MOVE_MASK | EXTERNAL_BORDER_MASK | EXTERNAL_RAISE_MASK)
/// Window is floating; Not tiled, above other windows and can be freely moved like by external programs/mouse and is above other windows
#define FLOATING_MASK 	(NO_TILE_MASK | ABOVE_MASK | EXTERNAL_CONFIGURABLE_MASK)

/**The window can receive input focus*/
#define INPUT_MASK 	(1U << 24)

/**The WM send a client message after focusing the window*/
#define WM_TAKE_FOCUS_MASK 	(1U << 25)

/**The WM will not forcibly delete windows immediately but request the application dies*/
#define WM_DELETE_WINDOW_MASK 	(1U << 26)
/**Used in conjunction with WM_DELETE_WINDOW_MASK to kill the window */
#define WM_PING_MASK 	(1U << 27)

///Keeps track on the visibility state of the window
#define PARTIALLY_VISIBLE_MASK 	(1U << 28)
///Keeps track on the visibility state of the window
#define FULLY_VISIBLE_MASK 	(1U << 29 | PARTIALLY_VISIBLE_MASK)
///Indicates the window is not withdrawn
#define MAPPABLE_MASK 	(1U << 30)
///the window is currently mapped
#define MAPPED_MASK 	(1U << 31)

/// These masks indicate state beyond our control and should not be arbitrarily set
#define EXTERNAL_MASKS 	(INPUT_MASK|WM_TAKE_FOCUS_MASK|WM_DELETE_WINDOW_MASK|WM_PING_MASK|FULLY_VISIBLE_MASK|MAPPABLE_MASK|MAPPED_MASK)
/// A change in these masks may cause windows to be retiled
#define RETILE_MASKS 	(MAXIMIZED_MASK | ALL_NO_TILE_MASKS | MAPPABLE_MASK)
/// set all masks
#define ALL_MASK        (-1)

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
#define _PRINT_MASK(windowMask) if( windowMask != 0 && (windowMask & M) == (uint32_t)windowMask || windowMask==0 && M==0){s+=#windowMask " ";M&=~windowMask;}
        uint32_t M = mask;
        std::string s = std::to_string(mask ? __builtin_popcount(mask) : 0) + " bits: ";
        _PRINT_MASK(NO_MASK);
        _PRINT_MASK(MAXIMIZED_MASK);
        _PRINT_MASK(X_MAXIMIZED_MASK);
        _PRINT_MASK(Y_MAXIMIZED_MASK);
        _PRINT_MASK(FULLSCREEN_MASK);
        _PRINT_MASK(ROOT_FULLSCREEN_MASK);
        _PRINT_MASK(FLOATING_MASK);
        _PRINT_MASK(BELOW_MASK);
        _PRINT_MASK(ABOVE_MASK);
        _PRINT_MASK(NO_TILE_MASK);
        _PRINT_MASK(STICKY_MASK);
        _PRINT_MASK(PRIMARY_MONITOR_MASK);
        _PRINT_MASK(HIDDEN_MASK);
        _PRINT_MASK(EXTERNAL_CONFIGURABLE_MASK);
        _PRINT_MASK(EXTERNAL_RESIZE_MASK);
        _PRINT_MASK(EXTERNAL_MOVE_MASK);
        _PRINT_MASK(EXTERNAL_BORDER_MASK);
        _PRINT_MASK(EXTERNAL_RAISE_MASK);
        _PRINT_MASK(IGNORE_WORKSPACE_MASKS_MASK);
        _PRINT_MASK(INPUT_MASK);
        _PRINT_MASK(NO_RECORD_FOCUS_MASK);
        _PRINT_MASK(NO_ACTIVATE_MASK);
        _PRINT_MASK(WM_TAKE_FOCUS_MASK);
        _PRINT_MASK(WM_DELETE_WINDOW_MASK);
        _PRINT_MASK(WM_PING_MASK);
        _PRINT_MASK(ALWAYS_ON_TOP_MASK);
        _PRINT_MASK(ALWAYS_ON_BOTTOM_MASK);
        _PRINT_MASK(FULLY_VISIBLE_MASK);
        _PRINT_MASK(PARTIALLY_VISIBLE_MASK);
        _PRINT_MASK(MAPPED_MASK);
        _PRINT_MASK(MAPPABLE_MASK);
        _PRINT_MASK(URGENT_MASK);
        assert(!M);
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
