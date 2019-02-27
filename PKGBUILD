# Maintainer: Arthur Williams <taaparthur@gmail.com>


pkgname='mpxmanager'
pkgver='0.9.2'
_language='en-US'
pkgrel=1
pkgdesc='My Personal XWindow Manager'

arch=('any')
license=('MIT')
depends=('xorg-server' 'libx11' 'libxcb')
md5sums=('SKIP')

source=("git://github.com/TAAPArthur/MyPersonalXWindowManager.git")
_srcDir="MyPersonalXWindowManager"

package() {
  cd "$_srcDir"
  make DESTDIR=$pkgdir install
}
