/**
 * @file layouts.h
 *
 * @brief Layout functions
 */

#ifndef LAYOUTS_H_
#define LAYOUTS_H_

#include "mywm-util.h"

#define CONFIG_LEN 6

#define TRANSFORM_LEN 6
typedef enum {NONE=0,ROT_90=1,ROT_180,ROT_270,REFLECT_HOR,REFLECT_VERT}Transform;
enum{CONFIG_X=0,CONFIG_Y=1,CONFIG_WIDTH,CONFIG_HEIGHT,CONFIG_BORDER,CONFIG_STACK};

/**
 * Options used to customize layouts
 */
typedef struct{
    /// at most limit windows will be tiled
    unsigned char limit;
    /// if set windows won't have a border
    unsigned int noBorder:1;
    /// if set windows won't have a border
    unsigned int noAdjustForBorders:1;
    /// the transformation (about the center of the monitor) to apply to all window positions
    Transform transform;
    /// if set layouts based on X/WIDTH will use Y/HEIGHT instead
    int dim:1;
    /// will tile layouts backwards -- starting with the last windows and going upwards
    int reverse:1;
    /// will raise the focused window
    int raiseFocused:1;
    /// whether to raise or lower windows when tiling
    int lowerWindows:1;
    /// generic argument
    float arg[1];
}LayoutArgs;

/**
 * Contains info provided to layout functions (and helper methods) that detail how the windows should be layed out
 */
typedef struct{
    /// Customized to the layout family
    LayoutArgs*args;
    /// the monitor used to layout the window
    Monitor*monitor;
    /// number of windows that should be layed out
    int numWindows;
    /// the stack of windows
    Node*stack;
    /// the last window in the stack to be tiled
    Node*last;
} LayoutState;

///holds meta data to to determine what tiling function to call and and when/how to call it
typedef struct Layout{
    /**
     * Arguments to pass into layout functions.
     * These vary with the function but the first arguments are
     * limit and border
     */
    LayoutArgs args;
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
enum {FULL,GRID,TWO_COLUMN,TWO_PANE,LAST_LAYOUT};

/// Array of layout familes (layouts with different layout functions
extern Layout LAYOUT_FAMILIES[];
/// Array of different layout 
extern Layout DEFAULT_LAYOUTS[];
/// number of DEFAULT_LAYOUTS 
extern int NUMBER_OF_DEFAULT_LAYOUTS;
/// number of LAYOUT_FAMILIES
extern int NUMBER_OF_LAYOUT_FAMILIES;

/**
* Returns the name of the given layout
* @param layout
* @return
*/
char* getNameOfLayout(Layout*layout);
/**
 * Addes an array of layouts to the specified workspace
 * @param workspaceIndex
 * @param layout - layouts to add
 * @param num - number of layouts to add
 */
void addLayoutsToWorkspace(int workspaceIndex,Layout*layout,int num);
/**
 * Clears all layouts assosiated with the give workspace.
 * The layouts themselves are not freeded
 * @param workspaceIndex the workspace to clear
 */
void clearLayoutsOfWorkspace(int workspaceIndex);

/**
 * Cycle through the layouts of the active workspace
 * @param index either UP or DOWN
 */
void cycleLayouts(int dir);

/**
 * Apply this external layout to the workspace if it is not already the active layout
 * @param layout
 */
void toggleLayout(Layout* layout);

/**
 * Manually retile the active workspace
 */
void retile(void);

/**
 * Tile the windows with BELOW_MASK to be under all normal windows
 */
void tileLowerLayers(Workspace*workspace);
/**
 * Tile the windows with ABOVE_MASK to be under all normal windows
 */
void tileUpperLayers(Workspace*workspace);
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
void applyLayout(Workspace*workspace);

/**
 *
 * @param windowStack the list of nodes that could possibly be tiled
 * @param args can control the max number of windows tiled (args->limit)
 * @param last if not null, the last tileable node will be return to last
 * @return the number of windows that will be tiled
 */
int getNumberOfWindowsToTile(Node*windowStack,LayoutArgs*args,Node**last);

/**
 * Provides a transformation of config
 * @param state holds info related to the layout like the monitor
 * @param config the config that will be modified
 */
void transformConfig(LayoutState*state,int config[CONFIG_LEN]);
void configureWindow(LayoutState*state,WindowInfo* winInfo,short values[CONFIG_LEN]);
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
void full(LayoutState*state);
/**
 * Windows will be the size of the monitor view port
 * @param state
 */
void column(LayoutState*state);
void masterPane(LayoutState*state);

#endif /* LAYOUTS_H_ */
