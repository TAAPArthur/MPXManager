/**
 * @file window-masks.h
 *
 */
#ifndef WINDOW_MASKS_H_
#define WINDOW_MASKS_H_

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
typedef enum {
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

    IGNORE_WORKSPACE_MASKS_MASK = 1 << 15,

    /**The window is an input only window (more importantly it cannot have a border)*/
    INPUT_ONLY_MASK =           1 << 16,
    /**The window can receive input focus*/
    INPUT_MASK =           1 << 17,
    /// Will not update our internal focus representation when a window with this mask is focused
    NO_RECORD_FOCUS =      1 << 18,
    /// Windows with this mask will not be activatable
    NO_ACTIVATE_MASK =      1 << 19,

    /**The WM will not forcibly set focus but request the application focus itself*/
    WM_TAKE_FOCUS_MASK =    1 << 20,

    /**The WM will not forcibly delete windows immediately but request the application dies*/
    WM_DELETE_WINDOW_MASK = 1 << 21,
    /**Used in conjunction with WM_DELETE_WINDOW_MASK to kill the window */
    WM_PING_MASK =          1 << 22,
    /// the window type was not set explicitly
    IMPLICIT_TYPE =         1 << 23,

    /**
     * Best effort will be made to place all windows with this mask above any other window without it.
     * One particular flaw the implementation is that if a window with this mask is lowered, it will not automatically be re-raised
     * This mask is implemented via a onWindowMove Rule
     */
    ALWAYS_ON_TOP = 1 << 24,
    /**
     * Best effort will be made to place all windows with this mask below any other window without it.
     * One particular flaw the implementation is that if a window with this mask is raised, it will not automatically be re-lowered
     * This mask is implemented via a onWindowMove Rule
     */
    ALWAYS_ON_BOTTOM = 1 << 25,

    ///Window is effectively associated with its monitor instead of its workspace
    /// (it is move dded between workspaces to stay on its monitor)
    STICKY_MASK =   1 << 26,


    /**Marks the window as urgent*/
    URGENT_MASK =           1 << 27,


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

} WindowMasks;
#include <string>
#define _PRINT_MASK(str,mask) if( (str & mask)||(str==0 &&mask==0))s+=#str " ";
static inline std::string maskToString(WindowMask mask) {
    std::string s = "";
    _PRINT_MASK(NO_MASK, mask);
    _PRINT_MASK(X_MAXIMIZED_MASK, mask);
    _PRINT_MASK(Y_MAXIMIZED_MASK, mask);
    _PRINT_MASK(MAXIMIZED_MASK, mask);
    _PRINT_MASK(FULLSCREEN_MASK, mask);
    _PRINT_MASK(ROOT_FULLSCREEN_MASK, mask);
    _PRINT_MASK(BELOW_MASK, mask);
    _PRINT_MASK(ABOVE_MASK, mask);
    _PRINT_MASK(NO_TILE_MASK, mask);
    _PRINT_MASK(STICKY_MASK, mask);
    _PRINT_MASK(HIDDEN_MASK, mask);
    _PRINT_MASK(EXTERNAL_RESIZE_MASK, mask);
    _PRINT_MASK(EXTERNAL_MOVE_MASK, mask);
    _PRINT_MASK(EXTERNAL_BORDER_MASK, mask);
    _PRINT_MASK(EXTERNAL_RAISE_MASK, mask);
    _PRINT_MASK(FLOATING_MASK, mask);
    _PRINT_MASK(SRC_INDICATION_OTHER, mask);
    _PRINT_MASK(SRC_INDICATION_APP, mask);
    _PRINT_MASK(SRC_INDICATION_PAGER, mask);
    _PRINT_MASK(SRC_ANY, mask);
    _PRINT_MASK(IGNORE_WORKSPACE_MASKS_MASK, mask);
    _PRINT_MASK(INPUT_ONLY_MASK, mask);
    _PRINT_MASK(INPUT_MASK, mask);
    _PRINT_MASK(NO_RECORD_FOCUS, mask);
    _PRINT_MASK(NO_ACTIVATE_MASK, mask);
    _PRINT_MASK(WM_TAKE_FOCUS_MASK, mask);
    _PRINT_MASK(WM_DELETE_WINDOW_MASK, mask);
    _PRINT_MASK(WM_PING_MASK, mask);
    _PRINT_MASK(IMPLICIT_TYPE, mask);
    _PRINT_MASK(ALWAYS_ON_TOP, mask);
    _PRINT_MASK(ALWAYS_ON_BOTTOM, mask);
    _PRINT_MASK(PARTIALLY_VISIBLE, mask);
    _PRINT_MASK(FULLY_VISIBLE, mask);
    _PRINT_MASK(MAPPABLE_MASK, mask);
    _PRINT_MASK(MAPPED_MASK, mask);
    _PRINT_MASK(URGENT_MASK, mask);
    return s;
}
#endif