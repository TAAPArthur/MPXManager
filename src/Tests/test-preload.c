#include "../ld-preload.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>


INTERCEPT(__pid_t, fork){
    char* var = getenv("BREAK_FORK");
    if(var)
        return atoi(var);
    else
        return BASE(fork)();
}
END_INTERCEPT
