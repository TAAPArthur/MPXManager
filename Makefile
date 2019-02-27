MAKE_SRC := +$(MAKE) -C src $@
%:
	+$(MAKE) -C src $@

doc:
	doxygen dconfig 
package:
	sed -i "s/pkgver=.*\$$/pkgver='$$(src/mpxmanager.sh --version)'/g" PKGBUILD

.PHONY: doc package

