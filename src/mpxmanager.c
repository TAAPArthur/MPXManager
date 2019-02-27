#include <execinfo.h>
#include <signal.h>
#include <unistd.h>

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "mywm-util.h"
#include "config.h"
#include "events.h"
#include "default-rules.h"
#include "args.h"

static void handler(int sig){
    void* array[32];
    size_t size;
    // get void*'s for all entries on the stack
    size = backtrace(array, LEN(array));
    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}

int main(int argc, char* argv[]){
    numPassedArguments = argc;
    passedArguments = argv;
    signal(SIGSEGV, handler);
    signal(SIGABRT, handler);
    //signal(SIGINT, quit);
    parseArgs(argc, argv, 1);
    startUpMethod = loadSettings;
    onStartup();
    runEventLoop(NULL);
}
