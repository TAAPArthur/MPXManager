/**
 * @file windows.c
 * @copybrief windows.h
 *
 */

#include <assert.h>

#include "ext.h"
#include "masters.h"
#include "windows.h"
#include "workspaces.h"
#include "boundfunction.h"
#include "user-events.h"


///list of all windows
static ArrayList<WindowInfo*> windows;
ArrayList<WindowInfo*>& getAllWindows(void) {
    return windows;
}

std::ostream& operator<<(std::ostream& stream, const WindowInfo& winInfo) {
    stream << "{ WindowInfo: ID " << winInfo.getID() << (winInfo.isTileable() ? "*" : "") <<
           (!winInfo.isMappable() ? "?" : "");
    stream << " Parent: " << winInfo.getParent();
    if(winInfo.getGroup())
        stream << " Group: " << winInfo.getGroup();
    if(winInfo.getTransientFor())
        stream << " Transient For: " << winInfo.getTransientFor();
    stream << " Title '" << winInfo.getTitle() << "'" ;
    stream << " Class '" << winInfo.getClassName()  << "' '" << winInfo.getInstanceName() << "'";
    stream << " Type '" << winInfo.getTypeName() << "'";
    stream << " Mask " << winInfo.getMask() << ": " << maskToString(winInfo.getMask());
    if(winInfo.getWorkspace())
        stream << " Workspace:" << winInfo.getWorkspaceIndex() ;
    if(winInfo.isOverrideRedirectWindow())
        stream << "OverrideRedirect: " << winInfo.isOverrideRedirectWindow() ;
    if(winInfo.getTilingOverrideMask())
        stream << " Tiling override " << winInfo.getTilingOverrideMask() << winInfo.getTilingOverride();
    if(winInfo.isDock())
        stream << " Dock: " << winInfo.isDock() ;
    stream << " Geometry " << winInfo.getGeometry();
    return stream << "}";
}

bool WindowInfo::isNotInInvisibleWorkspace()const {
    if(getWorkspaceIndex() == NO_WORKSPACE)
        return 1;
    return getWorkspace()->isVisible();
}
WindowMask WindowInfo::hasPartOfMask(WindowMask mask)const {
    Workspace* w = getWorkspace();
    WindowMask winMask = getMask();
    if(w && !((winMask | mask) & IGNORE_WORKSPACE_MASKS_MASK))
        winMask |= w->getMask();
    return (winMask & mask);
}
bool WindowInfo::hasMask(WindowMask mask)const {
    return hasPartOfMask(mask) == mask;
}

void WindowInfo::toggleMask(WindowMask mask) {
    if((getMask()&mask) == mask)
        removeMask(mask);
    else
        addMask(mask);
}

bool WindowInfo::removeFromWorkspace() {
    Workspace* w = getWorkspace();
    return w ? w->getWindowStack().removeElement(this) : 0;
}

void WindowInfo::moveToWorkspace(WorkspaceID destIndex) {
    assert(destIndex < getNumberOfWorkspaces() || destIndex == NO_WORKSPACE);
    if(destIndex == NO_WORKSPACE) {
        addMask(STICKY_MASK);
        destIndex = getActiveMaster()->getWorkspaceIndex();
    }
    removeFromWorkspace();
    ::getWorkspace(destIndex)->getWindowStack().add(this);
    applyEventRules(WindowWorkspaceMove);
}


WorkspaceID WindowInfo::getWorkspaceIndex()const {
    Workspace* w = getWorkspace();
    return w ? w->getID() : NO_WORKSPACE;
}
Workspace* WindowInfo::getWorkspace(void)const {
    for(Workspace* w : getAllWorkspaces())
        if(w->getWindowStack().find(getID()))
            return w;
    return NULL;
}

WindowInfo::~WindowInfo() {
    for(Master* master : getAllMasters())
        master->getWindowStack().removeElement(this);
    removeFromWorkspace();
    removeID(this);
}
void WindowInfo::setDockProperties(int* properties, int numberofProperties) {
    memset(getDockProperties(1), 0, 12 * sizeof(int));
    memcpy(getDockProperties(1), properties, numberofProperties * sizeof(int));
}
int* WindowInfo::getDockProperties(bool createNew) {
    static Index<int[12]> key;
    return *get(key, this, createNew);
}

bool WindowInfo::matches(std::string str) {
    return getTitle() == str || getClassName() == str || getInstanceName() == str || getTypeName() == str;
}
