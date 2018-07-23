CC := gcc
OPTIMIZATION_FLAGS ?= -Os
DEBUGGING_FLAGS ?= -g -rdynamic
CFLAGS ?= -Werror -Wincompatible-pointer-types 
IGNORED_FLAGS ?= -Wno-parentheses -Wno-sign-compare -Wno-unused-parameter -Wno-missing-braces -Wno-missing-field-initializers
ERROR_FLAGS ?= -pedantic -Werror -Wall -Wextra 
CFLAGS += ${IGNORED_FLAGS} ${DEBUGGING_FLAGS} -lpthread
 
TESTLIBS ?= -lcheck -lm -lrt
LDFLAGS ?= -s -lX11 -lXtst -lXi -lxcb -lxcb-xinput -lxcb-xtest -lxcb-ewmh -lxcb-icccm -lxcb-randr -lX11-xcb 

LIBSRCS := event-loop.c defaults.c extra-functions.c functions.c  layouts.c bindings.c wmfunctions.c logger.c mywm-util.c util.c

SRCS := mywm.c config.c


all: test xtest mywm 

mywm: ${SRCS} ${LIBSRCS} ${LIBHEADERS}
	@echo "Building mywm"
	gcc ${LIBSRCS} ${SRCS} -o mywm ${CFLAGS} ${LDFLAGS} ${ERROR_FLAGS}
	
test: unit-test testMem	testX11
	./unit-test
	
xtest: testX11
	./test-x11
unit-test: Tests/TestHelper.c Tests/UnitTests/*.c ${LIBSRCS} ${LIBHEADERS}
	@echo "building test"
	${CC} ${LIBSRCS} Tests/UnitTests/UnitTests.c -o unit-test ${CFLAGS} ${LDFLAGS} ${TESTLIBS}
		
	
testX11: Tests/TestHelper.c Tests/TestX11.c ${LIBSRCS} ${LIBHEADERS}
	${CC} ${LIBSRCS} Tests/TestX11.c -o testX11 ${CFLAGS} ${LDFLAGS} ${TESTLIBS}	

testMem: Tests/TestHelper.c Tests/MemTest/TestMemoryLeaks.c ${LIBSRCS} ${LIBHEADERS}
	${CC} ${LIBSRCS} Tests/MemTest/TestMemoryLeaks.c -o testMem ${CFLAGS} ${LDFLAGS} ${TESTLIBS}	
	 	

.PHONY: test all clean

clean:
	rm -f mywm testX11 unit-test testMem
