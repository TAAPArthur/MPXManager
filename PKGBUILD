# Maintainer: Arthur Williams <taaparthur@gmail.com>


pkgname='mpxmanager'
pkgver='0.9.0'
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
  mkdir -p "$pkgdir/usr/bin/"
  mkdir -p "$pkgdir/usr/lib/$pkgname/"
  make
  install -D -m 0755 "mpxmanager" "$pkgdir/usr/bin/"
}
