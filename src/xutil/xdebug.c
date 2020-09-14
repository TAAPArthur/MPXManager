#include <stdio.h>

#include "../bindings.h"
#include "xsession.h"

void dumbBinding(Binding* b) {
    if(isButton(b->buttonOrKey))
        printf("%d %d\n", b->mod, b->buttonOrKey);
    else
        printf("%d %s %d %d\n", b->mod, XKeysymToString(b->buttonOrKey), b->buttonOrKey, b->detail);
}

