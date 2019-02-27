CC := gcc -std=c11
OPTIMIZATION_FLAGS ?= -O3
DEBUGGING_FLAGS ?= -g -rdynamic -O0
IGNORED_FLAGS ?= -Wno-parentheses -Wno-sign-compare -Wno-missing-braces -Wno-missing-field-initializers -Wno-int-to-pointer-cast -Wno-cast-function-type
ERROR_FLAGS ?= -Werror -Wall -Wextra
CFLAGS ?= -Werror -Wincompatible-pointer-types ${ERROR_FLAGS} ${IGNORED_FLAGS} -D_DEFAULT_SOURCE

TESTFLAGS := ${CFLAGS} ${DEBUGGING_FLAGS} -DINIT_LOG_LEVEL=LOG_LEVEL_ERROR -D MPX_TESTING  --coverage -Wno-unused-function
TESTLIBS ?= -lcheck
LDFLAGS ?= -lpthread -lm -lX11 -lXi -lxcb -lxcb-xinput -lxcb-xtest -lxcb-ewmh -lxcb-icccm -lxcb-randr -lX11-xcb
#-pedantic
RELEASE_FLAGS := ${CFLAGS} ${OPTIMIZATION_FLAGS} -DNDEBUG
DEBUG_FLAGS := ${CFLAGS} ${DEBUGGING_FLAGS} -DINIT_LOG_LEVEL=LOG_LEVEL_DEBUG

LAYER0_SRCS := logger.c globals.c 
LAYER1_SRCS := util.c 
LAYER2_SRCS := xsession.c test-functions.c masters.c monitors.c workspaces.c devices.c bindings.c mywm-util.c events.c windows.c
LAYER3_SRCS := layouts.c wmfunctions.c default-rules.c state.c
LAYER4_SRCS := mpx.c window-clone.c functions.c
SRCS := mywm.c config.c ${LAYER4_SRCS} ${LAYER3_SRCS} ${LAYER2_SRCS} ${LAYER1_SRCS} ${LAYER0_SRCS}


CHECK_TEST_COVERAGE = gcov -f $(1) 2> /dev/null |grep -v -P "Function '(spawn|spawnPipe)'" |grep -A 1 "Function" | grep -v '100' |grep -B 1 "Lines e" && echo "test coverage is not high enough" && exit 2 || true
RUN_TEST = $(if $(QUICK) ,$(1), valgrind -q --leak-check=full --error-exitcode=123 $(1))


RUN_XTEST = Tests/xisolate $(call RUN_TEST, ./$@ )
RUN_XTEST_NO_DISPLAY = Tests/xisolate --no-xserver $(call RUN_TEST, ./$@ --test-no-xdisplay )

all: quick-test mywm

mywm: ${SRCS}
	${CC} $^ -o $@  ${RELEASE_FLAGS} ${LDFLAGS}
debug: ${SRCS}
	${CC} $^ -o $@  ${DEBUG_FLAGS} ${LDFLAGS}

quick-test: QUICK=1
quick-test: test

test: unitTest1 unitTest2 unitTest3 unitTest4
		
unitTest1: Tests/Layer1Tests/*.c ${LAYER1_SRCS}
	${CC} $^ -o $@  ${TESTFLAGS} ${TESTLIBS} ${LDFLAGS}
	$(call RUN_TEST, ./$@ )
	$(call CHECK_TEST_COVERAGE,${LAYER1_SRCS})

unitTest2: Tests/*.c Tests/Layer2Tests/*.c ${LAYER2_SRCS} ${LAYER1_SRCS}
	${CC} ${LAYER0_SRCS} $^ -o $@  ${TESTFLAGS} ${TESTLIBS} ${LDFLAGS}
	$(call RUN_XTEST_NO_DISPLAY)
	$(call RUN_XTEST)
	$(call CHECK_TEST_COVERAGE,${LAYER2_SRCS})

unitTest3: Tests/*.c Tests/Layer3Tests/*.c ${LAYER3_SRCS} ${LAYER2_SRCS} ${LAYER1_SRCS} ${LAYER0_SRCS}
	${CC} $^ -o $@ ${TESTFLAGS} ${TESTLIBS} ${LDFLAGS}
	$(call RUN_XTEST)
	$(call CHECK_TEST_COVERAGE,${LAYER3_SRCS})
		
unitTest4: Tests/*.c Tests/Layer4Tests/*.c ${LAYER4_SRCS} ${LAYER3_SRCS} ${LAYER2_SRCS} ${LAYER1_SRCS} ${LAYER0_SRCS}
	${CC} $^ -o $@ ${TESTFLAGS} ${TESTLIBS} ${LDFLAGS}
	$(call RUN_XTEST)
	$(call CHECK_TEST_COVERAGE,${LAYER4_SRCS},100)

format:

	find . \( -name '*.c' -o -name '*.h' \) -exec bash -c 't=$$(stat -c %y "$$0"); astyle --project "$$0" >/dev/null && sed -ri "s/\)\s+\{/){/g" "$$0"; touch -d "$$t" "$$0"' {} \;

doc:
	+$(MAKE) -C .. $@
.PHONY: test all clean doc format

.DELETE_ON_ERROR:

clean:
	rm -f mywm debug unitTest* *gc?? vgcore*
