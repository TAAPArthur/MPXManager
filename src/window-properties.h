/**
 * @file
 * Methods to load client supplied window info into our WindowInfo
 */
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

/**
 * Sets the win to be a transient for transientTo
 *
 * @param win
 * @param transientTo
 */
void setWindowTransientFor(WindowID win, WindowID transientTo);
/**
 * Sets the ICCC class and instance (resource) for the window
 *
 * @param win
 * @param className
 * @param instanceName
 */
void setWindowClass(WindowID win, std::string className, std::string instanceName);
/**
 * Sets the window title
 *
 * @param win
 * @param title
 */
void setWindowTitle(WindowID win, std::string title);
/**
 * Sets the window type in order of preference
 *
 * @param win
 * @param atoms
 * @param num
 */
void setWindowType(WindowID win, xcb_atom_t* atoms, int num);
/**
 * Sets a single window type
 *
 * @param win
 * @param atom
 */
static inline void setWindowType(WindowID win, xcb_atom_t atom) {setWindowType(win, &atom, 1);}

/**
 * Loads class and instance name for the given window
 * @param win
 * @param className
 * @param instanceName
 *
 * @return 1 iff the class info was successfully loaded
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
 */
std::string loadTitleInfo();

/**
 * Loads type name and atom value for a given window
 * If the window type is not set, the type is set to _NET_WM_WINDOW_TYPE_DIALOG or _NET_WM_WINDOW_TYPE_NORMAL depending if the window is transient or not
 *
 * @param winInfo
 * @return 1 the window type was set for the window
 */
bool loadWindowType(WindowInfo* winInfo) ;
/**
 * Load various window properties
 * This should be called when a window is requested to be mapped
 * @param winInfo
 */
void loadWindowProperties(WindowInfo* winInfo);
/**
 * Loads normal hints
 * Loads grouptID, input and window state for a given window
 * @param winInfo
 * @see loadWindowProperties
 */
void loadWindowHints(WindowInfo* winInfo);
/**
 * Loads the window title
 *
 * @param winInfo
 * @see loadWindowProperties
 */
void loadWindowTitle(WindowInfo* winInfo) ;


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
 * @param master the master who gets the focus
 * @return 1 iff no error was detected
 */
int focusWindow(WindowID win, Master* master = getActiveMaster());
/**
 * Focus the given window
 * This method is different from focusWindow in that it allows different protocols for
 * focusing the window based on window masks
 */
int focusWindow(WindowInfo* winInfo, Master* master = getActiveMaster());

/**
 * Returns the cached window size hints
 *
 * These are cached on calls to load properties
 *
 * @param winInfo
 *
 * @return
 */
xcb_size_hints_t* getWindowSizeHints(WindowInfo* winInfo);

/**
 * @param id
 * @return the live window geometry
 */
RectWithBorder getRealGeometry(WindowID id) ;
/**
 * @param winInfo
 * @return the live window geometry
 */
static inline RectWithBorder getRealGeometry(WindowInfo* winInfo) {
    return getRealGeometry(winInfo->getID());
}

/**
 * @param win
 *
 * @return the _NET_USER_TIME property of win or 0 if unset
 */
uint32_t getUserTime(WindowID win);
/**
 * Sets the _NET_USER_TIME property on win to time
 *
 * @param win
 * @param time
 */
void setUserTime(WindowID win, uint32_t time);
#endif
