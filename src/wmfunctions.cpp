/**
 * @file wmfunctions.c
 * @brief wmfunctions.h
 *
 */

#include <assert.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include <X11/extensions/XInput2.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xinput.h>


#include "system.h"
#include "bindings.h"
#include "globals.h"
#include "logger.h"
#include "masters.h"
#include "windows.h"
#include "window-properties.h"
#include "wmfunctions.h"
#include "monitors.h"
#include "workspaces.h"
#include "xsession.h"
#include "user-events.h"
#include "time.h"



bool postRegisterWindow(WindowInfo* winInfo, bool newlyCreated) {
    if(newlyCreated)
        winInfo->setCreationTime(getTime());
    return applyEventRules(PostRegisterWindow, winInfo);
}

bool registerWindow(WindowID win, WindowID parent, xcb_get_window_attributes_reply_t* attr) {
    assert(!getAllWindows().find(win) && "Window registered exists");
    LOG(LOG_LEVEL_TRACE, "processing %d (%x)\n", win, win);
    WindowInfo* winInfo = new WindowInfo(win, parent);
    bool freeAttr = !attr;
    bool newlyCreated = !attr;
    if(!attr)
        attr = xcb_get_window_attributes_reply(dis, xcb_get_window_attributes(dis, win), NULL);
    if(attr) {
        if(attr->override_redirect)
            winInfo->markAsOverrideRedirect();
        if(attr->_class == XCB_WINDOW_CLASS_INPUT_ONLY)
            winInfo->addMask(INPUT_ONLY_MASK);
        if(attr->map_state)
            winInfo->addMask(MAPPABLE_MASK | MAPPED_MASK);
        if(freeAttr)
            free(attr);
    }
    else {
        LOG(LOG_LEVEL_DEBUG, "Could not load attributes %d \n", win);
        delete winInfo;
        return 0;
    }
    if(!applyEventRules(PreRegisterWindow, winInfo)) {
        delete winInfo;
        return 0;
    }
    LOG(LOG_LEVEL_DEBUG, "Registering %d (%x)\n", winInfo->getID(), winInfo->getID());
    getAllWindows().add(winInfo);
    return postRegisterWindow(winInfo, newlyCreated);
}
void scan(xcb_window_t baseWindow) {
    LOG(LOG_LEVEL_TRACE, "Scanning children of %d\n", baseWindow);
    xcb_query_tree_reply_t* reply;
    reply = xcb_query_tree_reply(dis, xcb_query_tree(dis, baseWindow), 0);
    assert(reply && "could not query tree");
    if(reply) {
        xcb_get_window_attributes_reply_t* attr;
        int numberOfChildren = xcb_query_tree_children_length(reply);
        LOG(LOG_LEVEL_TRACE, "detected %d kids\n", numberOfChildren);
        xcb_window_t* children = xcb_query_tree_children(reply);
        xcb_get_window_attributes_cookie_t cookies[numberOfChildren];
        for(int i = 0; i < numberOfChildren; i++)
            cookies[i] = xcb_get_window_attributes(dis, children[i]);
        // iterate in top to bottom order
        for(int i = numberOfChildren - 1; i >= 0; i--) {
            LOG(LOG_LEVEL_TRACE, "processing child %d\n", children[i]);
            attr = xcb_get_window_attributes_reply(dis, cookies[i], NULL);
            if(registerWindow(children[i], baseWindow, attr)) {
                WindowInfo* winInfo = getWindowInfo(children[i]);
                assert(winInfo);
                xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, children[i]), NULL);
                if(reply) {
                    getWindowInfo(children[i])->setGeometry(&reply->x);
                    free(reply);
                }
                if(winInfo->hasMask(MAPPED_MASK)) {
                    LOG(LOG_LEVEL_DEBUG, "Window is mapped %d\n", winInfo->getID());
                    if(!applyEventRules(ClientMapAllow, winInfo))
                        winInfo = NULL;
                }
                if(winInfo && !IGNORE_SUBWINDOWS)
                    scan(children[i]);
            }
            if(attr)
                free(attr);
        }
        free(reply);
    }
}

static void focusNextVisibleWindow(Master* master, WindowInfo* defaultWinInfo) {
    auto filter = [](WindowInfo * winInfo) {return winInfo->isNotInInvisibleWorkspace() && winInfo->isActivatable() && focusWindow(winInfo);};
    if(!master->getMostRecentlyFocusedWindow(filter))
        focusWindow(defaultWinInfo);
}

bool unregisterWindow(WindowInfo* winInfo) {
    if(!winInfo)
        return 0;
    WindowID winToRemove = winInfo->getID();
    LOG(LOG_LEVEL_DEBUG, "window %d has been removed\n", winToRemove);
    ArrayList<Master*> mastersFocusedOnWindow ;
    for(Master* master : getAllMasters())
        if(master->getFocusedWindow() == winInfo)
            mastersFocusedOnWindow.add(master);
    applyEventRules(UnregisteringWindow, winInfo);
    bool result = removeWindow(winToRemove);
    ArrayList<WindowInfo*>& windowStack = getActiveMaster()->getWorkspace()->getWindowStack();
    WindowInfo* defaultWinInfo = windowStack.empty() ? NULL : windowStack[0];
    if(defaultWinInfo)
        for(Master* master : mastersFocusedOnWindow)
            focusNextVisibleWindow(master, defaultWinInfo);
    return result;
}


static void* waitForWindowToDie(void* p) {
    int id = *reinterpret_cast<int*>(p);
    lock();
    WindowInfo* winInfo = getWindowInfo(id);
    int hasPingMask = 0;
    if(winInfo) {
        winInfo->setPingTimeStamp(getTime());
        hasPingMask = winInfo->hasMask(WM_PING_MASK);
    }
    unlock();
    while(!isShuttingDown()) {
        if(hasPingMask) {
            xcb_ewmh_send_wm_ping(ewmh, id, 0);
            flush();
        }
        lock();
        winInfo = getWindowInfo(id);
        int timeout = winInfo ? getTime() - winInfo->getPingTimeStamp() >= KILL_TIMEOUT : 0;
        unlock();
        if(!winInfo) {
            LOG(LOG_LEVEL_DEBUG, "Window %d no longer exists\n", id);
            break;
        }
        else if(timeout) {
            LOG(LOG_LEVEL_INFO, "Window %d is not responsive; force killing\n", id);
            killClientOfWindow(id);
            break;
        }
        msleep(KILL_TIMEOUT);
    }
    LOG(LOG_LEVEL_DEBUG, "Finished waiting for window %d\n", id);
    delete reinterpret_cast<int*>(p);
    return NULL;
}

int destroyWindow(WindowID win) {
    assert(win);
    LOG(LOG_LEVEL_DEBUG, "Destroying window %d\n", win);
    return catchError(xcb_destroy_window_checked(dis, win));
}
int killClientOfWindow(WindowID win) {
    assert(win);
    LOG(LOG_LEVEL_DEBUG, "Killing window %d\n", win);
    return catchError(xcb_kill_client_checked(dis, win));
}
void killClientOfWindowInfo(WindowInfo* winInfo) {
    if(winInfo->hasMask(WM_DELETE_WINDOW_MASK)) {
        unsigned int data[5] = {WM_DELETE_WINDOW, getTime()};
        xcb_ewmh_send_client_message(dis, root, winInfo->getID(), ewmh->WM_PROTOCOLS, 5, data);
        LOG(LOG_LEVEL_INFO, "Sending request to delete window\n");
        runInNewThread(waitForWindowToDie, new int(winInfo->getID()), "wait for window to die");
    }
    else {
        killClientOfWindow(winInfo->getID());
    }
}


int attemptToMapWindow(WindowID id) {
    WindowInfo* winInfo = getWindowInfo(id);
    if(!winInfo || winInfo->isNotInInvisibleWorkspace() && winInfo->isMappable())
        return catchError(xcb_map_window_checked(dis, id));
    return -1;
}

// TODO xcb_ewmh_set_wm_desktop(ewmh, winInfo->getID(), destIndex);
// TODO trigger this on window move
void updateWindowWorkspaceState(WindowInfo* winInfo, bool updateFocus) {
    Workspace* w = winInfo->getWorkspace();
    if(!w)
        return;
    if(w->isVisible()) {
        attemptToMapWindow(winInfo->getID());
    }
    else {
        if(winInfo->hasMask(STICKY_MASK)) {
            Workspace* w = getActiveWorkspace();
            if(!w->isVisible())
                w = getActiveMaster()->getWorkspace()->getNextWorkspace(1, VISIBLE);
            assert(w);
            LOG(LOG_LEVEL_DEBUG, "Moving sticky window %d to workspace %d\n", winInfo->getID(), w->getID());
            if(w)
                winInfo->moveToWorkspace(w->getID());
            return;
        }
        catchError(xcb_unmap_window_checked(dis, winInfo->getID()));
        if(updateFocus) {
            for(Master* master : getAllMasters()) {
                if(master->getFocusedWindow() == winInfo)
                    focusNextVisibleWindow(master, NULL);
            }
        }
    }
}
void syncMappedState(WorkspaceID index) {
    for(WindowInfo* winInfo : getWorkspace(index)->getWindowStack())
        updateWindowWorkspaceState(winInfo, 1);
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
            assert(visibleWorkspace);
            if(visibleWorkspace)
                currentIndex = visibleWorkspace->getID();
        }
        LOG(LOG_LEVEL_DEBUG, "Swapping visible workspace %d with %d\n", currentIndex, workspaceIndex);
        swapMonitors(workspaceIndex, currentIndex);
    }
    getActiveMaster()->setWorkspaceIndex(workspaceIndex);
}

/* TODO delete
void moveWindowToWorkspace(WindowInfo* winInfo, int destIndex){
    assert(destIndex == -1 || destIndex >= 0 && destIndex < getNumberOfWorkspaces());
    assert(winInfo);
    if(destIndex == -1){
        winInfo->addMask(STICKY_MASK);
        destIndex = getActiveMaster()->getWorkspaceIndex();
    }
    winInfo->removeFromWorkspace();
    winInfo->addToWorkspace(destIndex);
    updateWindowWorkspaceState(winInfo, destIndex, NO_WORKSPACE, 1);
    xcb_ewmh_set_wm_desktop(ewmh, winInfo->getID(), destIndex);
    LOG(LOG_LEVEL_TRACE, "window %d added to workspace %d\n", winInfo->getID(), destIndex);
}
*/


void activateWorkspace(int workspaceIndex) {
    switchToWorkspace(workspaceIndex);
    Workspace* w = getWorkspace(workspaceIndex);
    auto filter = [](WindowInfo * winInfo) {return winInfo->getWorkspaceIndex() == getActiveWorkspaceIndex();};
    WindowInfo* winInfo = getActiveMaster()->getMostRecentlyFocusedWindow(filter);
    if(!winInfo)
        winInfo = w->getWindowStack().empty() ? NULL : w->getWindowStack()[0];
    if(winInfo)
        activateWindow(winInfo);
    else focusWindow(root);
}


WindowID activateWindow(WindowInfo* winInfo) {
    if(winInfo && raiseWindowInfo(winInfo) && winInfo->isActivatable()) {
        LOG(LOG_LEVEL_DEBUG, "activating window %d in workspace %d\n", winInfo->getID(), winInfo->getWorkspaceIndex());
        if(winInfo->getWorkspaceIndex() != NO_WORKSPACE) {
            switchToWorkspace(winInfo->getWorkspaceIndex());
        }
        if(!winInfo->hasMask(MAPPED_MASK))
            attemptToMapWindow(winInfo->getID());
        return focusWindow(winInfo) ? winInfo->getID() : 0;
    }
    return 0;
}

bool doesWorkspaceHaveWindowWithMask(WorkspaceID index, WindowMask mask) {
    for(WindowInfo* winInfo : getWorkspace(index)->getWindowStack())
        if(winInfo->hasMask(mask))return 1;
    return 0;
}



void configureWindow(WindowID win, uint32_t mask, int values[7]) {
#ifndef DNDEBUG
    LOG(LOG_LEVEL_DEBUG, "Config %d: mask %d %d\n", win, mask, __builtin_popcount(mask));
    if(mask)
        PRINT_ARR("values", values, std::min(__builtin_popcount(mask), 7), "\n");
    catchError(xcb_configure_window_checked(dis, win, mask, values));
#else
    xcb_configure_window(dis, win, mask, values);
#endif
}
void setWindowPosition(WindowID win, const RectWithBorder geo) {
    int values[5];
    geo.copyTo(values);
    uint32_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
                    XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT ;
    configureWindow(win, mask, values);
}

void swapWindows(WindowInfo* winInfo1, WindowInfo* winInfo2) {
    LOG(LOG_LEVEL_TRACE, "swapping windows %d %d\n", winInfo1->getID(), winInfo2->getID());
    Workspace* w1 = winInfo1->getWorkspace();
    Workspace* w2 = winInfo2->getWorkspace();
    int index1 = w1 ? w1->getWindowStack().indexOf(winInfo1) : -1;
    int index2 = w2 ? w2->getWindowStack().indexOf(winInfo2) : -1;
    if(index1 != -1)
        w1->getWindowStack()[index1] = winInfo2;
    else if(w2)
        w2->getWindowStack().removeElement(winInfo2);
    if(index2 != -1)
        w2->getWindowStack()[index2] = winInfo1;
    else if(w1)
        w1->getWindowStack().removeElement(winInfo1);
    if((w1 && w1->isVisible()) != (w2 && w2->isVisible())) {
        if(w2)
            updateWindowWorkspaceState(winInfo1, 0);
        if(w1)
            updateWindowWorkspaceState(winInfo2, 0);
    }
    setWindowPosition(winInfo2->getID(), winInfo1->getGeometry());
    setWindowPosition(winInfo1->getID(), winInfo2->getGeometry());
}
static inline int filterConfigValues(int* filteredArr, const WindowInfo* winInfo, const short values[5],
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
void processConfigureRequest(WindowID win, const short values[5], WindowID sibling, int stackMode, int configMask) {
    LOG(LOG_LEVEL_TRACE, "processing configure request window %d\n", win);
    int actualValues[7];
    WindowInfo* winInfo = getWindowInfo(win);
    if(!winInfo) {
        for(int i = 0; i < 5; i++)
            actualValues[i] = values[i];
        actualValues[5] = sibling;
        actualValues[6] = stackMode;
        configureWindow(win, configMask, actualValues);
    }
    int mask = filterConfigValues(actualValues, winInfo, values, sibling, stackMode, configMask);
    if(mask) {
        configureWindow(win, mask, actualValues);
        LOG(LOG_LEVEL_TRACE, "re-configure window %d configMask: %d\n", win, mask);
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
                else LOG(LOG_LEVEL_WARN, "not updating workspace window stack to reflect sibling change: win %d, sibling %d\n", win,
                             sibling);
        }
    }
    else
        LOG(LOG_LEVEL_DEBUG, "configure request denied for window %d; configMasks %d (%d)\n", win, mask, configMask);
}
void removeBorder(WindowInfo* winInfo) {
    short i = 0;
    xcb_configure_window(dis, winInfo->getID(), XCB_CONFIG_WINDOW_BORDER_WIDTH, &i);
}
bool raiseWindowInfo(WindowInfo* winInfo, bool above) {
    assert(winInfo);
    int values[] = {above ? XCB_STACK_MODE_ABOVE : XCB_STACK_MODE_BELOW};
    int mask = XCB_CONFIG_WINDOW_STACK_MODE;
    return !catchError(xcb_configure_window(dis, winInfo->getID(), mask, values));
}
bool lowerWindowInfo(WindowInfo* winInfo) {
    return raiseWindowInfo(winInfo, 0);
}
RectWithBorder getRealGeometry(WindowID id) {
    RectWithBorder rect;
    xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, id), NULL);
    if(reply) {
        rect = &reply->x;
        free(reply);
    }
    else
        return {0, 0, 0, 0};
    return rect;
}
