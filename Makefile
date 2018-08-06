MAKE_SRC := +$(MAKE) -C src $@
%: all

all:
	+$(MAKE) -C src $@

doc:
	doxygen dconfig 

.PHONY: doc

