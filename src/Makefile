CC := gcc -std=c11
OPTIMIZATION_FLAGS ?= -Os
DEBUGGING_FLAGS ?= -g -rdynamic -Og
IGNORED_FLAGS ?= -Wno-parentheses -Wno-sign-compare -Wno-missing-braces -Wno-missing-field-initializers -Wno-int-to-pointer-cast -Wno-cast-function-type
ERROR_FLAGS ?= -Werror -Wall -Wextra 
CFLAGS ?= -Werror -Wincompatible-pointer-types ${ERROR_FLAGS} ${IGNORED_FLAGS}

TESTFLAGS := ${CFLAGS} ${DEBUGGING_FLAGS} -D MPX_TESTING --coverage -Wno-unused-function 
TESTLIBS ?= -lcheck -lm -lrt 
LDFLAGS ?= -s -lpthread -lX11 -lXi -lxcb -lxcb-xinput -lxcb-xtest -lxcb-ewmh -lxcb-icccm -lxcb-randr -lX11-xcb
#-pedantic
RELEASE_FLAGS := ${CFLAGS} ${OPTIMIZATION_FLAGS} 

LAYER0_SRCS := logger.c 
LAYER1_SRCS := mywm-util.c util.c 
LAYER2_SRCS := globals.c bindings.c xsession.c devices.c test-functions.c monitors.c events.c
LAYER3_SRCS := state.c layouts.c wmfunctions.c functions.c default-rules.c
LAYER4_SRCS := window-clone.c
LIBSRCS :=  mywm.c config.c

SRCS := mywm.c config.c ${LIBSRCS} ${LAYER4_SRCS} ${LAYER3_SRCS} ${LAYER2_SRCS} ${LAYER1_SRCS} ${LAYER0_SRCS}

CHECK_TEST_COVERAGE = gcov -f $(1) 2> /dev/null |grep -v "Function 'focusFollowMouse'"| grep -v "Function 'spawn'" |grep -A 1 "Function" | grep -v '100' |grep -B 1 "Lines e" && echo "test coverage is not high enough" && exit 2 || true
RUN_TEST = $(if $(QUICK) ,$(1), valgrind -q --leak-check=full --error-exitcode=123 $(1))


RUN_XTEST = Tests/xisolate $(call RUN_TEST, ./$@ )
RUN_XTEST_NO_DISPLAY = Tests/xisolate --no-xserver $(call RUN_TEST, ./$@ --test-no-xdisplay )

all: test mywm 

mywm: ${SRCS}
	gcc $^ -o $@  ${RELEASE_FLAGS} ${LDFLAGS}

quick-test: QUICK=1
quick-test: test	

test: unitTest1 unitTest2 unitTest3 unitTest4
		
unitTest1: Tests/Layer1Tests/*.c ${LAYER1_SRCS}
	${CC}  $^ -o $@ ${TESTFLAGS} ${TESTLIBS} 
	$(call RUN_TEST, ./$@ )
	$(call CHECK_TEST_COVERAGE,${LAYER1_SRCS}) 

unitTest2: Tests/*.c Tests/Layer2Tests/*.c ${LAYER2_SRCS} ${LAYER1_SRCS} ${LAYER0_SRCS}
	${CC} $^ -o $@  ${TESTFLAGS} ${TESTLIBS} ${LDFLAGS}
	$(call RUN_XTEST_NO_DISPLAY)
	$(call RUN_XTEST)
	$(call CHECK_TEST_COVERAGE,${LAYER2_SRCS})

unitTest3: Tests/*.c Tests/Layer3Tests/*.c ${LAYER3_SRCS} ${LAYER2_SRCS} ${LAYER1_SRCS} ${LAYER0_SRCS}
	${CC} $^ -o $@ ${TESTFLAGS} ${TESTLIBS} ${LDFLAGS}
	$(call RUN_XTEST)
	$(call CHECK_TEST_COVERAGE,${LAYER3_SRCS})
		
unitTest4: Tests/*.c Tests/Layer4Tests/* ${LAYER4_SRCS} ${LAYER3_SRCS} ${LAYER2_SRCS} ${LAYER1_SRCS} ${LAYER0_SRCS}
	${CC} $^ -o $@ ${TESTFLAGS} ${TESTLIBS} ${LDFLAGS}
	$(call RUN_XTEST)
	$(call CHECK_TEST_COVERAGE,${LAYER4_SRCS},100)

.PHONY: test all clean doc

.DELETE_ON_ERROR:

clean:
	rm -f mywm unitTest* *gc?? vgcore*
