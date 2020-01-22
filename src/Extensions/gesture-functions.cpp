#include "../logger.h"
#include "../test-functions.h"
#include "gestures.h"

void moveAndClick(int button) {
    auto* event = getLastGestureEvent();
    movePointer(event->endPoint.x, event->endPoint.y);
    double ratio = event->flags.avgSqDisplacement / (event->flags.duration / 100.0 + .01);
    INFO("RATIO " << ratio << " " << button);
    int i;
    for(i = 0; i <= ratio * event->flags.count; i++)
        clickButton(button);
    INFO("RATIO " << ratio << " " << i);
}
