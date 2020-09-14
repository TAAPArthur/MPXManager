#include <assert.h>
#include <string.h>
#include "unistd.h"

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <X11/Xlib-xcb.h>

#include <stdio.h>

#include "xsession.h"


void dumpAtoms(const xcb_atom_t* atoms, int numberOfAtoms) {
    printf("Atoms %d:", numberOfAtoms);
    char buffer[MAX_NAME_LEN];
    for(int i = 0; i < numberOfAtoms; i++) {
        printf(" %s (%d)", getAtomName(atoms[i], buffer), atoms[i]);
    }
    printf("\n");
}

xcb_atom_t getAtom(const char* name) {
    if(!name)return XCB_ATOM_NONE;
    xcb_intern_atom_reply_t* reply;
    reply = xcb_intern_atom_reply(dis, xcb_intern_atom(dis, 0, strlen(name), name), NULL);
    xcb_atom_t atom = reply->atom;
    free(reply);
    return atom;
}
char* getAtomName(xcb_atom_t atom, char* buffer) {
    xcb_get_atom_name_reply_t* valueReply = xcb_get_atom_name_reply(dis, xcb_get_atom_name(dis, atom), NULL);
    if(valueReply) {
        strncpy(buffer, xcb_get_atom_name_name(valueReply), MIN_NAME_LEN(valueReply->name_len));
        buffer[MIN_NAME_LEN(valueReply->name_len)] = 0;
        free(valueReply);
    }
    else {
        *buffer = 0;
    }
    return buffer;
}

xcb_get_property_reply_t* getWindowProperty(WindowID win, xcb_atom_t atom, xcb_atom_t type) {
    xcb_get_property_reply_t* reply;
    xcb_get_property_cookie_t cookie = xcb_get_property(dis, 0, win, atom, type, 0, -1);
    if((reply = xcb_get_property_reply(dis, cookie, NULL)))
        if(xcb_get_property_value_length(reply))
            return reply;
        else free(reply);
    return NULL;
}
int getWindowPropertyValueInt(WindowID win, xcb_atom_t atom, xcb_atom_t type) {
    xcb_get_property_reply_t* reply = getWindowProperty(win, atom, type);
    int result = reply ? *(int*)xcb_get_property_value(reply) : 0;
    free(reply);
    return result;
}


char* getWindowPropertyString(WindowID win, xcb_atom_t atom, xcb_atom_t type, char* result) {
    xcb_get_property_reply_t* reply = getWindowProperty(win, atom, type);
    if(reply) {
        strcpy(result, (char*)xcb_get_property_value(reply));
        free(reply);
    }
    else result[0] = 0;
    return result;
}


int getButtonDetailOrKeyCode(int buttonOrKey) {
    return isButton(buttonOrKey) ? buttonOrKey : getKeyCode(buttonOrKey);
}
int getKeyCode(int keysym) {
    return XKeysymToKeycode(dpy, keysym);
}
