/**
 * @file wmfunctions.c
 * @brief Connect our internal structs to X
 *
 */

#ifndef MYWM_FUNCTIONS_H_
#define MYWM_FUNCTIONS_H_

#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI.h>

#include "mywm-structs.h"
#include "logger.h"
#include "mywm-util.h"

#define LEN(X) (sizeof X / sizeof X[0])


extern char* SHELL;
extern char MANAGE_OVERRIDE_REDIRECT_WINDOWS;

typedef enum{REMOVE,ADD,TOGGLE}MaskAction;



void msleep(int mil);


/**
 * Window Manager name used to comply with EWMH
 */
#define WM_NAME "My Window Manger"
/**XDisplay instance (only used for events/device commands)*/
extern Display *dpy;
/**XCB display instance*/
extern xcb_connection_t *dis;
/**EWMH instance*/
extern xcb_ewmh_connection_t *ewmh;
/**Root window*/
extern int root;
/**Default screen (assumed to be only 1*/
extern xcb_screen_t* screen;
extern int defaultScreenNumber;


extern int ROOT_EVENT_MASKS;
extern int NON_ROOT_EVENT_MASKS;
extern int DEVICE_EVENT_MASKS;

extern int CHAIN_BINDING_GRAB_MASKS;

extern int IGNORE_MASK;
extern int NUMBER_OF_WORKSPACES;
extern int IGNORE_MASK;
extern int NUMBER_OF_WORKSPACES;
extern int DEFAULT_BORDER_WIDTH;


int isShuttingDown();
/**
 * if deviceId is a slave, will return the master device;
 * else,  deviceId is a master and the parnter master id is returned
 * @param context
 * @param deviceId
 * @return
 */
int getAssociatedMasterDevice(int deviceId);
/**
 * @return the id of the master keyboard associated with the master pointer
 */
int getClientKeyboard();
/**
 * Wrapper around XIUngrabDevice
 * Ungrabs the keyboard or mouse
 *
 * @param id    id of the device to ungrab
 */
int ungrabDevice(int id);
/**
 * Wrapper around XIGrabDevice
 *
 * Grabs the keyboard or mouse
 * @param id    if of the device to grab
 * @param maskValue mask of the events to grab
 * @see XIGrabDevice
 */
int grabDevice(int deviceID,int maskValue);


/**
 * Do everything needed to comply when EWMH.
 * Creates a functionless window with set all the Atoms we support, clear any
 * unsporrted values from the root window
 */
void broadcastEWMHCompilence();

/**
 * Wraps XIGrabButton
 * @see XIGrabButton
 */
int grabButton(int deviceID,int button,int mod,int maskValue);
/**
 * Wraps XIUnGrabButton
 * @see XIGrabButton
 */
int ungrabButton(int deviceID,int button,int mod);

/**
 * Wraps XIGrabKey
 */
int grabKey(int deviceID,int keyCode,int mod,int mask);
/**
 *
 * Wraps XIUnGrabKey
 */
int ungrabKey(int deviceID,int keyCode,int mod);

/**
 * Sets the active master to be the device associated with  mouseSlaveId
 * @param mouseSlaveId either master pointer or slave pointerid
 * @return 1 iff mouseSlaveId was a master pointer device
 */
int setActiveMasterByMouseId(int mouseSlaveId);
/**
 * Sets the active master to be the device associated with keyboardSlaveId
 * @param keyboardSlaveId either master keyboard or slave keyboard (or master pointer)id
 * @return 1 iff keyboardSlaveId was a master keyboard device
 */
int setActiveMasterByKeyboardId(int keyboardSlaveId);

/**
 * Tile the layouts according to layout
 * @param workspace the workspace to tile
 * @param layout the layout to use
 */
void applyLayout(Workspace*workspace,Layout* layout);
/**
 * Retile all visible workspaces with their active layout
 */
void tileWindows();



/**
 * Sets the border color for the given window
 * @param win   the window in question
 * @param color the new window border color
 * @return 1 iff no error was deteced
 */
int setBorderColor(xcb_window_t win,unsigned int color);


/**
 * Switch to window's worksspace, raise and focus the window
 * @param id
 * @return
 */
int activateWindow(WindowInfo* winInfo);
/**
 * Focuses the given window
 * @param win   the window to focus
 * @return 1 iff no error was detected
 */
int focusWindow(xcb_window_t win);
/**
 * Raises the given window
 * @param win   the window to raise
 * @return 1 iff no error was detected
 */
int raiseWindow(xcb_window_t win);

int setUnmapped(xcb_window_t win);
int deleteWindow(xcb_window_t winToRemove);

/**
 * Forks and executes command
 * @param command the command to execute
 */
void runCommand(char* command);

/**
 *
 * @param winInfo
 * @return 1 iff external resize requests should be granted
 */
int isExternallyResizable(WindowInfo* winInfo);
/**
 *
 * @param winInfo
 * @return 1 iff external move requests should be granted
 */
int isExternallyMoveable(WindowInfo* winInfo);

/**
 *Removes,Adds or toggles the given mask for the given window
 * @param winInfo
 * @param mask
 * @param action an instance of MaskAction
 */
void updateState(WindowInfo*winInfo,int mask,MaskAction action);

/**
 * Adds the states give by mask to the window
 * @param winInfo
 * @param mask
 */
void addState(WindowInfo*winInfo,int mask);
/**
 * Removes the states give by mask from the window
 * @param winInfo
 * @param mask
 */
void removeState(WindowInfo*winInfo,int mask);

/**
 * Determines if a window should be tiled given its mapState and masks
 * @param winInfo
 * @return true if the window should be tiled
 */
int isTileable(WindowInfo* winInfo);
/**
 *
 * @param windowStack the stack of windows to check from
 * @return the number of windows that can be tiled
 */
int getNumberOfTileableWindows(Node*windowStack);
/**
 * Returns the max dims allowed for this window based on its mask
 * @param m the monitor the window is one
 * @param winInfo   the window
 * @param height    whether to check the height(1) or the width(0)
 * @return the max dimension or 0 if the window has not relevant mask
 */
int getMaxDimensionForWindow(Monitor*m,WindowInfo*winInfo,int height);


void removeWindowFromAllWorkspaces(WindowInfo* winInfo);
void setWorkspaceNames(char*names[],int numberOfNames);
void activateWorkspace(int workspaceIndex);
void moveWindowToWorkspace(WindowInfo* winInfo,int destIndex);
void moveWindowToWorkspaceAtLayer(WindowInfo *winInfo,int destIndex,int layer);

void toggleShowDesktop();


void switchToWorkspace(int workspaceIndex);
int checkError(xcb_void_cookie_t cookie,int cause,char*msg);
void scan();


void closeConnection();
void quit();
/**
 * Grab all specified keys/buttons and listen for root window events
 */
void registerForEvents();
/**
 * Called when an connection to the Xserver has been established
 */
void onXConnect();
/**
 * Establishes a connection with the X server.
 * Creates and Xlib and xcb display
 */
void connectToXserver();
/**
 * Query for all monitors
 */
void detectMonitors();

/**
 * Determines if and how a given window should be managed
 * @param winInfo
 */
void processNewWindow(WindowInfo* winInfo);
void avoidStruct(WindowInfo*info);

/**
 * Add all existing masters to the list of master devices
 */
void initCurrentMasters();
/**
 * Sets the window state based on a list of attoms
 * @param winInfo
 * @param atoms
 * @param numberOfAtoms
 * @param action
 */
void setWindowStateFromAtomInfo(WindowInfo*winInfo,xcb_atom_t* atoms,int numberOfAtoms,int action);

/**
 * Adds the window to our list of managed windows as a non-dock
 * and load any ewhm saved state
 * @param winInfo
 */
void registerWindow(WindowInfo*winInfo);

void detectMonitors();

void configureWindow(Monitor*m,WindowInfo* winInfo,short values[CONFIG_LEN]);



/**
 *
 * @param winInfo the window whose map state to query
 * @return the window map state either UNMAPPED(0) or MAPPED(1)
 */
int isMappable(WindowInfo* winInfo);
/**
 * Sets the mapped state to UNMAPPED(0) or MAPPED(1)
 * @param win the window whose state is being modified
 * @param state the new state
 * @return  true if the state actuall changed
 */
void updateMapState(int id,int state);
/**
 *
 * @param winInfo
 * @return true if the window can receive focus
 */
int isActivatable(WindowInfo* winInfo);
int isVisible(WindowInfo* winInfo);
void setVisible(WindowInfo* winInfo,int v);
void setDefaultWindowMasks(int mask);
//Modifes window masks that determines special propertie of window
/**
 *
 * @param info
 * @return The mask for the given window
 */
int getWindowMask(WindowInfo*info);


/**
 * Send a kill signal to the client with the window
 * @param win
 */
void killWindow(xcb_window_t win);

#endif
