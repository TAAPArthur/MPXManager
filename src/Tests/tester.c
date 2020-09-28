#include <scutest/tester.h>
#include <stdlib.h>

#include "tester.h"
int main() {
    char* logLevelStr = getenv("LOG_LEVEL");;
    if(logLevelStr)
        setLogLevel(atoi(logLevelStr));
    else
        setLogLevel(LOG_LEVEL_INFO);
    return runUnitTests();
}
