/**
 * @file mpxmanager.c
 * Entrypoint for MPXManager
 */
#include <assert.h>
#include <execinfo.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "args.h"
#include "default-rules.h"
#include "events.h"
#include "globals.h"
#include "logger.h"
#include "mywm-util.h"
#include "settings.h"

static void handler(int sig){
    fprintf(stderr, "Error: signal %d:\n", sig);
    printStackTrace();
    quit();
}

/**
 * @param argc
 * @param argv[]
 *
 * @return
 */
int main(int argc, char* argv[]){
    numPassedArguments = argc;
    passedArguments = argv;
    signal(SIGSEGV, handler);
    signal(SIGABRT, handler);
    signal(SIGKILL, handler);
    signal(SIGPIPE, resetPipe);
    //signal(SIGINT, quit);
    parseArgs(argc, argv, 1);
    startUpMethod = loadSettings;
    onStartup();
    runEventLoop(NULL);
}
