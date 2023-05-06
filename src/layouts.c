#include <assert.h>
#include <math.h>
#include <string.h>

#include "bindings.h"
#include "boundfunction.h"
#include "globals.h"
#include "masters.h"
#include "monitors.h"
#include "user-events.h"
#include "util/logger.h"
#include "windows.h"
#include "wmfunctions.h"
#include "workspaces.h"

#define TILE_FILTER_FUNC(S, E) isTileable(E)
#define LAYOUT_DEFAULT_BORDER DEFAULT_BORDER_WIDTH

#define TILE_IMPL
#include "layouts.h"
#include "tile.h"


static ArrayList registeredLayouts;

void registerLayout(Layout* layout) {
    addElement(&registeredLayouts, layout);
}
void unregisterLayout(Layout* layout) {
    removeElement(&registeredLayouts, layout, sizeof(Layout));
}

void registerDefaultLayouts() {
    for (int i = 0; i < LEN(defaultLayouts); i++) {
        layoutSaveArgs(defaultLayouts[i]);
        registerLayout(defaultLayouts[i]);
    }
}

const ArrayList* getRegisteredLayouts() {
    return &registeredLayouts;
}

const char* getLayoutName(const Layout* layout) {
    return layout->name;
};

void restoreActiveLayout() {
    if(getActiveLayout())
        getActiveLayout()->args = getActiveLayout()->refArgs;
}

void increaseActiveLayoutArg(int index, int step) {
    increaseLayoutArg(index, step, getActiveLayout());
};

Layout* findLayoutByName(const char* name) {
    FOR_EACH(Layout*, layout, getRegisteredLayouts()) {
        if (strcmp(getLayoutName(layout), name) == 0)
            return layout;
    }
    return NULL;
}

static uint32_t applyMasksToConfig(const WindowInfo* winInfo, const Monitor* m, tile_type_t* values) {
    if (hasMask(winInfo, ROOT_FULLSCREEN_MASK)) {
        values[CONFIG_INDEX_X] = 0;
        values[CONFIG_INDEX_Y] = 0;
        values[CONFIG_INDEX_WIDTH] = getRootWidth();
        values[CONFIG_INDEX_HEIGHT] = getRootHeight();
        values[CONFIG_INDEX_BORDER] = 0;
    }
    else if (hasMask(winInfo, FULLSCREEN_MASK)) {
        for (int i = 0; i < 4; i++)
            values[CONFIG_INDEX_X + i] = ((short*)&m->base)[i];
        values[CONFIG_INDEX_BORDER] = 0;
    }
    else {
        uint32_t mask = 0;
        if (hasMask(winInfo, X_CENTERED_MASK)) {
            values[CONFIG_INDEX_X] = m->view.x + m->view.width / 2;
            mask |= XCB_CONFIG_WINDOW_X;
        }
        if (hasMask(winInfo, Y_CENTERED_MASK)) {
            values[CONFIG_INDEX_Y] = m->view.y + m->view.height / 2;
            mask |= XCB_CONFIG_WINDOW_Y;
        }
        if (hasMask(winInfo, X_MAXIMIZED_MASK)) {
            values[CONFIG_INDEX_WIDTH] = m->view.width;
            mask |= XCB_CONFIG_WINDOW_WIDTH;
        }
        if (hasMask(winInfo, Y_MAXIMIZED_MASK)) {
            values[CONFIG_INDEX_HEIGHT] = m->view.height;
            mask |= XCB_CONFIG_WINDOW_HEIGHT;
        }
        return mask;
    }
    return
        XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
        XCB_CONFIG_WINDOW_BORDER_WIDTH;
}

Rect getRelativeRegion(const Rect bounds, const Rect region, bool percent) {
    Rect config = {0, 0, 0, 0};
    for (int i = 0; i < 4; i++) {
        short fixedValue = (&region.x)[i];
        short refEndPoint = (&bounds.x)[i % 2 + 2];
        (&config.x)[i] = percent & (1 << i) ?
            (&region.x)[i] * (&bounds.x)[i % 2 + 2] / 100 :
            fixedValue < 0 ? fixedValue + refEndPoint : fixedValue ;
        if (i < 2)
            (&config.x)[i] += (&bounds.x)[i];
        else if (!(&config.x)[i])
            (&config.x)[i] = (&bounds.x)[i];
    }
    return config;
}

static void applyTilingOverrideToConfig(const WindowInfo* winInfo, const Monitor* m, tile_type_t* config) {
    if (!getTilingOverrideMask(winInfo))
        return;
    Rect tilingOverride = *getTilingOverride(winInfo);
    Rect bounds = winInfo->dock ? m->base : m->view;
    if (winInfo->dock || getWorkspaceOfWindow(winInfo) || hasPartOfMask(winInfo, ROOT_FULLSCREEN_MASK | FULLSCREEN_MASK))
        for (int i = 0; i <= CONFIG_INDEX_HEIGHT; i++)
            (&bounds.x)[i] = config[i];

    Rect region = getRelativeRegion(bounds, tilingOverride, getTilingOverridePercent(winInfo));

    for (int i = 0; i <= CONFIG_INDEX_HEIGHT; i++)
        if (isTilingOverrideEnabledAtIndex(winInfo, i))
            if (!isTilingOverrideRelativeAtIndex(winInfo, i))
                config[i] = (&region.x)[i];
            else
                config[i] += (&tilingOverride.x)[i];
    if (isTilingOverrideEnabledAtIndex(winInfo, CONFIG_INDEX_BORDER))
        config[CONFIG_INDEX_BORDER] = getTilingOverrideBorder(winInfo);
}

static void prepareTileWindow(const LayoutState* state, const void* winInfo, tile_type_t* values) {
    applyMasksToConfig(winInfo, state->userData, values);
    applyTilingOverrideToConfig(winInfo, state->userData, values);
}

static void tileWindow(const LayoutState* state, const void* winInfo, tile_type_t* values) {
    assert(winInfo);
    int mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
        XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
        XCB_CONFIG_WINDOW_BORDER_WIDTH;

    uint32_t config[CONFIG_LEN] = {0};
    for (int i = 0; i < CONFIG_LEN; i++)
        config[i] = values[i];

    config[CONFIG_INDEX_WIDTH] = MAX(1, (short)config[CONFIG_INDEX_WIDTH]);
    config[CONFIG_INDEX_HEIGHT] = MAX(1, (short)config[CONFIG_INDEX_HEIGHT]);

    configureWindow(((WindowInfo*)winInfo)->id, mask, config);
}

void arrangeNonTileableWindow(const WindowInfo* winInfo, const Monitor* monitor) {
    tile_type_t config[CONFIG_LEN] = {0};
    config[CONFIG_INDEX_BORDER] = DEFAULT_BORDER_WIDTH;
    const Rect * bounds = (winInfo->dock || !getWorkspaceOfWindow(winInfo)) ? &monitor->base : &monitor->view;
    for (int i = 0; i <= CONFIG_INDEX_HEIGHT; i++)
        config[i] = (&bounds->x)[i];


    uint32_t mask = applyMasksToConfig(winInfo, monitor, config);
    mask |= getTilingOverrideMask(winInfo);
    applyTilingOverrideToConfig(winInfo, monitor, config);
    if (isFocusable(winInfo))
        mask |= XCB_CONFIG_WINDOW_BORDER_WIDTH;
    if (mask) {
        uint32_t finalConfig[CONFIG_LEN] = {0};
        for (int i = 0, counter = 0; i < CONFIG_LEN; i++) {
            if (mask & (1 << i))
                finalConfig[counter++] = config[i];
        }
        configureWindow(winInfo->id, mask, finalConfig);
    }
}

static void applyAboveBelowMask(ArrayList* list) {
    WindowID currentAbove = 0;
    WindowID currentBelow = 0;
    FOR_EACH(WindowInfo*, winInfo, list) {
        if (hasMask(winInfo, BELOW_MASK)) {
            lowerWindow(winInfo->id, currentBelow);
            currentBelow = winInfo->id;
        }
        else if (hasMask(winInfo, ABOVE_MASK)) {
            if (!currentAbove) {
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
    if (workspace->dirty || workspace->lastTiledLayout != getLayout(workspace))
        return 1;
    if (isWorkspaceVisible(workspace) && memcmp(&getMonitor(workspace)->view, &workspace->lastBounds, sizeof(Rect)) != 0)
        return 1;
    FOR_EACH(WindowInfo*, winInfo, getWorkspaceWindowStack(workspace)) {
        if ((winInfo->mask ^ winInfo->savedMask) & RETILE_MASKS)
            return 1;
    }
    return 0;
}

void retileAllDirtyWorkspaces() {
    FOR_EACH(Workspace*, workspace, getAllWorkspaces()) {
        if (isWorkspaceVisible(workspace) && getWorkspaceWindowStack(workspace)->size && isWorkspaceDirty(workspace))
            tileWorkspace(workspace);
    }
}

void tileWorkspace(Workspace* workspace) {
    assert(workspace);
    DEBUG("Tiling workspace %d", workspace->id);
    ArrayList* windowStack = getWorkspaceWindowStack(workspace);
    if (!isWorkspaceVisible(workspace) || !windowStack->size) {
        TRACE("Cannot tile workspace; Visible %d; Size %d", !isWorkspaceVisible(workspace), windowStack->size);
        return;
    }
    Monitor* m = getMonitor(workspace);
    Layout* layout = getLayout(workspace);
    workspace->dirty = 0;
    workspace->lastTiledLayout = layout;
    workspace->lastBounds = m->view;

    if (!layout) {
        TRACE("No layout is set\n");
    }
    layoutApply(layout, &m->view.x, getData(windowStack), windowStack->size, prepareTileWindow, tileWindow, m);

    FOR_EACH(WindowInfo*, winInfo, windowStack) {
        if (!isTileable(winInfo))
            arrangeNonTileableWindow(winInfo, m);
    }
    applyAboveBelowMask(windowStack);
    applyEventRules(TILE_WORKSPACE, workspace);
}
