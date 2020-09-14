/**
 * @file window-masks.h
 *
 */
#ifndef WINDOW_MASKS_H_
#define WINDOW_MASKS_H_

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


/**Marks the window as urgent*/
#define URGENT_MASK 	(1U << 8)
/**
 * Window is effectively associated with its monitor instead of its workspace
 * (it is move added between workspaces to stay on its monitor)
 */
#define STICKY_MASK 	(1U << 9)
/// will cause WindowInfo::isSpecial to return 1
#define SPECIAL_MASK 	(1U << 10)
/// will cause all masks to be synced regardless of MASKS_TO_SYNC
#define SYNC_ALL_MASKS 	(1U << 11)
/// corresponds to modal state
#define MODAL_MASK 	(1U << 12)

/**The window's X position will be centered in the viewport */
#define X_CENTERED_MASK 	(1U << 13)
/**The window's Y position will be centered in the viewport */
#define Y_CENTERED_MASK 	(1U << 14)
/// Have the top left point of the window be in the center of the monitor
#define CENTERED_MASK 	(X_CENTERED_MASK | Y_CENTERED_MASK)

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
#define FLOATING_MASK  (SYNC_ALL_MASKS | NO_TILE_MASK | EXTERNAL_CONFIGURABLE_MASK | ABOVE_MASK )

/// or of all masks that will cause the layout function to skip a window
#define ALL_NO_TILE_MASKS 	(FULLSCREEN_MASK | ROOT_FULLSCREEN_MASK | NO_TILE_MASK | HIDDEN_MASK | MODAL_MASK)

/**The window can receive input focus*/
#define INPUT_MASK 	(1U << 24)

/**The WM send a client message after focusing the window*/
#define WM_TAKE_FOCUS_MASK 	(1U << 25)

/**The WM will not forcibly delete windows immediately but request the application dies*/
#define WM_DELETE_WINDOW_MASK 	(1U << 26)
/**Used in conjunction with WM_DELETE_WINDOW_MASK to kill the window */
#define WM_PING_MASK 	(1U << 27)
///Indicates the window is not withdrawn
#define MAPPABLE_MASK 	(1U << 30)
///the window is currently mapped
#define MAPPED_MASK 	(1U << 31)


/// Masks that all windows to enter the top layer
#define TOP_LAYER_MASKS (FULLSCREEN_MASK | ROOT_FULLSCREEN_MASK | ABOVE_MASK )
/// Masks that all windows to enter the bottom layer
#define BOTTOM_LAYER_MASKS (BELOW_MASK)

/// The minimum requirements to be reported has the focusedWindow
#define FOCUSABLE_MASK  (INPUT_MASK|MAPPED_MASK)
/// These masks indicate state beyond our control and should not be arbitrarily set
#define EXTERNAL_MASKS 	(INPUT_MASK|WM_TAKE_FOCUS_MASK|WM_DELETE_WINDOW_MASK|WM_PING_MASK|MAPPABLE_MASK|MAPPED_MASK)
/// A change in these masks may cause windows to be retiled
#define RETILE_MASKS 	(MAXIMIZED_MASK | ALL_NO_TILE_MASKS | MAPPABLE_MASK)
/// set all masks
#define ALL_MASK        (-1)

#endif
