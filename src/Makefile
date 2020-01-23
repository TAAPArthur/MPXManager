CC := g++ -std=gnu++17 -lstdc++
CXX := ${CC}
SHELL := /bin/bash
DEBUGGING_FLAGS := -g -rdynamic -O0 -D_GLIBCXX_DEBUG
RELEASE_FLAGS := -g -rdynamic -O3
SPEED_TEST_FLAGS := -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable

IGNORED_FLAGS := -Wno-parentheses -Wno-missing-field-initializers -Wno-sign-compare
ERROR_FLAGS := -Werror -Wall -Wextra
CPPFLAGS := ${ERROR_FLAGS} ${IGNORED_FLAGS} ${INJECT}

TESTFLAGS := ${CPPFLAGS} ${DEBUGGING_FLAGS} --coverage
NON_X_LDFLAGS := -lm -pthread
LDFLAGS :=  ${NON_X_LDFLAGS} -lX11 -lXi -lxcb -lxcb-xinput -lxcb-xtest -lxcb-ewmh -lxcb-icccm -lxcb-randr -lX11-xcb

LAYER0_SRCS := globals.cpp logger.cpp debug.cpp
LAYER1_SRCS := ringbuffer.h arraylist.h ext.cpp boundfunction.cpp rect.cpp slaves.cpp masters.cpp workspaces.cpp windows.cpp monitors.cpp system.cpp
LAYER2_SRCS := xsession.cpp bindings.cpp devices.cpp test-functions.cpp device-grab.cpp
LAYER3_SRCS := window-properties.cpp wmfunctions.cpp ewmh.cpp layouts.cpp state.cpp wm-rules.cpp extra-rules.cpp
LAYER4_SRCS := chain.cpp functions.cpp communications.cpp

#ifndef NO_MPX
LAYER5_SRCS += Extensions/mpx.cpp
#endif
#ifndef NO_WINDOW_CLONE
LAYER5_SRCS += Extensions/window-clone.cpp
#endif
#ifndef NO_XMOUSE_CONTROL
LAYER5_SRCS += Extensions/xmousecontrol.cpp
#endif
#ifndef NO_SESSION
LAYER5_SRCS += Extensions/session.cpp
#endif

TEST_SRCS := ${LAYER1_SRCS} ${LAYER2_SRCS} ${LAYER3_SRCS} ${LAYER4_SRCS} ${LAYER5_SRCS}
TOP_LAYER_SRCS := settings.cpp mpxmanager.cpp
BASE_SRCS := ${TOP_LAYER_SRCS} ${TEST_SRCS} ${LAYER0_SRCS}
SRCS := ${BASE_SRCS} config.cpp

PRELOAD := $$(pwd)/libmpx-test.so
CHECK_TEST_COVERAGE = (gcov -mr $(2) $(1) 2> /dev/null  | grep -v '100.00%' |grep -B 1 "Lines e" |grep -A1 $(1) && echo "test coverage is not high enough" && cat $$(basename -a $(1)).gcov |grep -C5 "\#\#\#\#" && echo "$$((100 - $$(grep -c '\#\#\#\#' $$(basename -a $(1)).gcov)*100/$$(wc -l $$(basename -a $(1)).gcov | cut -d' ' -f1)))%")

MEM_CHECK = valgrind -q  --error-exitcode=123
RUN_TEST = LD_PRELOAD=$(PRELOAD) xvfb-run -w 0 -a $(if $(QUICK),  $(MEM_CHECK) $(1) 20,$(1))

NO_XRANDR ?= 0
ifeq ($(NO_XRANDR),1)
	CPPFLAGS += -DNO_XRANDR=1
endif

ifeq ($(QUICK),1)
	MEM_CHECK = valgrind -q  --error-exitcode=123
else ifeq ($(QUICK),2)
	MEM_CHECK = valgrind -q --leak-check=full --errors-for-leak-kinds=definite --show-leak-kinds=definite --error-exitcode=123
endif

ifeq ($(DEBUG),1)
  CPPFLAGS += ${DEBUGGING_FLAGS}
else ifeq ($(DEBUG),2)
  CPPFLAGS += ${DEBUGGING_FLAGS} -DNDEBUG
else ifeq ($(DEBUG),3)
  CPPFLAGS += ${RELEASE_FLAGS} -O2
else ifeq ($(DEBUG),4)
  CPPFLAGS += ${TESTFLAGS}
else
  CPPFLAGS += ${RELEASE_FLAGS} -DNDEBUG
endif


pkgname := mpxmanager

all: mpxmanager

install-headers:
	install -m 0744 -Dt "$(DESTDIR)/usr/include/$(pkgname)/" *.h
	install -m 0744 -Dt "$(DESTDIR)/usr/include/$(pkgname)/Extensions" Extensions/*.h

install: mpxmanager.sh mpxmanager-autocomplete.sh mpxmanager.1 libmpxmanager.a libmpx-patch.so install-headers
	install -m 0744 -Dt "$(DESTDIR)/usr/lib/" libmpxmanager.a
	install -m 0744 -Dt "$(DESTDIR)/usr/lib/" libmpx-patch.so
	install -m 0744 -Dt "$(DESTDIR)/usr/share/$(pkgname)/" sample-config.cpp
	install -m 0744 -Dt "$(DESTDIR)/usr/share/man/man1/" mpxmanager.1
	install -D -m 0755 "mpxmanager-autocomplete.sh" "$(DESTDIR)/etc/bash_completion.d/mpxmanager-autocomplete"
	install -D -m 0755 "mpxmanager.sh" "$(DESTDIR)/usr/bin/$(pkgname)"
uninstall:
	rm -f "$(DESTDIR)/usr/lib/libmpxmanager.a"
	rm -f "$(DESTDIR)/usr/lib/libmpx-patch.so"
	rm -frd "$(DESTDIR)/usr/include/$(pkgname)"
	rm -f "$(DESTDIR)/usr/share/$(pkgname)/sample-config.cpp"
	rm -f "$(DESTDIR)/usr/share/man/man1/mpxmanager.1"
	rm -f "$(DESTDIR)/etc/bash_completion.d/mpxmanager-autocomplete"
	rm -f "$(DESTDIR)/usr/bin/$(pkgname)"

preinstall: mpxmanager.1 mpxmanager-autocomplete.sh
	sed -i "s/pkgver=.*/pkgver='$$(./mpxmanager.sh -v)'/g" ../PKGBUILD

mpxmanager.1: mpxmanager.sh
	help2man -No mpxmanager.1 ./mpxmanager.sh

mpxmanager.sh: mpxmanager.sh.template
	cp $< $@
	sed -i "s/CC=.*/CC='${CC}'/g" mpxmanager.sh
	sed -i "s/LDFLAGS=.*/LDFLAGS='${LDFLAGS}'/g" mpxmanager.sh
	sed -i "s/CPPFLAGS=.*/CPPFLAGS='${CPPFLAGS}'/g" mpxmanager.sh
	sed -i "s/GIT_HASH=.*/GIT_HASH=$$(git rev-parse --short HEAD)/g" mpxmanager.sh

mpxmanager-autocomplete.sh: mpxmanager-autocomplete.sh.template mpxmanager.cpp communications.cpp | mpxmanager
	cp $< $@
	sed -i "s/startOptions=.*/startOptions=\"$$(echo $$(./mpxmanager --list-start-options 2>&1 | sed 's/[,}{]//g' | awk '{print tolower($$0)}'| sed -E 's/((\w|-)+\W)/--\1/g'))\"/g"  mpxmanager-autocomplete.sh
	sed -i "s/sendOptions=.*/sendOptions=\"$$(echo $$(./mpxmanager --set list-options quit 2>&1 | sed 's/[,}{]//g' | awk '{print tolower($$0)}'))\"/g"  mpxmanager-autocomplete.sh
	sed -i "s/modeOptions=.*/modeOptions=\"$$(echo $$(./mpxmanager --list-modes 2>&1 | sed 's/[,}{]//g' | awk '{print tolower($$0)}'))\"/g"  mpxmanager-autocomplete.sh

mpxmanager: $(BASE_SRCS:.cpp=.o)
	${CC} $^ -o $@ ${CPPFLAGS} ${LDFLAGS}


libmpxmanager.so: $(BASE_SRCS:.cpp=.o)
	${CC} -fPIC -shared -o libmpxmanager.so $^ ${CPPFLAGS} ${LDFLAGS}

libmpxmanager.a: $(BASE_SRCS:.cpp=.o)
	ar rcs libmpxmanager.a $^

libmpx-patch.so: Hacks/mpx-patch.c
	${CC} -fPIC -shared -o $@ $^ ${CPPFLAGS} ${LDFLAGS}

libmpx-test.so: Tests/test-preload.c
	${CC} -fPIC -shared $^ -o $@  ${TESTFLAGS} ${TESTLIBS} ${LDFLAGS}

test: libmpx-test.so code_coverage.out arg_test.out functional.out format doc-check correctness.out preinstall

functionalTest: CPPFLAGS := ${TESTFLAGS}
functionalTest: $(BASE_SRCS:.cpp=.o) Tests/tester.o Tests/functional_test.cpp Tests/compatibility_test.cpp |libmpx-patch.so
	${CC} $^ -o $@  ${TESTFLAGS} ${TESTLIBS} ${LDFLAGS}

functional.out: PRELOAD := $$(pwd)/libmpx-patch.so
functional.out: functionalTest
	$(call RUN_TEST, ./$^ )
	$(if $(TEST_FILE)$(TEST_FUNC),exit 1)
	touch $@

unitTest: CPPFLAGS := ${TESTFLAGS}
unitTest: Tests/tester.o   $(patsubst %.cpp,%.o,$(wildcard Tests/Extensions/*_unit.cpp Tests/*_unit.cpp)) $(TEST_SRCS:.cpp=.o) $(LAYER0_SRCS:.cpp=.o) | libmpx-test.so
	${CC} $^ -o $@  ${TESTFLAGS} ${TESTLIBS} ${LDFLAGS}

correctness.out: unitTest
	$(call RUN_TEST, ./unitTest )
	touch $@
	$(if $(TEST_FILE)$(TEST_FUNC),exit 1)

Extensions/Tests/%: Tests/Extensions/%
	@true

%_code_coverage.out: %.cpp Tests/%_unit.cpp | unitTest
	rm -f $@ *gcda
	TEST_FILE=Tests/$*_unit.cpp $(call RUN_TEST, ./unitTest ) && ($(call CHECK_TEST_COVERAGE,$<) 2>&1 | tee -a $@)
	[ -s  $$(basename -a $<).gcov ]
	[ ! -s  $@ ]

%_code_coverage.out: %.h Tests/%_unit.cpp  | unitTest
	rm -f $@ *gcda
	TEST_FILE=Tests/$*_unit.cpp $(call RUN_TEST, ./unitTest ) && ($(call CHECK_TEST_COVERAGE, $<, Tests/$*_unit.cpp) 2>&1 | tee -a $@)
	[ -s  $$(basename -a $<).gcov ]
	[ ! -s  $@ ]


code_coverage.out: unitTest $(patsubst %.h,%_code_coverage.out,${TEST_SRCS}) $(patsubst %.cpp,%_code_coverage.out,${TEST_SRCS})


arg_test.out: CPPFLAGS := ${TESTFLAGS}
arg_test.out: mpxmanager Tests/arg-tests.sh
	$(call RUN_TEST, Tests/arg-tests.sh )
	touch $@


FORMATTER := astyle --project "$$file" >/dev/null ; rm -f "$$file.orig" || true

format:
	cd ..;while read file; do \
		$(FORMATTER);\
	done < <(git diff --name-only|grep -E '\.(c|h|cpp)$$')

format-all:
	find . \( -name '*.cpp' -o -name '*.h' \) -exec bash -c 'file="$$0"; $(FORMATTER);' {} \;


package:
	+$(MAKE) -j1 -C .. $@
doc:
	+$(MAKE) -j1 -C .. $@
doc-check:
	+$(MAKE) -j1 -C .. $@


.PHONY: test all clean-test clean doc format format-all install package

.DELETE_ON_ERROR:

clean-test:
	find . \( -name "*.out" \) -exec rm -f {} \;
clean:
	rm -f unitTest vgcore* *gc?? mpxmanager libmpxmanager*.a libmpxmanager*.so mpxmanager-autocomplete.sh mpxmanager.sh
	find . \( -name "*.orig" -o -name "*.gc??" -o -name "*.out" -o -name "*.o" \) -exec rm -f {} \;
