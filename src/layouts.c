#include <assert.h>
#include <math.h>
#include <string.h>

#include "bindings.h"
#include "boundfunction.h"
#include "globals.h"
#include "layouts.h"
#include "util/logger.h"
#include "masters.h"
#include "monitors.h"
#include "user-events.h"
#include "windows.h"
#include "wmfunctions.h"
#include "workspaces.h"

Layout FULL = {.name = "Full", .func = full, .args = {.noBorder = 1}},
       GRID = {.name = "Grid", .func = column},
       TWO_COL = {.name = "2 Col", .func = column, .args = {.dim = 0, .arg = 2}},
       THREE_COL = {.name = "2 Col", .func = column, .args = {.dim = 0, .arg = 3}},
       TWO_ROW = {.name = "2 Row", .func = column, .args = {.dim = 1, .arg = 2}},
       TWO_PANE = {.name = "2 Pane", .func = column, .args = {.limit = 2, .dim = 0, .arg = 2}},
       TWO_PANE_H = {.name = "2 HPane", .func = column, .args = {.limit = 2, .dim = 1, .arg = 2 }},
       MASTER = {.name = "Master", .func = masterPane, .args = {.arg = .7, .argStep = .1}},
       MASTER_H = {.name = "Master", .func = masterPane, .args = {.dim = 1, .arg = .7, .argStep = .1}},
       TWO_MASTER = {.name = "2 Master", .func = masterPane, .args = {.limit = 2, .arg = .7, .argStep = .1}},
       TWO_MASTER_FLIPPED = {.name = "2 Master Flipped", .func = masterPane, .args = {.limit = 2, .transform = ROT_180, .arg = .7, .argStep = .1}},
       TWO_MASTER_H = {.name = "2 HMaster", .func = masterPane, .args = {.limit = 2, .dim = 1, .arg = .7, .argStep = .1}};
static Layout* defaultLayouts[] = {&FULL, &GRID, &TWO_ROW, &TWO_COL, &THREE_COL, &TWO_PANE, &TWO_PANE_H, &MASTER, &TWO_MASTER, &TWO_MASTER_FLIPPED,  &TWO_MASTER_H};
static ArrayList registeredLayouts;

void registerLayout(Layout* layout) {
    addElement(&registeredLayouts, layout);
}
void unregisterLayout(Layout* layout) {
    removeElement(&registeredLayouts, layout, sizeof(Layout));
}

void registerDefaultLayouts() {
    for(int i = 0; i < LEN(defaultLayouts); i++)
        registerLayout(defaultLayouts[i]);
}
const ArrayList* getRegisteredLayouts() {
    return &registeredLayouts;
}


Layout* findLayoutByName(const char* name) {
    FOR_EACH(Layout*, layout, getRegisteredLayouts()) {
        if(strcmp(layout->name, name) == 0)
            return layout;
    }
    return NULL;
}

void increaseLayoutArg(int index, int step, Layout* l) {
    if(l) {
        switch(index) {
            case LAYOUT_LIMIT:
                l->args.limit += step;
                break;
            case LAYOUT_PADDING:
                l->args.leftPadding += step;
                l->args.topPadding += step;
                l->args.rightPadding += step;
                l->args.bottomPadding += step;
                break;
            case LAYOUT_LEFT_PADDING:
                l->args.leftPadding += step;
                break;
            case LAYOUT_TOP_PADDING:
                l->args.topPadding += step;
                break;
            case LAYOUT_RIGHT_PADDING:
                l->args.rightPadding += step;
                break;
            case LAYOUT_BOTTOM_PADDING:
                l->args.bottomPadding += step;
                break;
            case LAYOUT_NO_BORDER:
                l->args.noBorder = !l->args.noBorder;
                break;
            case LAYOUT_NO_ADJUST_FOR_BORDERS:
                l->args.noAdjustForBorders = !l->args.noAdjustForBorders;
                break;
            case LAYOUT_DIM:
                l->args.dim = !l->args.dim;
                break;
            case LAYOUT_TRANSFORM:
                l->args.transform = (Transform)((l->args.transform + step % TRANSFORM_LEN + TRANSFORM_LEN) % TRANSFORM_LEN) ;
                break;
            case LAYOUT_ARG:
                l->args.arg += l->args.argStep * step;
                break;
        }
        retile();
    }
}

static int dimIndexToPos(int dim) {
    return dim - 2;
}


void transformConfig(const LayoutArgs* args, const Monitor* m, uint32_t config[CONFIG_LEN]) {
    if(args) {
        int endX = m->view.x * 2 + m->view.width;
        int endY = m->view.y * 2 + m->view.height;
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
static void adjustBorders(const LayoutState* state, uint32_t config[CONFIG_LEN]) {
    if(state->args) {
        config[CONFIG_INDEX_BORDER] = state->args->noBorder ? 0 : DEFAULT_BORDER_WIDTH;
        if(!state->args->noAdjustForBorders) {
            config[CONFIG_INDEX_WIDTH] -= config[CONFIG_INDEX_BORDER] * 2;
            config[CONFIG_INDEX_HEIGHT] -= config[CONFIG_INDEX_BORDER] * 2;
        }
    }
}
static uint32_t applyMasksToConfig(const WindowInfo* winInfo, const Monitor* m, uint32_t* values) {
    if(hasMask(winInfo, ROOT_FULLSCREEN_MASK)) {
        values[CONFIG_INDEX_X] = 0;
        values[CONFIG_INDEX_Y] = 0;
        values[CONFIG_INDEX_WIDTH] = getRootWidth();
        values[CONFIG_INDEX_HEIGHT] = getRootHeight();
        values[CONFIG_INDEX_BORDER] = 0;
    }
    else if(hasMask(winInfo, FULLSCREEN_MASK)) {
        for(int i = 0; i < 4; i++)
            values[CONFIG_INDEX_X + i] = ((short*)&m->base)[i];
        values[CONFIG_INDEX_BORDER] = 0;
    }
    else {
        uint32_t mask = 0;
        if(hasMask(winInfo, X_CENTERED_MASK)) {
            values[CONFIG_INDEX_X] = m->view.x + m->view.width / 2;
            mask |= XCB_CONFIG_WINDOW_X;
        }
        if(hasMask(winInfo, Y_CENTERED_MASK)) {
            values[CONFIG_INDEX_Y] = m->view.y + m->view.height / 2;
            mask |= XCB_CONFIG_WINDOW_Y;
        }
        if(hasMask(winInfo, X_MAXIMIZED_MASK)) {
            values[CONFIG_INDEX_WIDTH] = m->view.width;
            mask |= XCB_CONFIG_WINDOW_WIDTH;
        }
        if(hasMask(winInfo, Y_MAXIMIZED_MASK)) {
            values[CONFIG_INDEX_HEIGHT] = m->view.height;
            mask |= XCB_CONFIG_WINDOW_HEIGHT;
        }
        return mask;
    }
    return
        XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
        XCB_CONFIG_WINDOW_BORDER_WIDTH;
}

static inline Rect getRelativeRegion(const Rect bounds, const Rect region, bool percent) {
    Rect config = {0, 0, 0, 0};
    for(int i = 0; i < 4; i++) {
        short fixedValue = (&region.x)[i];
        short refEndPoint = (&bounds.x)[i % 2 + 2];
        (&config.x)[i] = percent & (1 << i) ?
            (&region.x)[i] * (&bounds.x)[i % 2 + 2] / 100 :
            fixedValue < 0 ? fixedValue + refEndPoint : fixedValue ;
        if(i < 2)
            (&config.x)[i] += (&bounds.x)[i];
        else if(!(&config.x)[i])
            (&config.x)[i] = (&bounds.x)[i];
    }
    return config;
}
static void applyTilingOverrideToConfig(const WindowInfo* winInfo, const Monitor* m, uint32_t* config) {
    if(!getTilingOverrideMask(winInfo))
        return;
    Rect tilingOverride = *getTilingOverride(winInfo);
    Rect bounds = m->view;
    if(winInfo->dock || getWorkspaceOfWindow(winInfo) || hasPartOfMask(winInfo, ROOT_FULLSCREEN_MASK | FULLSCREEN_MASK))
        for(int i = 0; i < 5; i++)
            (&bounds.x)[i] = config[i];
    Rect region = getRelativeRegion(bounds, tilingOverride, getTilingOverridePercent(winInfo));
    for(int i = 0; i <= CONFIG_INDEX_HEIGHT; i++)
        if(isTilingOverrideEnabledAtIndex(winInfo, i))
            if(!isTilingOverrideRelativeAtIndex(winInfo, i))
                config[i] = (&region.x)[i];
            else
                config[i] += (&tilingOverride.x)[i];
    if(isTilingOverrideEnabledAtIndex(winInfo, CONFIG_INDEX_BORDER))
        config[CONFIG_INDEX_BORDER] = getTilingOverrideBorder(winInfo);
}

void tileWindow(const LayoutState* state, const WindowInfo* winInfo, const short values[CONFIG_LEN]) {
    assert(winInfo);
    int mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
        XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
        XCB_CONFIG_WINDOW_BORDER_WIDTH ;
    uint32_t config[CONFIG_LEN] = {0};
    for(int i = 0; i <= CONFIG_INDEX_HEIGHT; i++)
        config[i] = values[i];
    transformConfig(state->args, state->monitor, config);
    applyMasksToConfig(winInfo, state->monitor, config);
    adjustBorders(state, config);
    applyTilingOverrideToConfig(winInfo, state->monitor, config);
    if(state->args)
        for(int i = 0; i <= CONFIG_INDEX_HEIGHT; i++) {
            if(i < CONFIG_INDEX_WIDTH)
                config[i] += (&state->args->leftPadding)[i];
            else
                config[i] -= (&state->args->rightPadding)[i % 2] + (&state->args->leftPadding)[i % 2];
        }
    config[CONFIG_INDEX_WIDTH] = MAX(1, (short)config[CONFIG_INDEX_WIDTH]);
    config[CONFIG_INDEX_HEIGHT] = MAX(1, (short)config[CONFIG_INDEX_HEIGHT]);
    assert(winInfo->id);
    configureWindow(winInfo->id, mask, config);
}

void arrangeNonTileableWindow(const WindowInfo* winInfo, const Monitor* monitor) {
    uint32_t config[CONFIG_LEN] = {0};
    config[CONFIG_INDEX_BORDER] = DEFAULT_BORDER_WIDTH;
    if(winInfo->dock || !getWorkspaceOfWindow(winInfo))
        copyTo(&monitor->base, 0, config);
    else
        copyTo(&monitor->view, 0, config);
    uint32_t mask = applyMasksToConfig(winInfo, monitor, config);
    mask |= getTilingOverrideMask(winInfo);
    applyTilingOverrideToConfig(winInfo, monitor, config);
    if(isFocusable(winInfo))
        mask |= XCB_CONFIG_WINDOW_BORDER_WIDTH;
    if(mask) {
        INFO("PreConfig %d: mask %d (%d bits)", winInfo->id, mask, __builtin_popcount(mask));
        uint32_t finalConfig[CONFIG_LEN] = {0};
        for(int i = 0, counter = 0; i < CONFIG_LEN; i++) {
            if(mask & (1 << i))
                finalConfig[counter++] = config[i];
        }
        configureWindow(winInfo->id, mask, finalConfig);
    }
}

static void applyAboveBelowMask(ArrayList* list) {
    WindowID currentAbove = 0;
    WindowID currentBelow = 0;
    FOR_EACH(WindowInfo*, winInfo, list) {
        if(hasMask(winInfo, BELOW_MASK)) {
            lowerWindow(winInfo->id, currentBelow);
            currentBelow = winInfo->id;
        }
        else if(hasMask(winInfo, ABOVE_MASK)) {
            if(!currentAbove) {
                raiseWindowInfo(winInfo, 0);
            }
            else
                lowerWindow(winInfo->id, currentAbove);
            currentAbove = winInfo->id;
        }
    }
}

void retile(void) {
    tileWorkspace(getActiveWorkspace());
}

bool isWorkspaceDirty(Workspace* workspace) {
    if(workspace->dirty || workspace->lastTiledLayout != getLayout(workspace))
        return 1;
    if(isWorkspaceVisible(workspace) && memcmp(&getMonitor(workspace)->view, &workspace->lastBounds, sizeof(Rect)) != 0)
        return 1;
    FOR_EACH(WindowInfo*, winInfo, getWorkspaceWindowStack(workspace)) {
        if((winInfo->mask ^ winInfo->savedMask) & RETILE_MASKS)
            return 1;
    }
    return 0;
}
void retileAllDirtyWorkspaces() {
    FOR_EACH(Workspace*, workspace, getAllWorkspaces()) {
        if(isWorkspaceVisible(workspace) && getWorkspaceWindowStack(workspace)->size && isWorkspaceDirty(workspace))
            tileWorkspace(workspace);
    }
}


void tileWorkspace(Workspace* workspace) {
    assert(workspace);
    DEBUG("Tiling workspace %d", workspace->id);
    ArrayList* windowStack = getWorkspaceWindowStack(workspace);
    if(!isWorkspaceVisible(workspace) || !windowStack->size) {
        TRACE("Cannot tile workspace; Visibile %d; Size %d", !isWorkspaceVisible(workspace), windowStack->size);
        return;
    }
    Monitor* m = getMonitor(workspace);
    Layout* layout = getLayout(workspace);
    workspace->dirty = 0;
    workspace->lastTiledLayout = layout;
    workspace->lastBounds = m->view;
    if(layout) {
        int maxWindowToTile = 0;
        FOR_EACH(WindowInfo*, winInfo, windowStack) {
            if(isTileable(winInfo))
                maxWindowToTile++;
        }
        if(layout->args.limit)
            maxWindowToTile = MIN(maxWindowToTile, layout->args.limit);
        LayoutState state = {.args = &layout->args, .monitor = m, .numWindows = maxWindowToTile, .stack = windowStack};
        if(maxWindowToTile)
            if(layout->func) {
                DEBUG("using '%s' layout: num win %d (max %d)", layout->name, maxWindowToTile,
                    layout->args.limit);
                layout->func(&state);
            }
            else
                WARN("WARNING there is not a set layout function");
        else
            TRACE("there are no windows to tile");
    }
    else if(!layout)
        TRACE("workspace %d does not have a layout; skipping ", workspace->id);
    FOR_EACH(WindowInfo*, winInfo, windowStack) {
        if(!isTileable(winInfo))
            arrangeNonTileableWindow(winInfo, m);
    }
    applyAboveBelowMask(windowStack);
    applyEventRules(TILE_WORKSPACE, workspace);
}

static uint32_t splitEven(LayoutState* state, int offset, short const* baseValues, int dim, int num,
    int last) {
    assert(num);
    short values[CONFIG_LEN];
    memcpy(values, baseValues, sizeof(values));
    int sizePerWindow = values[dim] / num;
    int rem = values[dim] % num;
    DEBUG("tiling at most %d windows size %d %d", num, sizePerWindow, dim);
    uint32_t i = offset;
    int count = 0;
    while(i < state->stack->size) {
        WindowInfo* winInfo = getElement(state->stack, i++);
        if(!isTileable(winInfo))continue;
        count++;
        values[dim] = sizePerWindow + (rem-- > 0 ? 1 : 0);
        tileWindow(state, winInfo, values);
        if(count == num)break;
        values[dim - 2] += values[dim]; //convert width/height index to x/y
    }
    if(last) {
        LayoutState copy = *state;
        for(; i < state->stack->size; i++) {
            WindowInfo* winInfo = getElement(state->stack, i);
            if(isTileable(winInfo))
                tileWindow(&copy, winInfo, values);
        }
    }
    return i;
}

void full(LayoutState* state) {
    short int values[CONFIG_LEN];
    memcpy(&values, &state->monitor->view.x, sizeof(short int) * 4);
    for(int i = state->stack->size - 1; i >= 0; i--) {
        WindowInfo* winInfo = getElement(state->stack, i);
        if(isTileable(winInfo))
            tileWindow(state, winInfo, values);
    }
}


int getDimIndex(LayoutArgs* args) {
    return args->dim == 0 ? CONFIG_INDEX_WIDTH : CONFIG_INDEX_HEIGHT;
}
int getOtherDimIndex(LayoutArgs* args) {
    return args->dim == 0 ? CONFIG_INDEX_HEIGHT : CONFIG_INDEX_WIDTH;
}

void column(LayoutState* state) {
    int size = state->numWindows;
    if(size == 1) {
        full(state);
        return;
    }
    int numCol = MAX(size, state->args->arg == 0 ? 31 - __builtin_clz(size + 1) : state->args->arg);
    int rem = numCol - size % numCol;
    // split of width(0) or height(1)
    int splitDim = getDimIndex(state->args);
    short values[CONFIG_LEN];
    memcpy(values, &state->monitor->view.x, sizeof(short) * 4);
    values[splitDim] /= numCol;
    int offset = 0;
    for(int i = 0; i < numCol; i++) {
        offset = splitEven(state, offset, values,
                getOtherDimIndex(state->args), size / numCol + (rem-- > 0 ? 0 : 1), i == numCol - 1);
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
    memcpy(values, &state->monitor->view.x, sizeof(short) * 4);
    int dimIndex = getDimIndex(state->args);
    int dim = values[dimIndex];
    values[dimIndex] = MAX(dim * state->args->arg, 1);
    int offset = splitEven(state, 0, values, getOtherDimIndex(state->args), 1, 0);
    values[dimIndexToPos(dimIndex)] += values[dimIndex];
    values[dimIndex] = dim - values[dimIndex];
    splitEven(state, offset, values, getOtherDimIndex(state->args), size - 1, 1);
}
