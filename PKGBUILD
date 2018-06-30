# Maintainer: Arthur Williams <taaparthur@gmail.com>


pkgname='cycle-windows'
pkgver='0.5.2'
_language='en-US'
pkgrel=1
pkgdesc='Control mouse from keyboard'

arch=('any')
license=('MIT')
depends=('xorg-server' 'libx11' 'xdotool')
md5sums=('SKIP')

source=("git://github.com/TAAPArthur/CycleWindows.git")
_srcDir="CycleWindows"

package() {

  cd "$_srcDir"
  mkdir -p "$pkgdir/usr/bin/"
  mkdir -p "$pkgdir/usr/lib/$pkgname/"
  install -D -m 0755 "cycle-windows" "$pkgdir/usr/bin/"
}
