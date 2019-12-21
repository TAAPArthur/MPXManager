#include <assert.h>
#include <math.h>
#include <string.h>

#include "bindings.h"
#include "globals.h"
#include "layouts.h"
#include "logger.h"
#include "masters.h"
#include "monitors.h"
#include "windows.h"
#include "wmfunctions.h"
#include "workspaces.h"
#include "user-events.h"

#define MAX(A,B) (A>=B?A:B)

std::ostream& operator<<(std::ostream& strm, const Layout& layout) {
    return strm << layout.getName();
}

Layout FULL = {.name = "Full", .layoutFunction = full, .args = {.noBorder = 1, .raiseFocused = 1}},
       GRID = {.name = "Grid", .layoutFunction = column},
       TWO_COL = {.name = "2 Col", .layoutFunction = column, .args = {.dim = 0, .arg = {2}}},
       TWO_ROW = {.name = "2 Row", .layoutFunction = column, .args = {.dim = 1, .arg = {2}}},
       TWO_PANE = {.name = "2 Pane", .layoutFunction = column, .args = {.limit = 2, .dim = 0, .raiseFocused = 1, .arg = {2}}},
       TWO_HPLANE = {.name = "2 HPane", .layoutFunction = column, .args = {.limit = 2, .dim = 1, .raiseFocused = 1, .arg = {2} }},
       MASTER = {.name = "Master", .layoutFunction = masterPane, .args = {.arg = {.7}, .argStep = {.1}}};
static ArrayList<Layout*> registeredLayouts = {&FULL, &GRID, &TWO_ROW, &TWO_COL, &TWO_PANE, &TWO_HPLANE, &MASTER};

void registeredLayout(Layout* layout) {
    registeredLayouts.add(layout);
}
void unregisteredLayout(Layout* layout) {
    registeredLayouts.removeElement(layout);
}
const ArrayList<Layout*>& getRegisteredLayouts() {
    return registeredLayouts;
}


Layout* findLayoutByName(std::string name) {
    for(Layout* layout : getRegisteredLayouts())
        if(layout->getName() == name)
            return layout;
    return NULL;
}

void increaseLayoutArg(int index, int step, Layout* l) {
    if(l) {
        switch(index) {
            case LAYOUT_LIMIT:
                l->getArgs().limit += step;
                break;
            case LAYOUT_PADDING:
                l->getArgs().leftPadding += step;
                l->getArgs().topPadding += step;
                l->getArgs().rightPadding += step;
                l->getArgs().bottomPadding += step;
                break;
            case LAYOUT_LEFT_PADDING:
                l->getArgs().leftPadding += step;
                break;
            case LAYOUT_TOP_PADDING:
                l->getArgs().topPadding += step;
                break;
            case LAYOUT_RIGHT_PADDING:
                l->getArgs().rightPadding += step;
                break;
            case LAYOUT_BOTTOM_PADDING:
                l->getArgs().bottomPadding += step;
                break;
            case LAYOUT_NO_BORDER:
                l->getArgs().noBorder = !l->getArgs().noBorder;
                break;
            case LAYOUT_NO_ADJUST_FOR_BORDERS:
                l->getArgs().noAdjustForBorders = !l->getArgs().noAdjustForBorders;
                break;
            case LAYOUT_DIM:
                l->getArgs().dim = !l->getArgs().dim;
                break;
            case LAYOUT_RAISE_FOCUSED:
                l->getArgs().raiseFocused = !l->getArgs().raiseFocused;
                break;
            case LAYOUT_TRANSFORM:
                l->getArgs().transform = (Transform)((l->getArgs().transform + step % TRANSFORM_LEN + TRANSFORM_LEN) % TRANSFORM_LEN) ;
                break;
            case LAYOUT_ARG:
                l->getArgs().arg += l->getArgs().argStep * step;
                break;
        }
        retile();
    }
}

static int dimIndexToPos(int dim) {
    return dim - 2;
}


int getNumberOfWindowsToTile(ArrayList<WindowInfo*>& windowStack, const LayoutArgs* args) {
    int size = 0;
    for(WindowInfo* winInfo : windowStack)
        if(args && args->limit && size == args->limit)
            break;
        else if(winInfo->isTileable())
            size++;
    return size;
}
void transformConfig(const LayoutArgs* args, const Monitor* m, uint32_t config[CONFIG_LEN]) {
    if(args) {
        int endX = m->getViewport().x * 2 + m->getViewport().width;
        int endY = m->getViewport().y * 2 + m->getViewport().height;
        switch(args->transform) {
            case NONE:
                break;
            case REFLECT_HOR:
                config[CONFIG_INDEX_X] = endX - (config[CONFIG_INDEX_X] + config[CONFIG_INDEX_WIDTH]);
                break;
            case REFLECT_VERT:
                config[CONFIG_INDEX_Y] = endY - (config[CONFIG_INDEX_Y] + config[CONFIG_INDEX_HEIGHT]);
                break;
            case ROT_180:
                config[CONFIG_INDEX_X] = endX - (config[CONFIG_INDEX_X] + config[CONFIG_INDEX_WIDTH]);
                config[CONFIG_INDEX_Y] = endY - (config[CONFIG_INDEX_Y] + config[CONFIG_INDEX_HEIGHT]);
                break;
        }
    }
}
static int adjustBorders(const WindowInfo* winInfo, const LayoutState* state, uint32_t config[CONFIG_LEN], int mask) {
    if(winInfo->isInputOnly()) {
        mask &= ~XCB_CONFIG_WINDOW_BORDER_WIDTH;
        for(int i = CONFIG_INDEX_BORDER; i < CONFIG_LEN - 1; i++)
            config[i] = config[i + 1];
    }
    else if(state->getArgs()) {
        config[CONFIG_INDEX_BORDER] = state->getArgs()->noBorder ? 0 : DEFAULT_BORDER_WIDTH;
        if(!state->getArgs()->noAdjustForBorders) {
            config[CONFIG_INDEX_WIDTH] -= config[CONFIG_INDEX_BORDER] * 2;
            config[CONFIG_INDEX_HEIGHT] -= config[CONFIG_INDEX_BORDER] * 2;
        }
    }
    return mask;
}
static void applyMasksToConfig(const WindowInfo* winInfo, const Monitor* m, uint32_t* values) {
    if(winInfo->hasMask(ROOT_FULLSCREEN_MASK)) {
        values[CONFIG_INDEX_X] = 0;
        values[CONFIG_INDEX_Y] = 0;
        values[CONFIG_INDEX_WIDTH] = getRootWidth();
        values[CONFIG_INDEX_HEIGHT] = getRootHeight();
    }
    else if(winInfo->hasMask(FULLSCREEN_MASK)) {
        for(int i = 0; i < 4; i++)
            values[CONFIG_INDEX_X + i] = m->getBase()[i];
    }
    else {
        if(winInfo->hasMask(X_MAXIMIZED_MASK))
            values[CONFIG_INDEX_WIDTH] = m->getViewport().width;
        if(winInfo->hasMask(Y_MAXIMIZED_MASK))
            values[CONFIG_INDEX_HEIGHT] = m->getViewport().height;
    }
}

static void applyTilingOverrideToConfig(const WindowInfo* winInfo, const Monitor* m, uint32_t* config) {
    RectWithBorder tilingOverride = winInfo->getTilingOverride();
    Rect bounds = m->getViewport();
    if(!winInfo->getWorkspace() || winInfo->hasPartOfMask(ROOT_FULLSCREEN_MASK | FULLSCREEN_MASK))
        bounds = config;
    for(int i = 0; i <= CONFIG_INDEX_HEIGHT; i++) {
        if(winInfo->isTilingOverrideEnabledAtIndex(i)) {
            short fixedValue = tilingOverride[i];
            short refEndPoint = bounds[i % 2 + 2];
            config[i] = fixedValue < 0 ? fixedValue + refEndPoint : fixedValue ;
            if(i < CONFIG_INDEX_WIDTH)
                config[i] += bounds[i];
            else if(!config[i])
                config[i] = bounds[i];
        }
    }
    if(winInfo->isTilingOverrideEnabledAtIndex(CONFIG_INDEX_BORDER))
        config[CONFIG_INDEX_BORDER] = MAX(0, tilingOverride.border);
}

void configureWindow(const LayoutState* state, const WindowInfo* winInfo, const short values[CONFIG_LEN]) {
    assert(winInfo);
    int mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
        XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
        XCB_CONFIG_WINDOW_BORDER_WIDTH ;
    uint32_t config[CONFIG_LEN] = {0};
    for(int i = 0; i <= CONFIG_INDEX_HEIGHT; i++)
        config[i] = values[i];
    transformConfig(state->getArgs(), state->monitor, config);
    applyMasksToConfig(winInfo, state->monitor, config);
    mask = adjustBorders(winInfo, state, config, mask);
    applyTilingOverrideToConfig(winInfo, state->monitor, config);
    if(state->getArgs())
        for(int i = 0; i <= CONFIG_INDEX_HEIGHT; i++) {
            if(i < CONFIG_INDEX_WIDTH)
                config[i] += (&state->getArgs()->leftPadding)[i];
            else
                config[i] -= (&state->getArgs()->rightPadding)[i % 2] + (&state->getArgs()->leftPadding)[i % 2];
        }
    config[CONFIG_INDEX_WIDTH] = MAX(1, config[CONFIG_INDEX_WIDTH]);
    config[CONFIG_INDEX_HEIGHT] = MAX(1, config[CONFIG_INDEX_HEIGHT]);
    assert(winInfo->getID());
    configureWindow(winInfo->getID(), mask, config);
}

void arrangeNonTileableWindow(const WindowInfo* winInfo, const Monitor* monitor) {
    uint32_t config[CONFIG_LEN] = {0};
    monitor->getViewport().copyTo(config);
    applyMasksToConfig(winInfo, monitor, config);
    applyTilingOverrideToConfig(winInfo, monitor, config);
    uint32_t mask = winInfo->getTilingOverrideMask();
    if(winInfo->hasPartOfMask(FULLSCREEN_MASK | ROOT_FULLSCREEN_MASK))
        mask |= XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
    else if(!winInfo->isInputOnly()) {
        config[CONFIG_INDEX_BORDER] = winInfo->isTilingOverrideEnabledAtIndex(CONFIG_INDEX_BORDER) ?
            winInfo->getTilingOverride()[CONFIG_INDEX_BORDER] :
            DEFAULT_BORDER_WIDTH;
        mask |= XCB_CONFIG_WINDOW_BORDER_WIDTH;
    }
    if(winInfo->isInputOnly()) {
        mask &= ~XCB_CONFIG_WINDOW_BORDER_WIDTH;
    }
    if(winInfo->hasMask(X_MAXIMIZED_MASK))
        mask |= XCB_CONFIG_WINDOW_WIDTH;
    if(winInfo->hasMask(Y_MAXIMIZED_MASK))
        mask |= XCB_CONFIG_WINDOW_HEIGHT;
    if(mask) {
        LOG(LOG_LEVEL_INFO, "Config %d: mask %d (%d bits)\n", winInfo->getID(), mask, __builtin_popcount(mask));
        uint32_t finalConfig[CONFIG_LEN] = {0};
        for(int i = 0, counter = 0; i < CONFIG_LEN; i++) {
            if(mask & (1 << i))
                finalConfig[counter++] = config[i];
        }
        configureWindow(winInfo->getID(), mask, finalConfig);
    }
}

static WindowID applyAboveBelowMask(ArrayList<WindowInfo*>& list) {
    WindowID currentAbove = 0;
    WindowID currentBelow = 0;
    WindowID lowestAbove = 0;
    for(WindowInfo* winInfo : list) {
        if(winInfo->hasMask(BELOW_MASK)) {
            lowerWindow(winInfo->getID(), currentBelow);
            currentBelow = winInfo->getID();
        }
        else if(winInfo->hasMask(ABOVE_MASK)) {
            if(!currentAbove) {
                raiseWindow(winInfo->getID());
                lowestAbove =  currentAbove;
            }
            else
                lowerWindow(winInfo->getID(), currentAbove);
            currentAbove = winInfo->getID();
        }
    }
    return lowestAbove;
}

void retile(void) {
    tileWorkspace(getActiveWorkspaceIndex());
}

void tileWorkspace(WorkspaceID index) {
    Workspace* workspace = getWorkspace(index);
    LOG(LOG_LEVEL_DEBUG, "Tiling workspace %d\n", index);
    if(!workspace->getMonitor() ||
        !applyEventRules(TileWorkspace, !workspace->getWindowStack().empty() ? workspace->getWindowStack()[0] : NULL))
        return;
    Monitor* m = workspace->getMonitor();
    assert(m);
    ArrayList<WindowInfo*>& windowStack = workspace->getWindowStack();
    if(windowStack.empty()) {
        LOG(LOG_LEVEL_TRACE, "no windows in workspace\n");
        return;
    }
    Layout* layout = workspace->getActiveLayout();
    if(layout) {
        int size = getNumberOfWindowsToTile(windowStack, &layout->getArgs());
        LayoutState state = {.args = &layout->getArgs(), .monitor = m, .numWindows = size, .stack = windowStack};
        if(size)
            if(layout->getFunc()) {
                LOG(LOG_LEVEL_DEBUG, "using '%s' layout: num win %d (max %d)\n", layout->getName().c_str(), size,
                    layout->getArgs().limit);
                layout->apply(&state);
            }
            else
                LOG(LOG_LEVEL_WARN, "WARNING there is not a set layout function\n");
        else
            LOG(LOG_LEVEL_TRACE, "there are no windows to tile\n");
    }
    else if(!layout)
        LOG(LOG_LEVEL_TRACE, "workspace %d does not have a layout; skipping \n", workspace->getID());
    for(WindowInfo* winInfo : windowStack) {
        if(!winInfo->isTileable())
            arrangeNonTileableWindow(winInfo, m);
    }
    WindowID lowestAbove = applyAboveBelowMask(windowStack);
    if(layout && layout->getArgs().raiseFocused) {
        for(WindowInfo* winInfo : getActiveMaster()->getWindowStack()) {
            if(winInfo->getWorkspace() == workspace) {
                if(lowestAbove)
                    lowerWindow(winInfo->getID(), lowestAbove);
                else
                    raiseWindow(winInfo->getID());
                lowestAbove =  winInfo->getID();
            }
        }
    }
}

static uint32_t splitEven(LayoutState* state, int offset, short const* baseValues, int dim, int num,
    int last) {
    assert(num);
    short values[CONFIG_LEN];
    memcpy(values, baseValues, sizeof(values));
    int sizePerWindow = values[dim] / num;
    int rem = values[dim] % num;
    LOG(LOG_LEVEL_DEBUG, "tiling at most %d windows size %d %d\n", num, sizePerWindow, dim);
    uint32_t i = offset;
    int count = 0;
    while(i < state->stack.size()) {
        WindowInfo* winInfo = state->stack[i++];
        if(!winInfo->isTileable())continue;
        count++;
        values[dim] = sizePerWindow + (rem-- > 0 ? 1 : 0);
        configureWindow(state, winInfo, values);
        if(count == num)break;
        values[dim - 2] += values[dim]; //convert width/height index to x/y
    }
    if(last) {
        LayoutState copy = *state;
        for(; i < state->stack.size(); i++) {
            WindowInfo* winInfo = state->stack[i];
            if(winInfo->isTileable())
                configureWindow(&copy, winInfo, values);
        }
    }
    return i;
}

void full(LayoutState* state) {
    short int values[CONFIG_LEN];
    memcpy(&values, &state->monitor->getViewport().x, sizeof(short int) * 4);
    for(int i = state->stack.size() - 1; i >= 0; i--) {
        WindowInfo* winInfo = state->stack[i];
        if(winInfo->isTileable())
            configureWindow(state, winInfo, values);
    }
}

void column(LayoutState* state) {
    int size = state->numWindows;
    if(size == 1) {
        full(state);
        return;
    }
    int numCol = state->getArgs()->arg == 0 ? log2(size + 1) : state->getArgs()->arg;
    int rem = numCol - size % numCol;
    // split of width(0) or height(1)
    int splitDim = state->getArgs()->getDimIndex();
    short values[CONFIG_LEN];
    memcpy(values, &state->monitor->getViewport().x, sizeof(short) * 4);
    values[splitDim] /= numCol;
    int offset = 0;
    for(int i = 0; i < numCol; i++) {
        offset = splitEven(state, offset, values,
                state->getArgs()->getOtherDimIndex(), size / numCol + (rem-- > 0 ? 0 : 1), i == numCol - 1);
        values[splitDim - 2] += values[splitDim];
    }
}

void masterPane(LayoutState* state) {
    int size = state->numWindows;
    if(size == 1) {
        full(state);
        return;
    }
    short values[CONFIG_LEN];
    memcpy(values, &state->monitor->getViewport().x, sizeof(short) * 4);
    int dimIndex = state->getArgs()->getDimIndex();
    int dim = values[dimIndex];
    values[dimIndex] = MAX(dim * state->getArgs()->arg, 1);
    int offset = splitEven(state, 0, values, state->getArgs()->getOtherDimIndex(), 1, 0);
    values[dimIndexToPos(dimIndex)] += values[dimIndex];
    values[dimIndex] = dim - values[dimIndex];
    splitEven(state, offset, values, state->getArgs()->getOtherDimIndex(), size - 1, 1);
}
