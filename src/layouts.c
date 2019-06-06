/**
 * @file layouts.c
 * @copydoc layouts.h
 */
#include <assert.h>
#include <math.h>
#include <string.h>

#include "bindings.h"
#include "events.h"
#include "globals.h"
#include "layouts.h"
#include "logger.h"
#include "masters.h"
#include "monitors.h"
#include "windows.h"
#include "wmfunctions.h"
#include "workspaces.h"
#include "xsession.h"


#define _LAYOUT_FAMILY(S){.name=""#S,.layoutFunction=S}

Layout LAYOUT_FAMILIES[] = {
    _LAYOUT_FAMILY(full),
    _LAYOUT_FAMILY(column),
    _LAYOUT_FAMILY(masterPane),
};
int NUMBER_OF_LAYOUT_FAMILIES = LEN(LAYOUT_FAMILIES);
Layout DEFAULT_LAYOUTS[] = {
    {.name = "Full", .layoutFunction = full, .args = {.noBorder = 1, .raiseFocused = 1}},
    {.name = "Grid", .layoutFunction = column},
    {.name = "2 Column", .layoutFunction = column, .args = {.dim = 0, .arg = {2}}},
    {.name = "2 Row", .layoutFunction = column, .args = {.dim = 1, .arg = {2}}},
    {.name = "2 Pane", .layoutFunction = column, .args = {.raiseFocused = 1, .dim = 0, .arg = {2}, .limit = 2}},
    {.name = "2 HPane", .layoutFunction = column, .args = {.raiseFocused = 1, .dim = 1, .arg = {2}, .limit = 2}},
    {.name = "Master", .layoutFunction = masterPane, .args = {.arg = {.7}, .argStep = {.1}}},

};
static ArrayList registeredLayouts;


void setActiveLayoutByName(char* name){
    setActiveLayout(findLayoutByName(name));
}
Layout* findLayoutByName(char* name){
    Layout* layout;
    UNTIL_FIRST(layout, &registeredLayouts, !name && !getNameOfLayout(layout) || name && getNameOfLayout(layout) &&
                strcmp(name, getNameOfLayout(layout)) == 0);
    return layout;
}
void registerLayouts(Layout* layouts, int num){
    for(int i = 0; i < num; i++)
        if(!find(&registeredLayouts, &layouts[i], 0)){
            saveLayoutArgs(&layouts[i]);
            addToList(&registeredLayouts, &layouts[i]);
        }
}

void saveLayoutArgs(Layout* layout){
    memcpy(&layout->refArgs, &layout->args, sizeof(LayoutArgs));
}
void restoreActiveLayout(void){
    Layout* l = getActiveLayout();
    if(l){
        memcpy(&l->args, &l->refArgs, sizeof(LayoutArgs));
        retile();
    }
}

void increaseActiveLayoutArg(int index, int step){
    Layout* l = getActiveLayout();
    if(l){
        switch(index){
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
            case LAYOUT_RAISE_FOCUSED:
                l->args.raiseFocused = !l->args.raiseFocused;
                break;
            case LAYOUT_LOWER_WINDOWS:
                l->args.lowerWindows = !l->args.lowerWindows;
                break;
            case LAYOUT_TRANSFORM:
                l->args.transform = WRAP(l->args.transform + step, TRANSFORM_LEN) ;
                break;
            case LAYOUT_ARG:
                l->args.arg[0] += l->args.argStep[0] * step;
                break;
        }
        retile();
    }
}

int NUMBER_OF_DEFAULT_LAYOUTS = LEN(DEFAULT_LAYOUTS);



static int getDimIndex(LayoutArgs* args){
    return args->dim == 0 ? CONFIG_WIDTH : CONFIG_HEIGHT;
}
static int getOtherDimIndex(LayoutArgs* args){
    return args->dim == 0 ? CONFIG_HEIGHT : CONFIG_WIDTH;
}
static int dimIndexToPos(int dim){
    return dim - 2;
}

char* getNameOfLayout(Layout* layout){
    return layout ? layout->name : NULL;
}
void addLayoutsToWorkspace(int workspaceIndex, Layout* layout, int num){
    if(num && !isNotEmpty(&getWorkspaceByIndex(workspaceIndex)->layouts))
        getWorkspaceByIndex(workspaceIndex)->activeLayout = layout;
    for(int n = 0; n < num; n++)
        addToList(&getWorkspaceByIndex(workspaceIndex)->layouts, &layout[n]);
}
void clearLayoutsOfWorkspace(int workspaceIndex){
    clearList(&getWorkspaceByIndex(workspaceIndex)->layouts);
    getWorkspaceByIndex(workspaceIndex)->activeLayout = NULL;
}

int getNumberOfWindowsToTile(ArrayList* windowStack, LayoutArgs* args){
    assert(windowStack != NULL);
    int size = 0;
    for(int i = 0; i < getSize(windowStack); i++){
        WindowInfo* winInfo = getElement(windowStack, i);
        if(args && args->limit && size == args->limit)break;
        if(isTileable(winInfo)){
            size++;
        }
    }
    return size;
}
void transformConfig(LayoutState* state, int config[CONFIG_LEN]){
    Monitor* m = state->monitor;
    if(state->args){
        int endX = m->view.x * 2 + m->view.width;
        int endY = m->view.y * 2 + m->view.height;
        switch(state->args->transform){
            case NONE:
                break;
            case REFLECT_HOR:
                config[CONFIG_X] = endX - (config[CONFIG_X] + config[CONFIG_WIDTH]);
                break;
            case REFLECT_VERT:
                config[CONFIG_Y] = endY - (config[CONFIG_Y] + config[CONFIG_HEIGHT]);
                break;
            case ROT_180:
                config[CONFIG_X] = endX - (config[CONFIG_X] + config[CONFIG_WIDTH]);
                config[CONFIG_Y] = endY - (config[CONFIG_Y] + config[CONFIG_HEIGHT]);
                break;
        }
    }
}
static void applyMasksToConfig(Monitor* m, int* values, WindowInfo* winInfo){
    if(hasMask(winInfo, ROOT_FULLSCREEN_MASK)){
        values[CONFIG_X] = 0;
        values[CONFIG_Y] = 0;
        values[CONFIG_WIDTH] = getRootWidth();
        values[CONFIG_HEIGHT] = getRootHeight();
    }
    else if(hasMask(winInfo, FULLSCREEN_MASK)){
        for(int i = 0; i < 4; i++)
            values[CONFIG_X + i] = (&m->base.x)[i];
    }
    else {
        if(hasMask(winInfo, X_MAXIMIZED_MASK))
            values[CONFIG_WIDTH] = m->view.width;
        if(hasMask(winInfo, Y_MAXIMIZED_MASK))
            values[CONFIG_HEIGHT] = m->view.height;
    }
}

static int adjustBorders(LayoutState* state, WindowInfo* winInfo, int config[CONFIG_LEN], int mask){
    if(hasMask(winInfo, INPUT_ONLY_MASK)){
        mask &= ~XCB_CONFIG_WINDOW_BORDER_WIDTH;
        for(int i = CONFIG_BORDER; i < CONFIG_LEN - 1; i++)
            config[i] = config[i + 1];
    }
    else if(state->args){
        config[CONFIG_BORDER] = state->args->noBorder ? 0 : DEFAULT_BORDER_WIDTH;
        if(!state->args->noAdjustForBorders){
            config[CONFIG_WIDTH] -= config[CONFIG_BORDER] * 2;
            config[CONFIG_HEIGHT] -= config[CONFIG_BORDER] * 2;
        }
    }
    return mask;
}
static Rect getMonitorBoundsForWindow(WindowInfo* winInfo, Monitor* m){
    if(hasMask(winInfo, ROOT_FULLSCREEN_MASK))
        return (Rect){0, 0, getRootWidth(), getRootHeight()};
    else if(hasMask(winInfo, FULLSCREEN_MASK))
        return m->base;
    else
        return m->view;
}
void configureWindow(LayoutState* state, WindowInfo* winInfo, short values[CONFIG_LEN]){
    assert(winInfo);
    int mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
               XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
               XCB_CONFIG_WINDOW_BORDER_WIDTH | XCB_CONFIG_WINDOW_STACK_MODE;
    int config[CONFIG_LEN] = {0};
    config[CONFIG_STACK] = state->args && state->args->lowerWindows ? XCB_STACK_MODE_BELOW : XCB_STACK_MODE_ABOVE;
    for(int i = 0; i <= CONFIG_HEIGHT; i++)
        config[i] = values[i];
    transformConfig(state, config);
    applyMasksToConfig(state->monitor, config, winInfo);
    mask = adjustBorders(state, winInfo, config, mask);
    Rect bounds = getMonitorBoundsForWindow(winInfo, state->monitor);
    for(int i = 0; i <= CONFIG_HEIGHT; i++){
        if(isTilingOverrideEnabledAtIndex(winInfo, i)){
            int fixedValue = getTilingOverride(winInfo)[i];
            int refPoint = (&bounds.x)[i];
            int refEndPoint = refPoint + (&bounds.x)[i % 2 + 2];
            config[i] = fixedValue < 0 ? fixedValue + refEndPoint : fixedValue ;
            if(i < CONFIG_WIDTH)
                config[i] += (&bounds.x)[i];
        }
        if(state->args){
            if(i < CONFIG_WIDTH)
                config[i] += (&state->args->leftPadding)[i];
            else
                config[i] -= (&state->args->rightPadding)[i % 2] + (&state->args->leftPadding)[i % 2];
        }
    }
    if(isTilingOverrideEnabledAtIndex(winInfo, CONFIG_BORDER))
        config[CONFIG_BORDER] = MAX(0, getTilingOverride(winInfo)[CONFIG_BORDER]);
    config[CONFIG_WIDTH] = MAX(1, config[CONFIG_WIDTH]);
    config[CONFIG_HEIGHT] = MAX(1, config[CONFIG_HEIGHT]);
    assert(winInfo->id);
#ifdef DEBUG
    LOG(LOG_LEVEL_DEBUG, "Config %d: mask %d %d\n", winInfo->id, mask, __builtin_popcount(mask));
    PRINT_ARR("values", config, CONFIG_LEN, "\n");
    catchError(xcb_configure_window_checked(dis, winInfo->id, mask, config));
#else
    xcb_configure_window(dis, winInfo->id, mask, config);
#endif
}



void cycleLayouts(int dir){
    ArrayList* layouts = &getActiveWorkspace()->layouts;
    if(!isNotEmpty(layouts))return;
    int pos = getNextIndex(layouts, getActiveWorkspace()->layoutOffset, dir);
    setActiveLayout(getElement(layouts, pos));
    getActiveWorkspace()->layoutOffset = pos;
}
int toggleLayout(Layout* layout){
    if(layout != getActiveLayout()){
        setActiveLayout(layout);
    }
    else {
        cycleLayouts(0);
        if(layout == getActiveLayout())
            return 0;
    }
    return 1;
}
void retile(void){
    tileWorkspace(getActiveWorkspaceIndex());
}

void tileWorkspace(int index){
    applyEventRules(TileWorkspace, NULL);
    if(!isWorkspaceVisible(index)){
        LOG(LOG_LEVEL_DEBUG, "refusing to tile invisible workspace %d\n", index);
        return;
    }
    Workspace* workspace = getWorkspaceByIndex(index);
    applyLayout(workspace);
    ArrayList* list = getWindowStack(workspace);
    LayoutState dummyState = {.monitor = getMonitorFromWorkspace(workspace)};
    for(int i = 0; i < getSize(list); i++){
        WindowInfo* winInfo;
        winInfo = getElement(list, i);
        if(isInteractable(winInfo) && !isTileable(winInfo))
            if(hasPartOfMask(winInfo, MAXIMIZED_MASK | FULLSCREEN_MASK | ROOT_FULLSCREEN_MASK)){
                short config[CONFIG_LEN] = {0};
                configureWindow(&dummyState, winInfo, config);
            }
            else if(!hasMask(winInfo, INPUT_ONLY_MASK))
                xcb_configure_window(dis, winInfo->id, XCB_CONFIG_WINDOW_BORDER_WIDTH, &DEFAULT_BORDER_WIDTH);
        if(hasMask(winInfo, BELOW_MASK))
            lowerWindowInfo(winInfo);
        winInfo = getElement(list, getSize(list) - i - 1);
        if(hasMask(winInfo, ABOVE_MASK))
            raiseWindowInfo(winInfo);
    }
}

void applyLayout(Workspace* workspace){
    Layout* layout = getActiveLayoutOfWorkspace(workspace->id);
    if(!layout){
        LOG(LOG_LEVEL_TRACE, "workspace %d does not have a layout; skipping \n", workspace->id);
        return;
    }
    Monitor* m = getMonitorFromWorkspace(workspace);
    assert(m);
    ArrayList* windowStack = getWindowStack(workspace);
    if(!isNotEmpty(windowStack)){
        LOG(LOG_LEVEL_TRACE, "no windows in workspace\n");
        return;
    }
    ArrayList* last = NULL;
    int size = getNumberOfWindowsToTile(windowStack, &layout->args);
    LayoutState state = {.args = &layout->args, .monitor = m, .numWindows = size, .stack = windowStack, .last = last};
    if(size)
        if(layout->layoutFunction){
            LOG(LOG_LEVEL_DEBUG, "using '%s' layout: num win %d\n", layout->name, size);
            layout->layoutFunction(&state);
        }
        else
            LOG(LOG_LEVEL_WARN, "WARNING there is not a set layout function\n");
    else
        LOG(LOG_LEVEL_TRACE, "there are no windows to tile\n");
    if(layout->args.raiseFocused){
        FOR_EACH_REVERSED(WindowInfo*, winInfo, getActiveMasterWindowStack()){
            if(getWorkspaceOfWindow(winInfo) == workspace)
                raiseWindowInfo(winInfo);
        }
    }
}

static int splitEven(LayoutState* state, ArrayList* stack, int offset, short const* baseValues, int dim, int num,
                     int last){
    assert(stack);
    assert(num);
    short values[CONFIG_LEN];
    memcpy(values, baseValues, sizeof(values));
    int sizePerWindow = values[dim] / num;
    int rem = values[dim] % num;
    LOG(LOG_LEVEL_DEBUG, "tiling at most %d windows size %d %d\n", num, sizePerWindow, dim);
    int i = offset;
    int count = 0;
    while(i < getSize(stack)){
        WindowInfo* winInfo = getElement(stack, i++);
        if(!isTileable(winInfo))continue;
        count++;
        values[dim] = sizePerWindow + (rem-- > 0 ? 1 : 0);
        configureWindow(state, winInfo, values);
        if(count == num)break;
        values[dim - 2] += values[dim]; //convert width/height index to x/y
    }
    if(last){
        state->args->lowerWindows = !state->args->lowerWindows;
        for(; i < getSize(stack); i++){
            WindowInfo* winInfo = getElement(stack, i);
            if(isTileable(winInfo))
                configureWindow(state, winInfo, values);
        }
        state->args->lowerWindows = !state->args->lowerWindows;
    }
    return i;
}

void full(LayoutState* state){
    short int values[CONFIG_LEN];
    memcpy(&values, &state->monitor->view.x, sizeof(short int) * 4);
    FOR_EACH_REVERSED(WindowInfo*, winInfo, state->stack){
        if(isTileable(winInfo))
            configureWindow(state, winInfo, values);
    }
}

void column(LayoutState* state){
    int size = state->numWindows;
    if(size == 1){
        full(state);
        return;
    }
    int numCol = state->args->arg[0] == 0 ? log2(size + 1) : state->args->arg[0];
    int rem = numCol - size % numCol;
    // split of width(0) or height(1)
    int splitDim = getDimIndex(state->args);
    short values[CONFIG_LEN];
    memcpy(values, &state->monitor->view.x, sizeof(short) * 4);
    values[splitDim] /= numCol;
    ArrayList* windowStack = state->stack;
    int offset = 0;
    for(int i = 0; i < numCol; i++){
        offset = splitEven(state, windowStack, offset, values,
                           getOtherDimIndex(state->args), size / numCol + (rem-- > 0 ? 0 : 1), i == numCol - 1);
        values[splitDim - 2] += values[splitDim];
    }
}

void masterPane(LayoutState* state){
    int size = state->numWindows;
    if(size == 1){
        full(state);
        return;
    }
    ArrayList* windowStack = state->stack;
    short values[CONFIG_LEN];
    memcpy(values, &state->monitor->view.x, sizeof(short) * 4);
    int dimIndex = getDimIndex(state->args);
    int dim = values[dimIndex];
    values[dimIndex] = MAX(dim * state->args->arg[0], 1);
    configureWindow(state, getHead(windowStack), values);
    values[dimIndexToPos(dimIndex)] += values[dimIndex];
    values[dimIndex] = dim - values[dimIndex];
    splitEven(state, windowStack, 1, values, getOtherDimIndex(state->args), size - 1, 1);
}
