/**
 * TODO
 * Complex Functions -- done
 * Layers
 * State
 * Printing 1hr
 * Common functions
 *
 * Testing
 * Doc
 *
 * Layouts
 * Fake monitors
 *
 * Restart/recompile
 *
 * Convience functions (focus follows mouse) -- done
 * Scripting //2hr
 *
 * Optimization/Benchmarking
 *
 * Patch mode  -- done
 */

#include <execinfo.h>
#include <signal.h>
#include <unistd.h>

#include <getopt.h>
#include <stdio.h>

#include "config.h"
#include "logger.h"
#include "events.h"
#include "default-rules.h"
#include "mywm-util.h"

int enterEventLoop=1;
int runAsWindowManager=1;
int numberOfWorkspaces=10;

void handler(int sig) {
  void *array[32];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, LEN(array));

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

void parseAgrs(int argc, char * argv[]){
    int             c;
    const char    * short_opt = "pn";
    struct option   long_opt[] =
    {
      {"help",          no_argument,       NULL, 'h'},
      {"patch",          no_argument, NULL, 'p'},
      {"no-event-loop",          no_argument, NULL, 'n'},
      {"crash-on-error",          no_argument, NULL, 'c'},
      {NULL,            0,                 NULL, 0  }
    };

    while((c = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1){
      switch(c)
      {
         case -1:       /* no more arguments */
         case 0:        /* long options toggles */
         break;
         case 'c':
            CRASH_ON_ERRORS=1;
            break;
         case 'n':
             enterEventLoop=0;
             break;
         case 'p':
             runAsWindowManager=0;
             break;
         case 'h':
             break;
      }
    }
}

int main(int argc, char * argv[]){
    signal(SIGSEGV, handler);
    signal(SIGABRT, handler);
    //signal(SIGINT, quit);

    parseAgrs(argc,argv);
    //TODO make var

    startUpMethod=loadSettings;
    onStartup();
    if(enterEventLoop)
        runEventLoop(NULL);
}
