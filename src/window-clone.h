#include <assert.h>

#include <xcb/xcb.h>

#include "ewmh.h"

#include "globals.h"
#include "mywm-util.h"
#include "wmfunctions.h"

/**
 * Clones a WindowInfo object and sets its id to id
 *
 * @param id    the id of the WindowInfo object
 * @param winInfo    the window to clone
 * @return  pointer to the newly created object
 */
WindowInfo* cloneWindowInfo(WindowID id, WindowInfo* winInfo);
WindowInfo* cloneWindow(WindowInfo* winInfo);

void updateClone(WindowInfo* winInfo, WindowInfo* dest);
void updateAllClones(WindowInfo* winInfo);
int getCloneOrigin(WindowInfo* winInfo);

void* autoUpdateClones();
void addCloneRules(void);
