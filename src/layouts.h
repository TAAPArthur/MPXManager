/**
 * @file layouts.h
 *
 * @brief Layout functions
 */

#ifndef LAYOUTS_H_
#define LAYOUTS_H_

#include "masters.h"
#include "mywm-structs.h"
#include "wmfunctions.h"
#include "workspaces.h"

/// size of config parameters
#define CONFIG_LEN 7
enum {CONFIG_INDEX_X = 0, CONFIG_INDEX_Y = 1, CONFIG_INDEX_WIDTH, CONFIG_INDEX_HEIGHT, CONFIG_INDEX_BORDER, CONFIG_INDEX_STACK};

/// Number of transformations
#define TRANSFORM_LEN (4)

/**
 * Transformations that can be applied to layouts.
 * Everything is relative to the midpoint of the view port of the monitor
 */
typedef enum {
    /// Apply no transformation
    NONE = 0,
    /// Rotate by 180 deg
    ROT_180,
    /// Reflect across x axis
    REFLECT_HOR,
    /// Reflect across the y axis
    REFLECT_VERT
} Transform ;
/// Represents a field(s) in Layout Args
typedef enum {
    LAYOUT_LIMIT = 0,
    /// represents all padding fields
    LAYOUT_PADDING,
    LAYOUT_LEFT_PADDING,
    LAYOUT_TOP_PADDING,
    LAYOUT_RIGHT_PADDING,
    LAYOUT_BOTTOM_PADDING,
    LAYOUT_NO_BORDER,
    LAYOUT_NO_ADJUST_FOR_BORDERS,
    LAYOUT_DIM,
    LAYOUT_TRANSFORM,
    LAYOUT_ARG,
} LayoutArgIndex ;
/**
 * Options used to customize layouts
 */
typedef struct {
    /// at most limit windows will be tiled
    unsigned short limit;
    /// @{ padding between the tiled size and the actual size of the window
    short leftPadding;
    short topPadding;
    short rightPadding;
    short bottomPadding;
    /// #}
    /// if set windows won't have a border
    bool noBorder;
    /// if we set the size of the window with/without borders
    bool noAdjustForBorders;
    /// if set layouts based on X/WIDTH will use Y/HEIGHT instead
    bool dim;
    /// the transformation (about the center of the monitor) to apply to all window positions
    Transform transform;
    /// generic argument
    float arg;
    /// generic argument
    float argStep;
} LayoutArgs ;

/**
 * Contains info provided to layout functions (and helper methods) that detail how the windows should be tiled
 */
typedef struct LayoutState {
    /// Customized to the layout family
    LayoutArgs* args;
    /// the monitor used to layout the window
    const Monitor* monitor;
    /// number of windows that should be tiled
    const int numWindows;
    /// the stack of windows
    const ArrayList* stack;
} LayoutState ;

///holds meta data to to determine what tiling function to call and when/how to call it
struct Layout {
    /**
     * The name of the layout.
     */
    const char* name;
    /**
     * The function to call to tile the windows
     */
    void (*func)(LayoutState*);
    /**
     * Arguments to pass into layout functions that change how windows are tiled.
     */
    LayoutArgs args;
    /**
     * Used to restore args after they have been modified
     */
    LayoutArgs refArgs;
} ;
/**
 * Saves the current args for layout so they can be restored later
 *
 */
static inline void saveArgs(Layout* l) { l->refArgs = l->args; }
/**
 * Restores saved args for the active layout
 */
static inline void restoreArgs(Layout* l) { l->args = l->refArgs; }
static inline void restoreActiveLayout() { if(getActiveLayout())getActiveLayout()->args = getActiveLayout()->refArgs; }

///@{ Default layouts
extern Layout FULL, GRID, TWO_COL, THREE_COL, TWO_ROW, TWO_PANE, TWO_PLANE_H, MASTER, TWO_MASTER, TWO_MASTER_FLIPPED,
       TWO_MASTER_H;
///@}

/**
 * Searches for the set of registered layouts for a layout with the given name and returns it
 *
 * @param name
 *
 * @return
 */
Layout* findLayoutByName(const char* name);

/*TODO
/// @param name the name of the registered layout that will become the new active layout of the active workspace
static inline void setActiveLayout(const char* name) {getActiveWorkspace()->setActiveLayout(findLayoutByName(name));}
/// @param l the new active layout of the active workspace
static inline void setActiveLayout(Layout* l) {getActiveWorkspace()->setActiveLayout(l);}
/// @return the active layout of the active workspace
static inline Layout* getActiveLayout() {return getActiveWorkspace()->getActiveLayout();}
*/

/**
 * Registers a window so it can be found by its name
 * All default layouts are considered registered
 * @param layout
 */
void registerLayout(Layout* layout);

void unregisterLayout(Layout* layout);
/**
 * @return list of registered layouts
 */
const ArrayList* getRegisteredLayouts();

/**
 * Unregisters a window so it can be found by its name
 *
 * @param layout
 */
void unregisteredLayout(Layout* layout);
/**
 * Increases the corresponding field for the layout by step
 *
 * @param index a LayoutArgIndex
 * @param step
 * @param l the layout to change
 */
void increaseLayoutArg(int index, int step, Layout* l);
static inline void increaseActiveLayoutArg(int index, int step) { increaseLayoutArg(index, step, getActiveLayout());};

/**
 * Manually retile the active workspace
 */
void retile(void);


/**
 * Provides a transformation of config
 *
 * @param args
 * @param m
 * @param config the config that will be modified
 */
void transformConfig(const LayoutArgs* args, const Monitor* m, uint32_t* config);
/**
 * Configures the winInfo using values as reference points and apply various properties of winInfo's mask and set configuration which will override values
 * @param state
 * @param winInfo the window to tile
 * @param values where the layout wants to position the window
 */
void tileWindow(const LayoutState* state, const WindowInfo* winInfo, const short* values);

/**
 * "Tiles" untileable windows
 * This involves applying layout-related masks and tiling override config
 *
 * @param winInfo
 * @param monitor
 */
void arrangeNonTileableWindow(const WindowInfo* winInfo, const Monitor* monitor) ;
/**
 * Tiles the specified workspace.
 * First the windows in the tileable windows are tiled according to the active layout's layoutFunction
 *
 * For un-tileable windows:
 * It will raise/lower window with the ABOVE_MASK/BELOW_MASK
 * If will will change the size and posibly position of windows with MAXIMIZED_MASK/FULLSCREEN_MASK/ROOT_FULLSCREEN_MASK
 * @param workspace the workspace to tile
 */
void tileWorkspace(Workspace* workspace);

/**
 * Windows will be the size of the monitor view port
 * @param state
 */
void full(LayoutState* state);

/**
 * If the number of windows to tile is 1, this method behaves like full();
 * Windows will be arranged in state->args->arg[0] columns. If arg[0] is 0 then there will log2(num windows to tile) columns
 * All columns will have the same number of windows (+/-1) and the later columns will have less
 * @param state
 */
void column(LayoutState* state);
/**
 * If the number of windows to tile is 1, this method behaves like full();
 * The head of the windowStack will take up MAX(state->args->arg[0]% of the screen, 1) and the remaining windows will be in a single column
 * @param state
 */
void masterPane(LayoutState* state);

void retileAllDirtyWorkspaces();

void registerDefaultLayouts();

/**
 * If percent, the field corresponding to the bitmask will be treated as a percent of region.
 * ie Rect(0, 25, 50, 100).getRelativeRegion({0, 0, 100, 200}) will yield {0, 50, 50, 200} if percent is 31
 * Else If region contains only positive numbers, then region is returned
 * Else If region contains negative numbers then it "wraps around" ie {-1, -1, 1, 1} refers the to the bottom right region
 * and {1, 1, -1, -1} refers to everything except the top-left most region
 * If a region has 0 for the width or height, then that index is set to this rect's width/height
 *
 * @param region
 *
 * @return the region described by region
 */
Rect getRelativeRegion(const Rect bounds, const Rect region, bool percent);

#endif /* LAYOUTS_H_ */
