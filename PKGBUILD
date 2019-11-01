# Maintainer: Arthur Williams <taaparthur@gmail.com>


pkgname='mpxmanager'
pkgver='1.0.0'
_language='en-US'
pkgrel=1
pkgdesc='My Personal XWindow Manager'

arch=('any')
license=('MIT')
options=(staticlibs !strip)
depends=('xorg-server' 'libx11' 'libxcb' 'xcb-util-wm' )
optdepends=('xorg-server-xvfb' 'xorg-xinput' 'xsane-xrandr' 'check')
makedepends=('git' 'help2man')
md5sums=('SKIP')

source=("git+https://github.com/TAAPArthur/MPXManager.git")
_srcDir="MPXManager"

package() {
  cd "$_srcDir"
  make DESTDIR=$pkgdir install
}
