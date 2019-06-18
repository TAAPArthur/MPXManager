CC := gcc -std=gnu11
SHELL := /bin/bash
DEBUGGING_FLAGS := -g -rdynamic -O0
RELEASE_FLAGS := -g -O3 -flto -DNDEBUG
PROFILE_FLAGS := -g -rdynamic -pg  -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable
SPEED_TEST_FLAGS := -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable

IGNORED_FLAGS := -Wno-parentheses -Wno-sign-compare -Wno-missing-braces -Wno-missing-field-initializers -Wno-cast-function-type
ERROR_FLAGS := -Werror -Wincompatible-pointer-types -Wall -Wextra
CFLAGS := ${ERROR_FLAGS} ${IGNORED_FLAGS} -D_DEFAULT_SOURCE ${INJECT}

TESTFLAGS := ${CFLAGS} ${DEBUGGING_FLAGS} -DINIT_LOG_LEVEL=LOG_LEVEL_ERROR -D MPX_TESTING  --coverage -Wno-unused-function
TESTLIBS := -lcheck
LDFLAGS :=  -lm -lX11 -lXi -lxcb -lxcb-xinput -lxcb-xtest -lxcb-ewmh -lxcb-icccm -lxcb-randr -lX11-xcb

LAYER0_SRCS := logger.c globals.c
LAYER1_SRCS := util.c
LAYER2_SRCS := xsession.c test-functions.c masters.c spawn.c monitors.c workspaces.c devices.c bindings.c mywm-util.c events.c windows.c
LAYER3_SRCS := state.c layouts.c wmfunctions.c default-rules.c
LAYER4_SRCS := communications.c functions.c Extensions/*.c
PROFILE_LAYERS := settings.c ${LAYER4_SRCS} ${LAYER3_SRCS} ${LAYER2_SRCS} ${LAYER1_SRCS} ${LAYER0_SRCS}
TOP_LAYER_SRCS := settings.c mpxmanager.c
BASE_SRCS := ${TOP_LAYER_SRCS} ${LAYER4_SRCS} ${LAYER3_SRCS} ${LAYER2_SRCS} ${LAYER1_SRCS} ${LAYER0_SRCS}
SRCS := ${BASE_SRCS} config.c 


CHECK_TEST_COVERAGE = [ -z "$$CK_RUN_CASE$$CK_RUN_SUITE" ] && (gcov -f $$(basename -a $(1)) 2> /dev/null |grep -v -P "Function '(spawn|spawnPipe|restart|handler)'" |grep -A 1 "Function" | grep -v '100' |grep -B 1 "Lines e" && echo "test coverage is not high enough" && exit 2 || rm -f *.gc??)
RUN_TEST = $(if $(QUICK) ,$(1), valgrind -q --leak-check=full --error-exitcode=123 $(1))


RUN_XTEST = xvfb-run -a $(call RUN_TEST, $(1) )
RUN_XTEST_NO_DISPLAY = DISPLAY=:1000 $(call RUN_TEST, $(1) --test-no-xdisplay )

NO_PTHREADS ?= 0
ifeq ($(NO_PTHREADS),1)
	CFLAGS += -DNO_PTHREADS=1
	TESTFLAGS += -DNO_PTHREADS=1
else
	LDFLAGS += -lpthread
endif
NO_XRANDR ?= 0
ifeq ($(NO_XRANDR),1)
	CFLAGS += -DNO_XRANDR=1
	TESTFLAGS += -DNO_XRANDR=1
endif

ifeq ($(DEBUG),1)
  CFLAGS += ${DEBUGGING_FLAGS} -DINIT_LOG_LEVEL=LOG_LEVEL_DEBUG -DDEBUG
  TESTFLAGS += -DDEBUG
else ifeq ($(DEBUG),2)
  CFLAGS += ${DEBUGGING_FLAGS} -DINIT_LOG_LEVEL=LOG_LEVEL_DEBUG -DNDEBUG
else
  CFLAGS += ${RELEASE_FLAGS}
endif

pkgname := mpxmanager

all: mpxmanager

install: mpxmanager.1 libmpxmanager.a libmpx-patch.so
	install -m 0744 -Dt "$(DESTDIR)/usr/lib/" libmpxmanager.a
	install -m 0744 -Dt "$(DESTDIR)/usr/lib/" libmpx-patch.so
	install -m 0744 -Dt "$(DESTDIR)/usr/include/$(pkgname)/" *.h
	install -m 0744 -Dt "$(DESTDIR)/usr/include/$(pkgname)/Extensions" Extensions/*.h
	install -m 0744 -Dt "$(DESTDIR)/usr/share/$(pkgname)/" sample-config.c
	install -m 0744 -Dt "$(DESTDIR)/usr/share/man/man1/" mpxmanager.1
	install -D -m 0755 "mpxmanager-autocomplete.sh" "$(DESTDIR)/etc/bash_completion.d/mpxmanager-autocomplete"
	install -D -m 0755 "mpxmanager.sh" "$(DESTDIR)/usr/bin/$(pkgname)"

preinstall: mpxmanager.1 mpxmanager-autocomplete.sh
	sed -i "s/pkgver=.*/pkgver='$$(./mpxmanager.sh -v)'/g" ../PKGBUILD

mpxmanager.1: mpxmanager.sh
	help2man -No mpxmanager.1 ./mpxmanager.sh

mpxmanager-autocomplete.sh: mpxmanager.c mpxmanager.sh
	sed -i "s/sendOptions=.*/sendOptions=\"$$(echo $$(./mpxmanager --list-options 2>&1 |grep -v ' '|cut -d'=' -f1 |xargs -I {} echo {} | awk '{print tolower($$0)}'))\"/g"  mpxmanager-autocomplete.sh

mpxmanager: ${BASE_SRCS}
	${CC} $^ -o $@ $$HOME/.mpxmanager/config.c ${CFLAGS} ${LDFLAGS}

recompile:
	${CC} $$HOME/.mpxmanager/config.c -o temp-Linux -lmpxmanager   ${CFLAGS} ${LDFLAGS}

libmpxmanager.so: ${BASE_SRCS}
	${CC} -fPIC -shared -o libmpxmanager.so $^ ${CFLAGS} ${LDFLAGS}

libmpxmanager.a: ${BASE_SRCS}
	${CC} $^ -c -fpic ${CFLAGS} ${LDFLAGS}
	ar rcs libmpxmanager.a *.o
	rm *.o


libmpx-patch.so: Hacks/mpx-patch.c
	${CC} -fPIC -shared -o $@ $^ ${CFLAGS} ${LDFLAGS}

config.c:
	cp --no-clobber sample-config.c config.c

test: unitTest1 unitTest2 unitTest3 unitTest4 unitTest5 preinstall format
		

unitTest1: Tests/Layer1Tests/*.c ${LAYER1_SRCS}
	${CC} $^ -o $@  ${TESTFLAGS} ${TESTLIBS} ${LDFLAGS}
	$(call RUN_TEST, ./$@ )
	$(call CHECK_TEST_COVERAGE,${LAYER1_SRCS})

unitTest2: Tests/*.c Tests/Layer2Tests/*.c ${LAYER2_SRCS} ${LAYER1_SRCS}
	${CC} ${LAYER0_SRCS} $^ -o $@  ${TESTFLAGS} ${TESTLIBS} ${LDFLAGS}
	$(call RUN_XTEST_NO_DISPLAY, ./$@)
	$(call RUN_XTEST, ./$@ )
	$(call CHECK_TEST_COVERAGE,${LAYER2_SRCS})

unitTest3: Tests/*.c Tests/Layer3Tests/*.c ${LAYER3_SRCS} ${LAYER2_SRCS} ${LAYER1_SRCS} ${LAYER0_SRCS}
	${CC} $^ -o $@ ${TESTFLAGS} ${TESTLIBS} ${LDFLAGS}
	$(call RUN_XTEST, ./$@ )
	$(call CHECK_TEST_COVERAGE,${LAYER3_SRCS})

unitTest4: Tests/*.c Tests/Layer4Tests/*.c ${LAYER4_SRCS} ${LAYER3_SRCS} ${LAYER2_SRCS} ${LAYER1_SRCS} ${LAYER0_SRCS}
	${CC} $^ -o $@ ${TESTFLAGS} ${TESTLIBS} ${LDFLAGS}
	$(call RUN_XTEST, ./$@ )
	$(call CHECK_TEST_COVERAGE, ${LAYER4_SRCS})

unitTest5: QUICK=1
unitTest5: mpxmanager Tests/arg-tests.sh
	$(call RUN_XTEST, Tests/arg-tests.sh )
	$(call CHECK_TEST_COVERAGE,${TOP_LAYER_SRCS})


FORMATTER := t=$$(stat -c %y "$$file"); hash="$$(md5sum $$file)";astyle --project "$$file" >/dev/null && sed -ri "s/\)\s+\{/){/g" "$$file"; [ "$$(md5sum $$file)" == "$$hash" ] && (touch -d "$$t" "$$file" && rm -f "$$file.orig") || true

format:
	cd ..;while read file; do \
		$(FORMATTER);\
	done < <(git diff --name-only|grep -E '\.(c|h)$$')

format-all:
	find . \( -name '*.c' -o -name '*.h' \) -exec bash -c 'file="$$0"; $(FORMATTER);' {} \;


package:
	+$(MAKE) -C .. $@
doc:
	+$(MAKE) -C .. $@
doc-check:
	+$(MAKE) -C .. $@

mpxmanager-profile: Tests/prof/profile.c Tests/*.c ${PROFILE_LAYERS}
	${CC} $^ -o $@  ${CFLAGS} ${PROFILE_FLAGS} ${LDFLAGS}
mpxmanager-speed-test: Tests/prof/profile.c Tests/*.c ${PROFILE_LAYERS}
	${CC} $^ -o $@  ${CFLAGS} ${SPEED_TEST_FLAGS} ${LDFLAGS}

benchmark.txt: mpxmanager-profile

speed-test-save:
	cp benchmark.txt benchmark-ref.txt
speed-test: QUICK=1
speed-test: mpxmanager-speed-test
	$(call RUN_XTEST, ./$^ 10) > benchmark.txt
	$(call RUN_XTEST, ./$^ 100) >> benchmark.txt
	$(call RUN_XTEST, ./$^ 1000) >> benchmark.txt
	cat benchmark.txt | Tests/prof/diff.py | sort

prof: QUICK=1
prof: mpxmanager-profile
	$(call RUN_XTEST, ./$^ 256 )
	gprof -p ./$^

.PHONY: test all clean doc format format-all install package unitTest5 prof

.DELETE_ON_ERROR:

clean:
	rm -f mywm debug unitTest* *gc?? vgcore* libmpxmanager*.a *.o libmpxmanager*.so mpxmanager-profile mpxmanager-speed-test
	find . -name '*.orig' -exec rm {} \;
