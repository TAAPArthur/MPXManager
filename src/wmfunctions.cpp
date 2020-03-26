#include <assert.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include <X11/extensions/XInput2.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xinput.h>

#include "bindings.h"
#include "globals.h"
#include "logger.h"
#include "masters.h"
#include "monitors.h"
#include "threads.h"
#include "time.h"
#include "user-events.h"
#include "window-properties.h"
#include "windows.h"
#include "wmfunctions.h"
#include "workspaces.h"
#include "xsession.h"



bool postRegisterWindow(WindowInfo* winInfo, bool newlyCreated) {
    if(newlyCreated)
        winInfo->setCreationTime(getTime());
    else
        loadWindowHints(winInfo);
    if(winInfo->hasMask(MAPPABLE_MASK)) {
        LOG(LOG_LEVEL_DEBUG, "Window is mappable %d", winInfo->getID());
        if(!applyEventRules(CLIENT_MAP_ALLOW, winInfo))
            return 0;
    }
    return applyEventRules(POST_REGISTER_WINDOW, winInfo);
}

bool registerWindow(WindowID win, WindowID parent, xcb_get_window_attributes_reply_t* attr) {
    assert(!getAllWindows().find(win) && "Window registered exists");
    LOG(LOG_LEVEL_TRACE, "processing %d (%x)", win, win);
    return registerWindow(new WindowInfo(win, parent), attr);
}
bool registerWindow(WindowInfo* winInfo, xcb_get_window_attributes_reply_t* attr) {
    bool freeAttr = !attr;
    bool newlyCreated = !attr;
    if(!attr)
        attr = xcb_get_window_attributes_reply(dis, xcb_get_window_attributes(dis, winInfo->getID()), NULL);
    if(attr) {
        if(attr->override_redirect)
            winInfo->markAsOverrideRedirect();
        if(attr->_class == XCB_WINDOW_CLASS_INPUT_ONLY)
            winInfo->markAsInputOnly();
        if(attr->map_state != XCB_MAP_STATE_UNMAPPED)
            winInfo->addMask(MAPPABLE_MASK | MAPPED_MASK);
        if(freeAttr)
            free(attr);
    }
    else {
        LOG(LOG_LEVEL_DEBUG, "Could not load attributes %d ", winInfo->getID());
        delete winInfo;
        return 0;
    }
    if(!applyEventRules(PRE_REGISTER_WINDOW, winInfo)) {
        delete winInfo;
        return 0;
    }
    LOG(LOG_LEVEL_DEBUG, "Registering %d (%x)", winInfo->getID(), winInfo->getID());
    getAllWindows().add(winInfo);
    return postRegisterWindow(winInfo, newlyCreated);
}
void scan(xcb_window_t baseWindow) {
    assert(baseWindow);
    LOG(LOG_LEVEL_TRACE, "Scanning children of window %d", baseWindow);
    xcb_query_tree_reply_t* reply;
    reply = xcb_query_tree_reply(dis, xcb_query_tree(dis, baseWindow), 0);
    if(reply) {
        xcb_get_window_attributes_reply_t* attr;
        int numberOfChildren = xcb_query_tree_children_length(reply);
        LOG(LOG_LEVEL_TRACE, "detected %d kids", numberOfChildren);
        xcb_window_t* children = xcb_query_tree_children(reply);
        xcb_get_window_attributes_cookie_t cookies[numberOfChildren];
        for(int i = 0; i < numberOfChildren; i++)
            cookies[i] = xcb_get_window_attributes(dis, children[i]);
        // iterate in bottom to top order
        for(int i = 0; i < numberOfChildren; i++) {
            LOG(LOG_LEVEL_TRACE, "processing child %d", children[i]);
            attr = xcb_get_window_attributes_reply(dis, cookies[i], NULL);
            if(registerWindow(children[i], baseWindow, attr)) {
                xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, children[i]), NULL);
                if(reply) {
                    getWindowInfo(children[i])->setGeometry(&reply->x);
                    free(reply);
                }
            }
            if(attr)
                free(attr);
        }
        free(reply);
    }
}

static bool focusNextVisibleWindow(Master* master, WindowInfo* ignore) {
    for(WindowInfo* winInfo : master->getWindowStack())
        if(winInfo != ignore && winInfo->isNotInInvisibleWorkspace() && winInfo->isFocusable() && focusWindow(winInfo))
            return 1;
    return 0;
}
void updateFocusForAllMasters(WindowInfo* winInfo) {
    DEBUG("Trying to update focus from " << (winInfo ? winInfo->getID() : 0));
    for(Master* master : getAllMasters())
        if(!winInfo || master->getFocusedWindow() == winInfo)
            if(!focusNextVisibleWindow(master, winInfo))
                DEBUG("Could not find window to update focus to out of " << master->getWindowStack());
            else
                DEBUG("Updated focus for master" << *master);
}

bool unregisterWindow(WindowInfo* winInfo, bool destroyed) {
    if(!winInfo)
        return 0;
    WindowID winToRemove = winInfo->getID();
    LOG(LOG_LEVEL_DEBUG, "window %d has been removed", winToRemove);
    if(!destroyed)
        unregisterForWindowEvents(winInfo->getID());
    bool result = 0;
    if(applyEventRules(UNREGISTER_WINDOW, winInfo)) {
        result = getAllWindows().removeElement(winInfo);
        if(winInfo)
            delete winInfo;
    }
    return result;
}


static void waitForWindowToDie(WindowID id) {
    lock();
    WindowInfo* winInfo = getWindowInfo(id);
    int hasPingMask = 0;
    if(winInfo) {
        winInfo->setPingTimeStamp(getTime());
        hasPingMask = winInfo->hasMask(WM_PING_MASK);
    }
    unlock();
    while(!isShuttingDown()) {
        lock();
        winInfo = getWindowInfo(id);
        int timeout = winInfo ? getTime() - winInfo->getPingTimeStamp() >= KILL_TIMEOUT : 0;
        unlock();
        if(!winInfo) {
            LOG(LOG_LEVEL_DEBUG, "Window %d no longer exists", id);
            break;
        }
        else if(timeout) {
            LOG(LOG_LEVEL_INFO, "Window %d is not responsive; force killing", id);
            killClientOfWindow(id);
            break;
        }
        if(hasPingMask) {
            xcb_ewmh_send_wm_ping(ewmh, id, 0);
            flush();
        }
        msleep(KILL_TIMEOUT);
    }
    LOG(LOG_LEVEL_DEBUG, "Finished waiting for window %d", id);
}

int killClientOfWindow(WindowID win) {
    assert(win);
    LOG(LOG_LEVEL_DEBUG, "Killing window %d", win);
    return catchError(xcb_kill_client_checked(dis, win));
}
void killClientOfWindowInfo(WindowInfo* winInfo) {
    if(winInfo->hasMask(WM_DELETE_WINDOW_MASK)) {
        unsigned int data[5] = {WM_DELETE_WINDOW, getTime()};
        xcb_ewmh_send_client_message(dis, root, winInfo->getID(), ewmh->WM_PROTOCOLS, 5, data);
        INFO("Sending request to delete window");
        WindowID id = winInfo->getID();
        spawnThread([ = ] {waitForWindowToDie(id);}, "wait for window to die");
    }
    else {
        killClientOfWindow(winInfo->getID());
    }
}

void updateWindowWorkspaceState(WindowInfo* winInfo) {
    Workspace* w = winInfo->getWorkspace();
    if(!w)
        return;
    DEBUG("updating window workspace state: " << w->isVisible() << "; " << *winInfo);
    if(winInfo->isNotInInvisibleWorkspace() && winInfo->isMappable()) {
        mapWindow(winInfo->getID());
        winInfo->addMask(MAPPABLE_MASK | MAPPED_MASK);
        updateFocusForAllMasters(NULL);
    }
    else {
        if(winInfo->hasMask(STICKY_MASK)) {
            Workspace* w = getActiveWorkspace();
            if(!w->isVisible())
                w = getActiveMaster()->getWorkspace()->getNextWorkspace(1, VISIBLE);
            if(w)
                winInfo->moveToWorkspace(w->getID());
            return;
        }
        updateFocusForAllMasters(winInfo);
        unmapWindow(winInfo->getID());
        winInfo->removeMask(FULLY_VISIBLE_MASK | MAPPED_MASK);
    }
}
void syncMappedState(Workspace* workspace) {
    DEBUG("Syncing map state: " << *workspace);
    for(WindowInfo* winInfo : workspace->getWindowStack())
        if(winInfo->hasMask(STICKY_MASK))
            updateWindowWorkspaceState(winInfo);
    for(WindowInfo* winInfo : workspace->getWindowStack())
        if(!winInfo->hasMask(STICKY_MASK))
            updateWindowWorkspaceState(winInfo);
}

void switchToWorkspace(int workspaceIndex) {
    if(!getWorkspace(workspaceIndex)->isVisible()) {
        int currentIndex = getActiveWorkspaceIndex();
        /*
         * Each master can have independent active workspaces. While an active workspace is generally visible when it is set,
         * it can become invisible due to another master switching workspaces. So when the master in the invisible workspaces
         * wants its workspace to become visible it must first find a visible workspace to swap with
         */
        if(!getActiveWorkspace()->isVisible()) {
            Workspace* visibleWorkspace = getActiveMaster()->getWorkspace()->getNextWorkspace(1, VISIBLE);
            if(visibleWorkspace)
                currentIndex = visibleWorkspace->getID();
        }
        LOG(LOG_LEVEL_DEBUG, "Swapping visible workspace %d with %d", currentIndex, workspaceIndex);
        swapMonitors(workspaceIndex, currentIndex);
    }
    getActiveMaster()->setWorkspaceIndex(workspaceIndex);
}

WindowID activateWindow(WindowInfo* winInfo) {
    if(winInfo && winInfo->isActivatable()) {
        raiseWindow(winInfo);
        LOG(LOG_LEVEL_DEBUG, "activating window %d in workspace %d", winInfo->getID(), winInfo->getWorkspaceIndex());
        if(winInfo->getWorkspaceIndex() != NO_WORKSPACE) {
            switchToWorkspace(winInfo->getWorkspaceIndex());
        }
        if(!winInfo->hasMask(MAPPED_MASK)) {
            assert(winInfo->hasMask(MAPPABLE_MASK));
            mapWindow(winInfo->getID());
            winInfo->addMask(MAPPABLE_MASK | MAPPED_MASK);
        }
        return focusWindow(winInfo) ? winInfo->getID() : 0;
    }
    return 0;
}


void configureWindow(WindowID win, uint32_t mask, uint32_t values[7]) {
    assert(mask);
    assert(mask < 128);
    if(mask) {
        LOG(LOG_LEVEL_INFO, "Config %d: mask %d (%d bits)", win, mask, __builtin_popcount(mask));
        LOG_RUN(LOG_LEVEL_INFO, PRINT_ARR("Config values", values, std::min(__builtin_popcount(mask), 7)));
    }
    XCALL(xcb_configure_window, dis, win, mask, values);
}
void setWindowPosition(WindowID win, const RectWithBorder geo, bool onlyPosition) {
    uint32_t values[5];
    geo.copyTo(values);
    uint32_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y;
    if(!onlyPosition)
        mask |= XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT ;
    configureWindow(win, mask, values);
}

void swapWindows(WindowInfo* winInfo1, WindowInfo* winInfo2) {
    LOG(LOG_LEVEL_TRACE, "swapping windows %d %d", winInfo1->getID(), winInfo2->getID());
    winInfo1->swapWorkspaceWith(winInfo2);
    RectWithBorder geo = getRealGeometry(winInfo2->getID());
    setWindowPosition(winInfo2->getID(), getRealGeometry(winInfo1->getID()));
    setWindowPosition(winInfo1->getID(), geo);
}
static inline int filterConfigValues(uint32_t* filteredArr, const WindowInfo* winInfo, const short values[5],
    WindowID sibling,
    int stackMode, int configMask) {
    int i = 0;
    int n = 0;
    if(configMask & XCB_CONFIG_WINDOW_X && ++n && winInfo->isExternallyMoveable())
        filteredArr[i++] = values[n - 1];
    else configMask &= ~XCB_CONFIG_WINDOW_X;
    if(configMask & XCB_CONFIG_WINDOW_Y && ++n && winInfo->isExternallyMoveable())
        filteredArr[i++] = values[n - 1];
    else configMask &= ~XCB_CONFIG_WINDOW_Y;
    if(configMask & XCB_CONFIG_WINDOW_WIDTH && ++n && winInfo->isExternallyResizable())
        filteredArr[i++] = values[n - 1];
    else configMask &= ~XCB_CONFIG_WINDOW_WIDTH;
    if(configMask & XCB_CONFIG_WINDOW_HEIGHT && ++n && winInfo->isExternallyResizable())
        filteredArr[i++] = values[n - 1];
    else configMask &= ~XCB_CONFIG_WINDOW_HEIGHT;
    if(configMask & XCB_CONFIG_WINDOW_BORDER_WIDTH && ++n && winInfo->isExternallyBorderConfigurable())
        filteredArr[i++] = values[n - 1];
    else configMask &= ~XCB_CONFIG_WINDOW_BORDER_WIDTH;
    if(configMask & XCB_CONFIG_WINDOW_SIBLING && ++n && winInfo->isExternallyRaisable())
        filteredArr[i++] = sibling;
    else configMask &= ~XCB_CONFIG_WINDOW_SIBLING;
    if(configMask & XCB_CONFIG_WINDOW_STACK_MODE && ++n && winInfo->isExternallyRaisable())
        filteredArr[i++] = stackMode;
    else configMask &= ~XCB_CONFIG_WINDOW_STACK_MODE;
    return configMask;
}
int processConfigureRequest(WindowID win, const short values[5], WindowID sibling, int stackMode, int configMask) {
    assert(configMask);
    LOG(LOG_LEVEL_DEBUG, "processing configure request window %d", win);
    uint32_t actualValues[7];
    WindowInfo* winInfo = getWindowInfo(win);
    if(!winInfo) {
        WindowInfo fake = WindowInfo(0, 0);
        fake.addMask(EXTERNAL_CONFIGURABLE_MASK);
        filterConfigValues(actualValues, &fake, values, sibling, stackMode, configMask);
        configureWindow(win, configMask, actualValues);
        return configMask;
    }
    int mask = filterConfigValues(actualValues, winInfo, values, sibling, stackMode, configMask);
    LOG(LOG_LEVEL_DEBUG, "Mask filtered from %d to %d", configMask, mask);
    if(mask) {
        configureWindow(win, mask, actualValues);
        /*
        LOG(LOG_LEVEL_TRACE, "re-configure window %d configMask: %d", win, mask);
        if(mask & XCB_CONFIG_WINDOW_STACK_MODE) {
            Workspace* workspace = winInfo->getWorkspace();
            if(workspace)
                if(sibling == XCB_NONE) {
                    int i = workspace->getWindowStack().indexOf(winInfo);
                    if(stackMode == XCB_STACK_MODE_ABOVE)
                        workspace->getWindowStack().shiftToHead(i);
                    else
                        workspace->getWindowStack().shiftToEnd(i);
                }
                else LOG(LOG_LEVEL_WARN, "not updating workspace window stack to reflect sibling change: win %d, sibling %d", win,
                             sibling);
        }
        */
    }
    else {
        LOG(LOG_LEVEL_INFO, "configure request denied for window %d; configMasks %d (%d)", win, mask, configMask);
        INFO(*winInfo);
    }
    return mask;
}
void removeBorder(WindowID win) {
    uint32_t i = 0;
    configureWindow(win, XCB_CONFIG_WINDOW_BORDER_WIDTH, &i);
}


void raiseWindow(WindowID win, WindowID sibling, bool above) {
    uint32_t values[] = {sibling, above ? XCB_STACK_MODE_ABOVE : XCB_STACK_MODE_BELOW};
    int mask = XCB_CONFIG_WINDOW_STACK_MODE;
    if(sibling)
        mask |= XCB_CONFIG_WINDOW_SIBLING;
    configureWindow(win, mask, &values[!sibling]);
}
void raiseWindow(WindowInfo* winInfo, WindowID sibling, bool above) {
    if(!sibling)
        if(winInfo->hasMask(ABOVE_MASK) && !above || winInfo->hasMask(BELOW_MASK) && above) {
            sibling = getWindowDivider(above);
            above = !above;
        }
        else if(above && !winInfo->hasMask(ABOVE_MASK) || !above && !winInfo->hasMask(BELOW_MASK))
            sibling = getWindowDivider(above = !above);
    raiseWindow(winInfo->getID(), sibling, above);
}
RectWithBorder getRealGeometry(WindowID id) {
    RectWithBorder rect;
    xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, id), NULL);
    if(reply) {
        rect = &reply->x;
        free(reply);
    }
    return rect;
}
