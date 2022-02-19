#include <stdio.h>

#include "../bindings.h"
#include "xsession.h"

void dumbBinding(Binding* b) {
    if(isButton(b->buttonOrKey))
        printf("Mod: %d Button: %d\n", b->mod, b->buttonOrKey);
    else
        printf("Mod: %d Sym : %d Detail: %d Mask: %d\n", b->mod, b->buttonOrKey, b->detail, b->flags.mask);
}

