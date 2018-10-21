/*
 * ewmh.h
 *
 *  Created on: Jul 14, 2018
 *      Author: arthur
 */

#ifndef EWMH_H_
#define EWMH_H_

#define ALLOWED_ACTIONS\
    ewmh->_NET_WM_ACTION_ABOVE, \
    ewmh->_NET_WM_ACTION_BELOW,\
    ewmh->_NET_WM_ACTION_CHANGE_DESKTOP,\
    ewmh->_NET_WM_ACTION_CLOSE,\
    ewmh->_NET_WM_ACTION_FULLSCREEN,\
    ewmh->_NET_WM_ACTION_MAXIMIZE_HORZ,\
    ewmh->_NET_WM_ACTION_MAXIMIZE_VERT,\
    ewmh->_NET_WM_ACTION_MINIMIZE,\
    ewmh->_NET_WM_ACTION_STICK,\
    ewmh->_NET_WM_ACTION_MOVE,\
    ewmh->_NET_WM_ACTION_RESIZE,\
    ewmh->_NET_WM_ALLOWED_ACTIONS

#define SET_ALLOWED_ACTIONS(ewmh) {\
    xcb_atom_t actions[] = {ALLOWED_ACTIONS};\
    xcb_ewmh_set_wm_allowed_actions_checked(ewmh, win, LEN(actions), list);\
    }

#define SUPPORTED_STATES\
       ewmh->_NET_WM_STATE_ABOVE,\
       ewmh->_NET_WM_STATE_BELOW,\
       ewmh->_NET_WM_STATE_DEMANDS_ATTENTION,\
       ewmh->_NET_WM_STATE_FULLSCREEN,\
       ewmh->_NET_WM_STATE_HIDDEN,\
       ewmh->_NET_WM_STATE_MAXIMIZED_HORZ,\
       ewmh->_NET_WM_STATE_MAXIMIZED_VERT,\
       ewmh->_NET_WM_STATE_STICKY,\
       WM_STATE_ROOT_FULLSCREEN,\
       WM_STATE_NO_TILE,\

#define SUPPORTED_TYPES \
       ewmh->_NET_WM_WINDOW_TYPE_NORMAL,\
       ewmh->_NET_WM_WINDOW_TYPE_DOCK,\
       ewmh->_NET_WM_WINDOW_TYPE_DIALOG,\
       ewmh->_NET_WM_WINDOW_TYPE_UTILITY,\
       ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR,\
       ewmh->_NET_WM_WINDOW_TYPE_SPLASH,\
       ewmh->_NET_WM_WINDOW_TYPE_MENU,\
       ewmh->_NET_WM_WINDOW_TYPE_DROPDOWN_MENU,\
       ewmh->_NET_WM_WINDOW_TYPE_POPUP_MENU,\
       ewmh->_NET_WM_WINDOW_TYPE_NOTIFICATION,
/**
 * Sets all the ewmh we support.
 *
 * We don't have true support for desktop geometry like atoms; they don't align with out multi monitor view
 * _NET_DESKTOP_GEOMETRY,_NET_DESKTOP_VIEWPORT,_NET_WORKAREA are have the "default" values
 */
#define SET_SUPPORTED_OPERATIONS(ewmh) {\
    xcb_atom_t net_atoms[] = { \
           ewmh->_NET_SUPPORTED,\
           ewmh->_NET_SUPPORTING_WM_CHECK,\
           ewmh->_NET_WM_NAME,\
           ewmh->_NET_WM_MOVERESIZE,\
           \
           ewmh->_NET_CLIENT_LIST,\
           ewmh->_NET_CLIENT_LIST_STACKING,\
           \
           ewmh->_NET_WM_STRUT, ewmh->_NET_WM_STRUT_PARTIAL,\
           ewmh->_NET_WM_STATE,\
               SUPPORTED_STATES\
           ewmh->_NET_WM_WINDOW_TYPE,\
               SUPPORTED_TYPES\
           ewmh->_NET_ACTIVE_WINDOW,ewmh->_NET_CLOSE_WINDOW,\
           ewmh->_NET_WM_DESKTOP,\
               ewmh->_NET_NUMBER_OF_DESKTOPS,\
               ewmh->_NET_CURRENT_DESKTOP,\
               ewmh->_NET_SHOWING_DESKTOP,\
               ewmh->_NET_DESKTOP_NAMES,\
               ewmh->_NET_DESKTOP_VIEWPORT,\
           ALLOWED_ACTIONS,\
           ewmh->_NET_DESKTOP_GEOMETRY,ewmh->_NET_DESKTOP_VIEWPORT,ewmh->_NET_WORKAREA\
           };\
           xcb_ewmh_set_supported_checked(ewmh, defaultScreenNumber,\
            sizeof(net_atoms) / sizeof(xcb_atom_t) , net_atoms);\
        }

#endif /* EWMH_H_ */
