SHELL := /bin/sh
DEBUGGING_FLAGS := -g -rdynamic -O0
RELEASE_FLAGS ?= -O2
SPEED_TEST_FLAGS := -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable

IGNORED_FLAGS := -Wno-parentheses -Wno-missing-field-initializers -Wno-sign-compare
ERROR_FLAGS := -Wall -Werror
CFLAGS := -std=c99 ${ERROR_FLAGS} ${IGNORED_FLAGS} ${INJECT} -D_DEFAULT_SOURCE

TESTFLAGS := ${CFLAGS} ${DEBUGGING_FLAGS}
LDFLAGS :=  -lxcb -lxcb-keysyms -lxcb-xinput -lxcb-xtest -lxcb-ewmh -lxcb-icccm -lxcb-randr -lm

LAYER0_SRCS :=  globals.c util/string-array.c util/logger.c util/debug.c
LAYER0_SRCS += xutil/xdebug.c xutil/test-functions.c xutil/properties.c xutil/window-properties.c xutil/xsession.c xutil/device-grab.c xutil/xerrors.c
LAYER1_SRCS := util/arraylist.c boundfunction.c
LAYER2_SRCS := slaves.c masters.c workspaces.c windows.c monitors.c
LAYER3_SRCS := system.c xevent.c devices.c bindings.c wmfunctions.c layouts.c
LAYER4_SRCS := wm-rules.c
LAYER5_SRCS := functions.c communications.c settings.c mpxmanager.c
LAYER6_SRCS := $(wildcard Extensions/*.c)
LAYER7_SRCS := $(wildcard Hacks/*.c)

TEST_SRCS := ${LAYER1_SRCS} ${LAYER11_SRCS} ${LAYER2_SRCS} ${LAYER3_SRCS} ${LAYER4_SRCS} ${LAYER5_SRCS}
TOP_LAYER_SRCS := settings.c mpxmanager.c
BASE_SRCS := ${TOP_LAYER_SRCS} ${TEST_SRCS} ${LAYER0_SRCS} ${LAYER6_SRCS}
SRCS := ${BASE_SRCS} config.c

MEM_CHECK = valgrind -q  --error-exitcode=123
RUN_TEST = ./run_in_x_container.sh $(if $(QUICK),  $(MEM_CHECK) $(1) 20,$(1))

NO_XRANDR ?= 0
ifeq ($(NO_XRANDR),1)
	CPPFLAGS += -DNO_XRANDR=1
endif

ifeq ($(QUICK),1)
	MEM_CHECK = valgrind -q  --error-exitcode=123
else ifeq ($(QUICK),2)
	MEM_CHECK = valgrind -q --leak-check=full --errors-for-leak-kinds=definite --show-leak-kinds=definite --error-exitcode=123
else ifeq ($(QUICK),3)
	MEM_CHECK = gdb
endif

ifeq ($(DEBUG),1)
  CFLAGS += ${DEBUGGING_FLAGS}
else ifeq ($(DEBUG),2)
  CFLAGS += ${DEBUGGING_FLAGS} -DNDEBUG
else ifeq ($(DEBUG),3)
  CFLAGS += ${RELEASE_FLAGS} -O2
else ifeq ($(DEBUG),4)
  CFLAGS += ${TESTFLAGS}
else
  CFLAGS += ${RELEASE_FLAGS} -DNDEBUG
endif


pkgname := mpxmanager

all: mpxmanager.sh mpxmanager-autocomplete.sh mpxmanager.1 libmpxmanager.a libmpx-patch.so

install-headers:
	install -m 0744 -Dt "$(DESTDIR)/usr/include/$(pkgname)/" *.h
	install -m 0744 -Dt "$(DESTDIR)/usr/include/$(pkgname)/Extensions" Extensions/*.h
	install -m 0744 -Dt "$(DESTDIR)/usr/include/$(pkgname)/util" util/*.h
	install -m 0744 -Dt "$(DESTDIR)/usr/include/$(pkgname)/xutil" xutil/*.h

install: all install-headers
	install -m 0744 -Dt "$(DESTDIR)/usr/lib/" libmpxmanager.a
	install -m 0744 -Dt "$(DESTDIR)/usr/lib/" libmpx-patch.so
	install -m 0744 -Dt "$(DESTDIR)/usr/share/$(pkgname)/" sample-config.c
	install -m 0744 -Dt "$(DESTDIR)/usr/share/man/man1/" mpxmanager.1
	install -D -m 0755 "mpxmanager-autocomplete.sh" "$(DESTDIR)/usr/share/bash-completion/completions/mpxmanager"
	install -D -m 0755 "mpxmanager.sh" "$(DESTDIR)/usr/bin/$(pkgname)"
uninstall:
	rm -f "$(DESTDIR)/usr/lib/libmpxmanager.a"
	rm -f "$(DESTDIR)/usr/lib/libmpx-patch.so"
	rm -frd "$(DESTDIR)/usr/include/$(pkgname)"
	rm -f "$(DESTDIR)/usr/share/$(pkgname)/sample-config.c"
	rm -f "$(DESTDIR)/usr/share/man/man1/mpxmanager.1"
	rm -f "$(DESTDIR)/etc/bash_completion.d/mpxmanager-autocomplete"
	rm -f "$(DESTDIR)/usr/bin/$(pkgname)"

preinstall: mpxmanager.1 mpxmanager-autocomplete.sh
	sed -i "s/pkgver=.*/pkgver='$$(./mpxmanager.sh -v)'/g" ../PKGBUILD

mpxmanager.sh: mpxmanager.sh.template | mpxmanager
	cp $< $@
	sed -i "s/LDFLAGS:-__place__holder/LDFLAGS:-'${LDFLAGS}'/g" mpxmanager.sh
	sed -i "s/GIT_HASH=.*/GIT_HASH=$$(git rev-parse --short HEAD)/g" mpxmanager.sh
	sed -i "s/VERSION=.*/VERSION=$$(./mpxmanager --version)/g" mpxmanager.sh

mpxmanager-autocomplete.sh: mpxmanager-autocomplete.sh.template mpxmanager.c communications.c | mpxmanager
	cp $< $@
	sed -i "s/startOptions=.*/startOptions=\"$$(echo $$(./mpxmanager --list-start-options 2>&1 | sed 's/[,}{]//g' | awk '{print tolower($$0)}'| sed -E 's/((\w|-)+\W)/--\1/g'))\"/g"  mpxmanager-autocomplete.sh
	sed -i "s/sendOptions=.*/sendOptions=\"$$(echo $$(./mpxmanager --list-options 2>&1 | sed 's/[,}{]//g' ))\"/g"  mpxmanager-autocomplete.sh

mpxmanager: $(BASE_SRCS:.c=.o) Tests/test-config.o
	${CC} ${CFLAGS} $^ -o $@ ${LDFLAGS}

libmpxmanager.a: $(BASE_SRCS:.c=.o)
	ar rcs libmpxmanager.a $^

libmpx-patch.so: Hacks/mpx-patch.c
	${CC} ${CFLAGS} -fPIC -shared -o $@ $^ ${LDFLAGS}

test: CFLAGS := ${TESTFLAGS}
test: unitTest.out functional.out unitTestExtensions.out endToEndTest.out unitTestHacks.out # doc-check

functionalTest: CFLAGS := ${TESTFLAGS}
functionalTest: $(BASE_SRCS:.c=.o) Tests/tester.o Tests/functional_test.o |libmpx-patch.so
	${CC} ${CFLAGS} $^ -o $@  ${TESTFLAGS} ${TESTLIBS} ${LDFLAGS}

functional.out: functionalTest
	LD_PRELOAD=$$(pwd)/libmpx-patch.so $(call RUN_TEST, ./$^ )
	$(if $(TEST_FILE)$(TEST_FUNC),exit 1)
	touch $@

endToEndTest: CFLAGS := ${TESTFLAGS}
endToEndTest: $(BASE_SRCS:.c=.o) Tests/tester.o Tests/endToEnd_test.o |libmpx-patch.so
	${CC} ${CFLAGS} $^ -o $@  ${TESTFLAGS} ${TESTLIBS} ${LDFLAGS}

endToEndTest.out: CFLAGS := ${TESTFLAGS}
endToEndTest.out: endToEndTest mpxmanager
	$(call RUN_TEST, ./$< )
	$(if $(TEST_FILE)$(TEST_FUNC),exit 1)
	touch $@

unitTest: CFLAGS := ${TESTFLAGS}
unitTest: Tests/tester.o   $(patsubst %.c,Tests/%_unit.o,$(TEST_SRCS)) $(TEST_SRCS:.c=.o) $(LAYER0_SRCS:.c=.o)
	${CC} ${CFLAGS} $^ -o $@  ${TESTFLAGS} ${TESTLIBS} ${LDFLAGS}

unitTest.out: unitTest
	$(call RUN_TEST, ./unitTest )
	$(if $(TEST_FUNC),exit 1)
	touch $@

unitTestExtensions: CFLAGS := ${TESTFLAGS}
unitTestExtensions: Tests/tester.o $(patsubst %.c,Tests/%_unit.o,$(LAYER6_SRCS)) $(LAYER6_SRCS) $(TEST_SRCS:.c=.o) $(LAYER0_SRCS:.c=.o)
	${CC} ${CFLAGS} $^ -o $@  ${TESTFLAGS} ${TESTLIBS} ${LDFLAGS}

unitTestExtensions.out: unitTestExtensions
	$(call RUN_TEST, ./$^)
	$(if $(TEST_FUNC),exit 1)
	touch $@

unitTestHacks: CFLAGS := ${TESTFLAGS}
unitTestHacks: Tests/tester.o $(patsubst %.c,Tests/%_unit.o,$(LAYER7_SRCS)) $(LAYER6_SRCS) $(TEST_SRCS:.c=.o) $(LAYER0_SRCS:.c=.o)
	${CC} ${CFLAGS} $^ -o $@  ${TESTFLAGS} ${TESTLIBS} ${LDFLAGS}

unitTestHacks.out: unitTestHacks | libmpx-patch.so
	LD_PRELOAD=libmpx-patch.so $(call RUN_TEST, ./$^)
	$(if $(TEST_FUNC),exit 1)
	touch $@


code_coverage.out: unitTest.out
	gcov -mr *
	grep "#####:" *c.gcov > $@
	grep "#####:" *c.gcov |wc -l

package:
	+$(MAKE) -j1 -C .. $@
doc:
	+$(MAKE) -j1 -C .. $@
doc-check:
	+$(MAKE) -j1 -C .. $@


.PHONY: test all clean doc install package

.DELETE_ON_ERROR:

clean-test:
	find . \( -name "*.out" \) -exec rm -f {} \;
clean:
	rm -f unitTest vgcore* *gc?? mpxmanager *.a *.so mpxmanager-autocomplete.sh mpxmanager.sh
	find . \( -name "*.orig" -o -name "*.gc??" -o -name "*.out" -o -name "*.o" \) -exec rm -f {} \;
