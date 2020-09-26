#include "../boundfunction.h"
#include "../util/logger.h"
#include "../monitors.h"
#include "../mywm-structs.h"
#include "../user-events.h"
#include "../windows.h"
#include "../xutil/window-properties.h"
#include "compatibility-rules.h"

void autoDetectDockPosition(WindowInfo* winInfo) {
    if(winInfo->dock && !getDockProperties(winInfo)) {
        const Rect rect = getRealGeometry(winInfo->id);
        int properties[12] = {0};
        FOR_EACH(Monitor*, monitor, getAllMonitors()) {
            const Rect bounds = monitor->base;
            if(contains(bounds, rect) && !containsProper(bounds, rect)) {
                bool dockHor = rect.y == bounds.y || rect.y + rect.height == bounds.y + bounds.height;
                int type = !dockHor ?
                    rect.x == 0 ? DOCK_LEFT : DOCK_RIGHT :
                    rect.y == 0 ? DOCK_TOP : DOCK_BOTTOM;
                int thickness = (&rect.x)[dockHor + 2];
                properties[type] = thickness;
                if((&rect.x)[!dockHor] != (&bounds.x)[!dockHor] && (&rect.x)[!dockHor + 2] != (&bounds.x)[!dockHor + 2]) {
                    properties[4 + 2 * type] = (&rect.x)[!dockHor];
                    properties[4 + 2 * type + 1] = (&rect.x)[!dockHor] + (&rect.x)[!dockHor + 2];
                }
                setDockProperties(winInfo, properties, LEN(properties));
                applyEventRules(SCREEN_CHANGE, NULL);
                return;
            }
        }
    }
}
void addAutoDetectDockPosition() {
    addEvent(WINDOW_MOVE, DEFAULT_EVENT(autoDetectDockPosition, LOWER_PRIORITY));
}
