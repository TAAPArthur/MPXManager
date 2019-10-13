CC := g++ -std=gnu++17 -lstdc++
CXX := ${CC}
SHELL := /bin/bash
DEBUGGING_FLAGS := -g -rdynamic -O0
RELEASE_FLAGS := -g -O3 -flto -DNDEBUG
PROFILE_FLAGS := -g -rdynamic -pg  -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable
SPEED_TEST_FLAGS := -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable

IGNORED_FLAGS := -Wno-parentheses -Wno-missing-field-initializers
ERROR_FLAGS := -Werror -Wall -Wextra
CPPFLAGS := ${ERROR_FLAGS} ${IGNORED_FLAGS} ${INJECT}

TESTFLAGS := ${CPPFLAGS} -Wno-sign-compare ${DEBUGGING_FLAGS} --coverage
NON_X_LDFLAGS := -lm -lpthread
LDFLAGS :=  ${NON_X_LDFLAGS} -lm -lpthread -lX11 -lXi -lxcb -lxcb-xinput -lxcb-xtest -lxcb-ewmh -lxcb-icccm -lxcb-randr -lX11-xcb

LAYER0_SRCS := globals.cpp logger.cpp debug.cpp
LAYER1_SRCS := arraylist.h ext.cpp boundfunction.cpp rect.cpp slaves.cpp masters.cpp workspaces.cpp windows.cpp monitors.cpp system.cpp
LAYER2_SRCS := xsession.cpp bindings.cpp devices.cpp test-functions.cpp device-grab.cpp
LAYER3_SRCS := window-properties.cpp wmfunctions.cpp ewmh.cpp layouts.cpp state.cpp wm-rules.cpp extra-rules.cpp
LAYER4_SRCS := chain.cpp functions.cpp communications.cpp
LAYER5_SRCS := $(wildcard Extensions/*.cpp)
TEST_SRCS := ${LAYER1_SRCS} ${LAYER2_SRCS} ${LAYER3_SRCS} ${LAYER4_SRCS} ${LAYER5_SRCS}
PROFILE_LAYERS := settings.cpp ${TEST_SRCS}
TOP_LAYER_SRCS := settings.cpp mpxmanager.cpp
BASE_SRCS := ${TOP_LAYER_SRCS} ${TEST_SRCS} ${LAYER0_SRCS}
SRCS := ${BASE_SRCS} config.cpp

CHECK_TEST_COVERAGE = (gcov -mr $(1) 2> /dev/null  | grep -v '100' |grep -B 1 "Lines e" |grep -A1 ".cpp" && echo "test coverage is not high enough" && cat $$(basename -a $(1)).gcov |grep -C5 "\#\#\#\#")
RUN_TEST = LD_PRELOAD=$$(pwd)/libmpx-test.so xvfb-run -w 1 -a $(if $(QUICK) , valgrind -q --leak-check=full --error-exitcode=123 $(1),$(1))

NO_XRANDR ?= 0
ifeq ($(NO_XRANDR),1)
	CPPFLAGS += -DNO_XRANDR=1
endif

ifeq ($(DEBUG),1)
  CPPFLAGS += ${DEBUGGING_FLAGS} -DINIT_LOG_LEVEL=LOG_LEVEL_DEBUG -DDEBUG
else ifeq ($(DEBUG),2)
  CPPFLAGS += ${DEBUGGING_FLAGS} -DINIT_LOG_LEVEL=LOG_LEVEL_DEBUG -DNDEBUG
else
  CPPFLAGS += ${RELEASE_FLAGS}
endif


pkgname := mpxmanager

all: mpxmanager

install: mpxmanager.sh mpxmanager-autocomplete.sh mpxmanager.1 libmpxmanager.a libmpx-patch.so
	install -m 0744 -Dt "$(DESTDIR)/usr/lib/" libmpxmanager.a
	install -m 0744 -Dt "$(DESTDIR)/usr/lib/" libmpx-patch.so
	install -m 0744 -Dt "$(DESTDIR)/usr/include/$(pkgname)/" *.h
	install -m 0744 -Dt "$(DESTDIR)/usr/include/$(pkgname)/Extensions" Extensions/*.h
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

mpxmanager-autocomplete.sh: mpxmanager-autocomplete.sh.template mpxmanager.cpp communications.cpp
	cp $< $@
	sed -i "s/startOptions=.*/startOptions=\"$$(echo $$(./mpxmanager --list-start-options 2>&1 | sed 's/[,}{]//g' | awk '{print tolower($$0)}'| sed -E 's/((\w|-)+\W)/--\1/g'))\"/g"  mpxmanager-autocomplete.sh
	sed -i "s/sendOptions=.*/sendOptions=\"$$(echo $$(./mpxmanager --set list-options quit 2>&1 | awk '{print tolower($$0)}'))\"/g"  mpxmanager-autocomplete.sh
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

config.cpp:
	cp --no-clobber sample-config.cpp config.cpp

test: libmpx-test.so correctness.out arg_test.out functional.out preinstall code_coverage.out format

T1 = $(patsubst %.cpp,Tests/%_unit.o, $(1))
TEST_FILES = $(patsubst %.h,Tests/%_unit.o, $(call T1, $(1)))

mpxmanager-debug: $(SRCS:.cpp=.o)
	${CC} $^ -o $@  ${TESTFLAGS} ${TESTLIBS} ${LDFLAGS}

functionalTest: CPPFLAGS := ${TESTFLAGS}
functionalTest: Tests/tester.o Tests/functional_test.cpp $(BASE_SRCS:.cpp=.o)
	${CC} $^ -o $@  ${TESTFLAGS} ${TESTLIBS} ${LDFLAGS}

functional.out: functionalTest
	$(call RUN_TEST, ./$^ )
	$(if $(TEST_FILE)$(TEST_FUNC),exit 1)
	touch $@

unitTest: CPPFLAGS := ${TESTFLAGS}
unitTest: Tests/tester.o $(call TEST_FILES, ${TEST_SRCS}) $(TEST_SRCS:.cpp=.o) $(LAYER0_SRCS:.cpp=.o)
	${CC} $^ -o $@  ${TESTFLAGS} ${TESTLIBS} ${LDFLAGS}

correctness.out: unitTest
	$(call RUN_TEST, ./unitTest )
	touch $@
	$(if $(TEST_FILE)$(TEST_FUNC),exit 1, exit 0)

arraylist.h_code_coverage.out: arraylist.h
	touch $@

%_code_coverage.out: % | unitTest
	rm -f $@ *gcda
	TEST_FILE=$(patsubst %.o,%.cpp, $(call TEST_FILES, $<)) $(call RUN_TEST, ./unitTest ) && ($(call CHECK_TEST_COVERAGE,$<) 2>&1 | tee -a $@)
	[ -s  $$(basename -a $<).gcov ]
	[ ! -s  $@ ]


code_coverage.out: unitTest $(patsubst %,%_code_coverage.out,${TEST_SRCS})


arg_test.out: CPPFLAGS := ${TESTFLAGS}
arg_test.out: mpxmanager-debug Tests/arg-tests.sh
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
	+$(MAKE) -C .. $@
doc:
	+$(MAKE) -C .. $@
doc-check:
	+$(MAKE) -C .. $@

mpxmanager-profile: Tests/prof/profile.c Tests/*.c ${PROFILE_LAYERS}
	${CC} $^ -o $@  ${CPPFLAGS} ${PROFILE_FLAGS} ${LDFLAGS}
mpxmanager-speed-test: Tests/prof/profile.c Tests/*.c ${PROFILE_LAYERS}
	${CC} $^ -o $@  ${CPPFLAGS} ${SPEED_TEST_FLAGS} ${LDFLAGS}

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
	find . -name '*.o' -exec rm -f {} \;
	find . -name '*.out' -exec rm {} \;
