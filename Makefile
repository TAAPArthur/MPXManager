CC := gcc
OPTIMIZATION_FLAGS ?= -Os
DEBUGGING_FLAGS ?= -g -rdynamic
CFLAGS ?= -Werror -Wincompatible-pointer-types 
IGNORED_FLAGS ?= -Wno-parentheses -Wno-missing-field-initializers
ERROR_FLAGS ?= -pedantic -Werror -Wall -Wextra 
CFLAGS += ${DEBUGGING_FLAGS} ${IGNORED_FLAGS}

TESTFLAGS := --coverage -Werror -Wall -Wextra -Wno-sign-compare -Wno-missing-braces -Wno-unused-function 
TESTLIBS ?= -lcheck -lm -lrt 
LDFLAGS ?= -s -lpthread -lX11 -lXi -lxcb -lxcb-xinput -lxcb-xtest -lxcb-ewmh -lxcb-icccm -lxcb-randr -lX11-xcb

LAYER0_SRCS := defaults.c logger.c xsession.c
LAYER1_SRCS := mywm-util.c util.c
LAYER2_SRCS := events.c devices.c bindings.c test-functions.c
LAYER3_SRCS := default-rules.c wmfunctions.c window-properties.c window-clone.c 
LAYER4_SRCS := layouts.c functions.c 
LIBSRCS :=  mywm.c config.c

SRCS := ${LIBSRCS} ${LAYER4_SRCS} ${LAYER3_SRCS} ${LAYER2_SRCS} ${LAYER1_SRCS} ${LAYER0_SRCS}

CHECK_TEST_COVERAGE = gcov -f $(1) 2> /dev/null | grep -v -E '$(2)' |grep -B 1 "Lines e" && echo "test coverage is not high enough" && exit 2 || true
RUN_TEST = valgrind -q --leak-check=full --error-exitcode=123 $(1)
RUN_XTEST = ./xisolate $(call RUN_TEST, ./$@ )
SRCS := mywm.c config.c

all: unitTest1 unitTest2 unitTest3 # layer4 # mywm 

mywm: ${SRCS}
	gcc $^ -o $@ ${CFLAGS} ${LDFLAGS} ${ERROR_FLAGS}
	
test: unit-test testMem	testX11
	./test-x11
		
unitTest1: Tests/Layer1Tests/*.c ${LAYER1_SRCS}
	${CC}  $^ -o $@ ${CFLAGS} ${TESTLIBS} ${TESTFLAGS}
	$(call RUN_TEST, ./$@ )
	$(call CHECK_TEST_COVERAGE,${LAYER1_SRCS},100) 

unitTest2: Tests/*.c Tests/Layer2Tests/*.c ${LAYER2_SRCS} ${LAYER1_SRCS} ${LAYER0_SRCS}
	${CC} $^ -o $@ ${CFLAGS} ${LDFLAGS} ${TESTLIBS} ${TESTFLAGS}
	$(call RUN_XTEST)
	$(call CHECK_TEST_COVERAGE,${LAYER2_SRCS},100)

unitTest3: Tests/*.c Tests/Layer3Tests/*.c ${LAYER3_SRCS} ${LAYER2_SRCS} ${LAYER1_SRCS} ${LAYER0_SRCS}
	${CC} $^ -o $@ ${CFLAGS} ${LDFLAGS} ${TESTLIBS}
	./test-x11 ./$@	
	$(call CHECK_TEST_COVERAGE,${LAYER3_SRCS},100)

testMem: Tests/TextX11Helper.c Tests/MemTest/TestMemoryLeaks.c ${LIBSRCS} 
	${CC} ${LIBSRCS} Tests/TextX11Helper.c Tests/MemTest/TestMemoryLeaks.c -o testMem ${CFLAGS} ${LDFLAGS} ${TESTLIBS}
		
layer4: Tests/TestHelper.c Tests/Layer4Tests/* ${LAYER4_SRCS} ${LAYER3_SRCS} ${LAYER2_SRCS} ${LAYER1_SRCS}
	${CC} ${LAYER4_SRCS} ${LAYER3_SRCS} ${LAYER2_SRCS} ${LAYER1_SRCS} Tests/TestX11Helper.c Tests/Layer4Tests/*.c -o unit-test4 ${CFLAGS} ${LDFLAGS} ${TESTLIBS}

doc:
	doxygen dconfig 

.PHONY: test all clean doc

.DELETE_ON_ERROR:

clean:
	rm -f mywm testX11 unitTest* testMem clone
