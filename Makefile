MAKE_SRC := +$(MAKE) -C src $@
%:
	+$(MAKE) -C src $@

doc:
	doxygen dconfig 
doc-check:
	 make doc 2>&1 |grep -i "Warning"
package:
	sed -i "s/pkgver=.*\$$/pkgver='$$(src/mpxmanager.sh --version)'/g" PKGBUILD

.PHONY: doc package

