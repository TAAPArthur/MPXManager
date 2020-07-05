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
enum Transform {
    /// Apply no transformation
    NONE = 0,
    /// Rotate by 180 deg
    ROT_180,
    /// Reflect across x axis
    REFLECT_HOR,
    /// Reflect across the y axis
    REFLECT_VERT
} ;
/// Represents a field(s) in Layout Args
enum LayoutArgIndex {
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
    LAYOUT_TRANSFORM,
    LAYOUT_ARG,
} ;
/**
 * Options used to customize layouts
 */
struct LayoutArgs {
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
    /// will raise the focused window
    bool raiseFocused;
    /// the transformation (about the center of the monitor) to apply to all window positions
    Transform transform;
    /// generic argument
    float arg;
    /// generic argument
    float argStep;
    int getDimIndex()const {
        return dim == 0 ? CONFIG_INDEX_WIDTH : CONFIG_INDEX_HEIGHT;
    }
    int getOtherDimIndex()const {
        return dim == 0 ? CONFIG_INDEX_HEIGHT : CONFIG_INDEX_WIDTH;
    }
} ;

/**
 * Contains info provided to layout functions (and helper methods) that detail how the windows should be tiled
 */
struct LayoutState {
    /// Customized to the layout family
    LayoutArgs* args;
    /// the monitor used to layout the window
    const Monitor* monitor;
    /// number of windows that should be tiled
    const int numWindows;
    /// the stack of windows
    const ArrayList<WindowInfo*>& stack;
    /// @return layout args
    const LayoutArgs* getArgs() const {return args;}
} ;

///holds meta data to to determine what tiling function to call and when/how to call it
struct Layout {
private:
    /**
     * The name of the layout.
     */
    const std::string name = "";
    /**
     * The function to call to tile the windows
     */
    void (*layoutFunction)(LayoutState*) = NULL;
    /**
     * Arguments to pass into layout functions that change how windows are tiled.
     */
    LayoutArgs args = {0};
    /**
     * Used to restore args after they have been modified
     */
    LayoutArgs refArgs;
    bool (*filter)(WindowInfo*);
public:
    /**
     * @param name the name of the layout
     * @param layoutFunction the underlying layout function
     * @param args
     * @param filter @see filterWindow
     */
    Layout(std::string name, void (*layoutFunction)(LayoutState*), LayoutArgs args = {0}, bool (*filter)(
            WindowInfo*) = NULL): name(name),
        layoutFunction(layoutFunction), args(args), refArgs(args), filter(filter) {
    }

    /// @return the name of the layout
    std::string getName()const {return name;}
    /// @return the name of the layout
    operator std::string()const {return name;}
    /// @return 1 iff the names match
    bool operator==(const Layout& l)const {return name == l.name;};
    /// @return the backing function
    auto getFunc() {return layoutFunction;}

    /**
     * Returns true if the window passes the filter (which it does it not set) and should be tiled normally
     * Windows that don't pass should be treated the same as those past the layout limit
     *
     * @param winInfo
     *
     * @return 1 if this window should be normally tiled
     */
    bool filterWindow(WindowInfo* winInfo) {return !filter || filter(winInfo);}

    /**
     * Tiles the windows of the stack specified by state
     *
     * @param state
     */
    void apply(LayoutState* state) {layoutFunction(state);}
    /// @return configurable options
    LayoutArgs& getArgs() {return args;}
    /**
     * Saves the current args for layout so they can be restored later
     *
     */
    void saveArgs() {
        refArgs = args;
    }
    /**
     * Restores saved args for the active layout
     */
    void restoreArgs() {
        args = refArgs;
    }
} ;

///@{ Default layouts
extern Layout FULL, GRID, TWO_ROW, TWO_COL, TWO_PANE, TWO_HPLANE, MASTER;
///@}

/**
 * Searches for the set of registered layouts for a layout with the given name and returns it
 *
 * @param name
 *
 * @return
 */
Layout* findLayoutByName(std::string name);

/**
 * Cycle through the layouts of the active workspace
 * @param dir
 */
static inline void cycleLayouts(int dir) {getActiveWorkspace()->cycleLayouts(dir);}

/**
 * Looks up the registered layout with this name and sets it to be the active
 * layout of the workspace if it is not already the active layout
 *
 * @param name the name of a register layout
 *
 * @return
 */
static inline int toggleLayout(std::string name) { return getActiveWorkspace()->toggleLayout(findLayoutByName(name));}

/// @param name the name of the registered layout that will become the new active layout of the active workspace
static inline void setActiveLayout(std::string name) {getActiveWorkspace()->setActiveLayout(findLayoutByName(name));}
/// @param l the new active layout of the active workspace
static inline void setActiveLayout(Layout* l) {getActiveWorkspace()->setActiveLayout(l);}
/// @return the active layout of the active workspace
static inline Layout* getActiveLayout() {return getActiveWorkspace()->getActiveLayout();}

/**
 * Registers a window so it can be found by its name
 * All default layouts are considered registered
 * @param layout
 */
void registeredLayout(Layout* layout);
/**
 * @return list of registered layouts
 */
const ArrayList<Layout*>& getRegisteredLayouts();

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
void increaseLayoutArg(int index, int step, Layout* l = getActiveWorkspace()->getActiveLayout());

/**
 * Manually retile the active workspace
 */
void retile(void);

/**
 * Raise windows in workspace by the order in which they were last focused by the active master
 *
 * @param workspace
 */
void raiseWindowInFocusOrder(Workspace* workspace);


/**
 * Provides a transformation of config
 *
 * @param args
 * @param m
 * @param config the config that will be modified
 */
void transformConfig(const LayoutArgs* args, const Monitor* m, uint32_t config[CONFIG_LEN]);
/**
 * Configures the winInfo using values as reference points and apply various properties of winInfo's mask and set configuration which will override values
 * @param state
 * @param winInfo the window to tile
 * @param values where the layout wants to position the window
 */
void configureWindow(const LayoutState* state, const WindowInfo* winInfo, const short values[CONFIG_LEN]);

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

#endif /* LAYOUTS_H_ */
