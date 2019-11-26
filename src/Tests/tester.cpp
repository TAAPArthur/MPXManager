#include <errno.h>
#include <pthread.h>
#include <string>

#include "../logger.h"
#include "../system.h"
#include "tester.h"

void breakFork(int i) {
    char strValue[8];
    sprintf(strValue, "%d", i);
    setenv("BREAK_FORK", strValue, 1);
}
static int TIMEOUT = 5;
void* timer(void* f) {
    const volatile int* flag = (int*)f;
    for(int i = 0; *flag && i < TIMEOUT * 100; i++) {
        msleep(10);
    }
    int pid = *flag;
    if(pid) {
        printf("Timeout %d\n", pid);
        printf("Sending signal to print stacktrace%d\n", pid);
        kill(pid, SIGUSR2);
        msleep(1000);
        if(kill(pid, SIGKILL)) {
            LOG(LOG_LEVEL_ERROR, "killing failed %s\n", strerror(errno));
        }
    }
    return NULL;
}
char* noForkStr = getenv("NO_FORK");;
bool noFork = noForkStr ? std::string(noForkStr) != "0" : 0;

int Test::runTest(int i) {
    assert(this->end);
    pthread_t thread;
    int flag = 1;
    if(noFork || !(flag = fork())) {
        if(!noFork)
            signal(SIGINT, NULL);
        printf("%s:%d %s.%d \n", fileName, lineNumber, name, i);
        if(testSetup)
            testSetup();
        if(exitCode && this->testTeardown) {
            assert(atexit(this->testTeardown) == 0);
        }
        func(i);
        if(this->testTeardown)
            this->testTeardown();
        if(!noFork)
            quit(0);
    }
    if(!noFork) {
        pthread_create(&thread, NULL, timer, (void*)&flag);
        status[i] = waitForChild(flag);
        flag = 0;
        pthread_join(thread, NULL);
    }
    return status[i] == exitCode;
}
void Test::printSummary(int i) {
    printf("%s:%d %s.%d (%d of %d) #%02d failed with status %d\n", fileName, lineNumber, name, i, i + 1, end, testNumber,
        status[i]);
}
struct FailedTest {Test* t; int index;};

const char* Env::file = "";
std::vector<Test*>tests;
std::vector<FailedTest>failedTests;
size_t passedCount = 0;
int exitCode;
void printResults(int signal = 0) {
    exitCode = signal ? signal : passedCount && failedTests.empty() ? 0 : 1;
    if(signal)
        printf("aborting\n");
    printf("....................%d\n", exitCode);
    printf("Passed %ld/%ld tests\n", passedCount, passedCount + failedTests.size());
    for(FailedTest& test : failedTests)
        test.t->printSummary(test.index);
    if(signal)
        exit(exitCode);
}
bool runTest(Test* t, int i) {
    if(!t->runTest(i)) {
        failedTests.push_back({t, i});
        return 0;
    }
    else passedCount++;
    return 1;
}
int main(int argc, char* const* argv) {
    assert(tests.size());
    char* startingFrom = getenv("STARTING_FROM");
    char* file = getenv("TEST_FILE");
    char* func = getenv("TEST_FUNC");
    char* index = func ? strchr(func, '.') : NULL;
    if(index) {
        *index = 0;
        index++;
    }
    char* timeout = getenv("TIMEOUT");
    if(timeout)
        TIMEOUT = atoi(timeout);
    if(argc >= 2) {
        TIMEOUT = atoi(argv[1]);
    }
    char* strictStr = getenv("STRICT");;
    bool strict = strictStr ? std::string(strictStr) != "0" : 0;
    bool veryStrict = strict && strictStr ? std::string(strictStr) != "1" : 0;
    char* failStr = getenv("MAX_FAILS");;
    uint32_t maxFails = failStr ? std::stoi(failStr) : -1;
    setLogLevel(LOG_LEVEL_ERROR);
    CRASH_ON_ERRORS = -1;
    signal(SIGINT, printResults);
    for(Test* t : tests)
        if((!file || strcmp(file, t->fileName) == 0) &&
            (!func || strcmp(func, t->name) == 0) &&
            (!startingFrom || strcmp(startingFrom, t->fileName) == 0 || strcmp(startingFrom, t->name) == 0 || passedCount ||
                failedTests.size())
        ) {
            if(index)
                runTest(t, atoi(index));
            else
                for(int i = 0; i < t->end; i++)
                    if(!runTest(t, i) && veryStrict)
                        break;
            if(strict && failedTests.size())
                break;
            if(failedTests.size() >= maxFails)
                break;
        }
    printResults();
    return exitCode;
}
