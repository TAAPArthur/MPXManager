CC := gcc
OPTIMIZATION_FLAGS ?= -Os
DEBUGGING_FLAGS ?= -g -rdynamic
CFLAGS ?= -Werror -Wincompatible-pointer-types 
IGNORED_FLAGS ?= -Wno-parentheses -Wno-missing-field-initializers
ERROR_FLAGS ?= -pedantic -Werror -Wall -Wextra 
CFLAGS += ${DEBUGGING_FLAGS} ${IGNORED_FLAGS}

TESTLIBS ?= -lcheck -lm -lrt --coverage
LDFLAGS ?= -s -lpthread -lX11 -lXi -lxcb -lxcb-xinput -lxcb-xtest -lxcb-ewmh -lxcb-icccm -lxcb-randr -lX11-xcb


LAYER1_SRCS := mywm-util.c util.c
LAYER2_SRCS := defaults.c devices.c logger.c bindings.c
LAYER3_SRCS := event-loop.c default-rules.c wmfunctions.c  window-properties.c 
LAYER4_SRCS := extra-functions.c functions.c layouts.c
LIBSRCS :=  mywm.c config.c

CHECK_TEST_COVERAGE = gcov -f $(1) 2> /dev/null | grep -v -E '$(2)' |grep -B 1 "Lines e" && echo "test coverage is not high enough" && exit 2 || true
SRCS := mywm.c config.c

all: unitTest1 unitTest2 unitTest3 # layer4 # mywm 

mywm: ${SRCS} ${LIBSRCS}
	@echo "Building mywm"
	gcc ${LIBSRCS} -o mywm ${CFLAGS} ${LDFLAGS} ${ERROR_FLAGS}
	
test: unit-test testMem	testX11
	./test-x11
		
unitTest1: $(wildcard Tests/Layer1Tests/*.c) ${LAYER1_SRCS}
	${CC}  $^ -o $@ ${CFLAGS} ${TESTLIBS}
	./$@
	$(call CHECK_TEST_COVERAGE,${LAYER1_SRCS},100) 

unitTest2: Tests/Layer2Tests/*.c ${LAYER2_SRCS} ${LAYER1_SRCS}
	${CC} $^ -o $@ ${CFLAGS} ${LDFLAGS} ${TESTLIBS}
	./test-x11 ./$@
	$(call CHECK_TEST_COVERAGE,${LAYER2_SRCS},100)

unitTest3: Tests/*.c $(wildcard Tests/Layer3Tests/*.c) ${LAYER3_SRCS} ${LAYER2_SRCS} ${LAYER1_SRCS}
	${CC} $^ -o $@ ${CFLAGS} ${LDFLAGS} ${TESTLIBS}
	./test-x11 ./$@	
	$(call CHECK_TEST_COVERAGE,${LAYER3_SRCS},100)

testMem: Tests/TextX11Helper.c Tests/MemTest/TestMemoryLeaks.c ${LIBSRCS} 
	${CC} ${LIBSRCS} Tests/TextX11Helper.c Tests/MemTest/TestMemoryLeaks.c -o testMem ${CFLAGS} ${LDFLAGS} ${TESTLIBS}
		
layer4: Tests/TestHelper.c Tests/Layer4Tests/* ${LAYER4_SRCS} ${LAYER3_SRCS} ${LAYER2_SRCS} ${LAYER1_SRCS}
	${CC} ${LAYER4_SRCS} ${LAYER3_SRCS} ${LAYER2_SRCS} ${LAYER1_SRCS} Tests/TestX11Helper.c Tests/Layer4Tests/*.c -o unit-test4 ${CFLAGS} ${LDFLAGS} ${TESTLIBS}

.PHONY: test all clean

.DELETE_ON_ERROR:

clean:
	rm -f mywm testX11 unitTest* testMem
