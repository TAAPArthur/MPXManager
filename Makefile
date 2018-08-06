MAKE_SRC := +$(MAKE) -C src $@
all:
	$(MAKE_SRC)
%:
	$(MAKE_SRC)
doc:
	doxygen dconfig 

.PHONY: all clean doc

