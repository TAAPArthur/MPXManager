#ifndef MPX_TESTER_H
#define MPX_TESTER_H
#include <assert.h>
#include <execinfo.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <iostream>

#include "../logger.h"
#include "../time.h"
#include "../globals.h"

#define WAIT_UNTIL_TRUE(COND,EXPR...) do{msleep(10);EXPR;}while(!(COND))


#define assertAndDelete(X) do {auto* _x=X;assert(_x); delete _x;} while(0)
#define assertEquals(A,B)do{auto __A=A; auto __B =B; int __result=(__A==__B); if(!__result){std::cout<<__A<<"!="<<__B<<"\n";assert(0 && #A "!=" #B);}}while(0)

#define MPX_TEST(name,body...) MPX_TEST_ITER(name,1,body)
#define MPX_TEST_ITER(name,end,body...) MPX_TEST_ITER_ERR(name,end,0,body)
#define MPX_TEST_ERR(name,err,body...) MPX_TEST_ITER_ERR(name,1,err,body)
#define MPX_TEST_ITER_ERR(name,end,err,body...)static  Test _CAT(test,__LINE__)=Test(name,[](int _i __attribute__((unused)))body,end,err,"" __FILE__,__LINE__);

#define SET_ENV(env...) static Env _CAT(e,__LINE__) __attribute__((unused))= Env(env,__FILE__)

void waitForAllThreadsToExit();

static void(*_setup)(void) = NULL;
static void(*_teardown)(void) = NULL;
static const char* _file = "";
struct Env {
    Env(void(*setup)(void), void(*teardown)(void), const char* f) {
        _setup = setup;
        _teardown = teardown;
        _file = f;
    }
};

struct Test ;
extern std::vector<Test*>tests;
struct Test {
    const char* name;
    void(*const func)(int);
    const int end;
    const int exitCode;
    const char* fileName;
    const int lineNumber;
    void(*testSetup)(void) = NULL;
    void(*testTeardown)(void) = NULL;
    char status[64];
    const int testNumber;
    Test(const char* name, void(*const func)(int), int end, int exitCode, const char* fileName,
        const int lineNumber): name(name), func(func), end(end), exitCode(exitCode), fileName(fileName),
        lineNumber(lineNumber), testNumber(tests.size()) {
        tests.push_back(this);
        if(strcmp(fileName, _file) == 0) {
            testSetup = _setup;
            testTeardown = _teardown;
        }
    }
    int runTest(int rem);
    void printSummary(int i);
};
void breakFork(int i);
#endif
