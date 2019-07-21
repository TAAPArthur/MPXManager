#ifndef MPX_WINDOW_PROPERTIES
#define MPX_WINDOW_PROPERTIES
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include "xsession.h"
#include "mywm-structs.h"
#include "window-masks.h"
#include "user-events.h"
#include "device-grab.h"
#include "ewmh.h"
#include <string>

void setWindowTransientFor(WindowID win, WindowID transientTo);
void setWindowClass(WindowID win, std::string className, std::string instanceName);
void setWindowTitle(WindowID win, std::string title);
void setWindowType(WindowID win, xcb_atom_t* atoms, int num);
static inline void setWindowType(WindowID win, xcb_atom_t atom) {setWindowType(win, &atom, 1);}

/**
 * Loads class and instance name for the given window
 * @param info
 */
bool loadClassInfo(WindowID win, std::string* className, std::string* instanceName);
/**
 * Returns the name of the window in a str that needs to be freeded
 *
 * @param win
 *
 * @return
 */
std::string getWindowTitle(WindowID win);
/**
 * Loads title for a given window
 * @param winInfo
 */
std::string loadTitleInfo();

/**
 * Loads type name and atom value for a given window
 * @param winInfo
 * @return 1 if the caller should continue as normal
 */
bool loadWindowType(WindowID win, bool transient, uint32_t* type, std::string* typeName);
/**
 * Load various window properties
 * This should be called when a window is requested to be mapped
 * @param winInfo
 */
void loadWindowProperties(WindowInfo*);


/**
 * Sets the border color for the given window
 * @param win   the window in question
 * @param color the new window border color
 */
void setBorderColor(WindowID win, unsigned int color);

/**
 * Sets the border color of the window to match the active master
 * @param win
 */
void setBorder(WindowID win);
/**
 * Sets the border color of the window to match the master that last focused this window
 * (not counting the current active master) or to the default color
 * @param win
 */
void resetBorder(WindowID win);

/**
 * Adds the FLOATING_MASK to a given window
 */
void floatWindow(WindowInfo* win);
/**
 * Removes any all all masks that would cause the window to not be tiled
 */
void sinkWindow(WindowInfo* win);

/**
 * Assigns names to the first n workspaces
 * @param names
 * @param numberOfNames
 */
void setWorkspaceNames(char* names[], int numberOfNames);

/**
 * Focuses the given window
 * @param win   the window to focus
 * @return 1 iff no error was detected
 */
int focusWindow(WindowID win, Master* master = getActiveMaster());
/**
 * Focus the given window
 * This method is different from focusWindow in that it allows different protocols for
 * focusing the window based on window masks
 */
int focusWindow(WindowInfo* winInfo, Master* master = getActiveMaster());

xcb_size_hints_t* getWindowSizeHints(WindowInfo* winInfo);
RectWithBorder getRealGeometry(WindowID id) ;
static inline RectWithBorder getRealGeometry(WindowInfo* winInfo) {
    return getRealGeometry(winInfo->getID());
}
#endif
