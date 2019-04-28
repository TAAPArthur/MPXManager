/**
 * @file layouts.h
 *
 * @brief Layout functions
 */

#ifndef LAYOUTS_H_
#define LAYOUTS_H_

#include "mywm-structs.h"

/// size of config parameters
#define CONFIG_LEN 6

/// Number of transformations
#define TRANSFORM_LEN (REFLECT_VERT+1)

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
} Transform;
enum {CONFIG_X = 0, CONFIG_Y = 1, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_BORDER, CONFIG_STACK};
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
    LAYOUT_RAISE_FOCUSED,
    LAYOUT_LOWER_WINDOWS,
    LAYOUT_TRANSFORM,
    LAYOUT_ARG,
} LayoutArgIndex;
/**
 * Options used to customize layouts
 */
typedef struct {
    /// at most limit windows will be tiled
    unsigned char limit;
    /// @{ padding between the tiled size and the actual size of the window
    int leftPadding;
    int topPadding;
    int rightPadding;
    int bottomPadding;
    /// #}
    /// if set windows won't have a border
    bool noBorder;
    /// if we set the sie of the window with/without borders
    bool noAdjustForBorders;
    /// if set layouts based on X/WIDTH will use Y/HEIGHT instead
    bool dim;
    /// will raise the focused window
    bool raiseFocused;
    /// whether to raise or lower windows when tiling
    bool lowerWindows;
    /// the transformation (about the center of the monitor) to apply to all window positions
    Transform transform;
    /// generic argument
    float argStep[1];
    /// generic argument
    float arg[1];
} LayoutArgs;

/**
 * Contains info provided to layout functions (and helper methods) that detail how the windows should be layed out
 */
typedef struct {
    /// Customized to the layout family
    LayoutArgs* args;
    /// the monitor used to layout the window
    Monitor* monitor;
    /// number of windows that should be layed out
    int numWindows;
    /// the stack of windows
    ArrayList* stack;
    /// the last window in the stack to be tiled
    ArrayList* last;
} LayoutState;

///holds meta data to to determine what tiling function to call and and when/how to call it
typedef struct Layout {
    /**
     * Arguments to pass into layout functions that change how windows are tiled.
     */
    LayoutArgs args;
    /**
     * Used to restore args after they have been modified
     */
    LayoutArgs refArgs;
    /**
     * The name of the layout. Used solely for user
     */
    char* name;
    /**
     * The function to call to tile the windows
     */
    void (*layoutFunction)(LayoutState*);
} Layout;

/// Convience indexes into DEFAULT_LAYOUTS
enum {FULL, GRID, TWO_COLUMN, TWO_PANE, TWO_HPLANE, MASTER, LAST_LAYOUT};

/// Array of layout familes (layouts with different layout functions
extern Layout LAYOUT_FAMILIES[];
/// Array of different layout
extern Layout DEFAULT_LAYOUTS[];
/// number of DEFAULT_LAYOUTS
extern int NUMBER_OF_DEFAULT_LAYOUTS;
/// number of LAYOUT_FAMILIES
extern int NUMBER_OF_LAYOUT_FAMILIES;

/**
 * Increases the corrosponding field for the layour by step
 *
 * @param index a LayoutArgIndex
 * @param step
 */
void increaseActiveLayoutArg(int index, int step);
/**
 * Searches for the set of registered layouts for a layout with the name, and if a match is found, sets the active layout this layoutyy
 *
 * @param name
 */
void setActiveLayoutByName(char* name);
/**
 * Searches for the set of registered layouts for a layout with the given name and returns it
 *
 * @param name
 *
 * @return
 */
Layout* findLayoutByName(char* name);
/**
 * Saves the current args for layout so they can be restored later
 *
 * @param layout
 */
void saveLayoutArgs(Layout* layout);
/**
 * Restores saved args for the active layout
 */
void restoreActiveLayout(void);
/**
 * Registers num layouts so they can searched for later
 *
 * @param layouts
 * @param num
 */
void registerLayouts(Layout* layouts, int num);

/**
* Returns the name of the given layout
* @param layout
* @return
*/
char* getNameOfLayout(Layout* layout);
/**
 * Addes an array of layouts to the specified workspace
 * @param workspaceIndex
 * @param layout - layouts to add
 * @param num - number of layouts to add
 */
void addLayoutsToWorkspace(int workspaceIndex, Layout* layout, int num);
/**
 * Clears all layouts assosiated with the give workspace.
 * The layouts themselves are not freeded
 * @param workspaceIndex the workspace to clear
 */
void clearLayoutsOfWorkspace(int workspaceIndex);

/**
 * Cycle through the layouts of the active workspace
 * @param dir
 */
void cycleLayouts(int dir);

/**
 * Apply this external layout to the workspace if it is not already the active layout
 * @param layout
 */
int toggleLayout(Layout* layout);

/**
 * Manually retile the active workspace
 */
void retile(void);

/**
 * Tile the windows with BELOW_MASK to be under all normal windows
 */
void tileLowerLayers(Workspace* workspace);
/**
 * Tile the windows with ABOVE_MASK to be under all normal windows
 */
void tileUpperLayers(Workspace* workspace);
/**
 * Tiles the specified workspace.
 * First the windows in the NORMAL_LAYER are tiled according to the active layout's layoutFunction
 * (if sett) or the conditionFunction is not true then the windows are left as in.
 * Afterwards the remaining layers are tiled in ascending order
 * @param index the index of the workspace to tile
 * @see tileLowerLayers, tileUpperLayers
 */
void tileWorkspace(int index);

/**
 * Apply the workspace's active layout on the given workspace
 * @param workspace
 */
void applyLayout(Workspace* workspace);

/**
 *
 * @param windowStack the list of nodes that could possibly be tiled
 * @param args can control the max number of windows tiled (args->limit)
 * @return the number of windows that will be tiled
 */
int getNumberOfWindowsToTile(ArrayList* windowStack, LayoutArgs* args);

/**
 * Provides a transformation of config
 * @param state holds info related to the layout like the monitor
 * @param config the config that will be modified
 */
void transformConfig(LayoutState* state, int config[CONFIG_LEN]);
/**
 * Configures the winInfo using values as refrence points and apply various properties of winInfo's mask and set configuration which will override values
 * @param state
 * @param winInfo the window to tile
 * @param values where the layout wants to position the window
 */
void configureWindow(LayoutState* state, WindowInfo* winInfo, short values[CONFIG_LEN]);
/**
 *
 * @return the active layout for the active workspace
 */
Layout* getActiveLayout(void);

/**
 * @param workspaceIndex
 * @return the active layout for the specified workspace
 */
Layout* getActiveLayoutOfWorkspace(int workspaceIndex);

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

#endif /* LAYOUTS_H_ */
