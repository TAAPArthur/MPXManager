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

void restoreActiveLayout();

/**
 * Searches for the set of registered layouts for a layout with the given name and returns it
 *
 * @param name
 *
 * @return
 */
Layout* findLayoutByName(const char* name);

const char* getLayoutName(const Layout* layout);

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
void increaseActiveLayoutArg(int index, int step);

/**
 * Manually retile the active workspace
 */
void retile(void);

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
