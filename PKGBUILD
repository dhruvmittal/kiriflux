# Maintainer: Dhruv <gnuslashdhruv@codeberg.org>

pkgname=kiriflux-git
pkgver=0.1.2
pkgrel=1
pkgdesc="Desktop-first RSS reader for Miniflux built with KDE Kirigami"
arch=('x86_64')
url="https://codeberg.org/gnuslashdhruv/kiriflux"
license=('GPL-3.0-or-later')
depends=(
  'gcc-libs'
  'glibc'
  'kconfig'
  'kcoreaddons'
  'kdbusaddons'
  'ki18n'
  'kirigami'
  'qt6-base'
  'qt6-declarative'
  'qt6-webengine'
)
makedepends=(
  'cmake'
  'extra-cmake-modules'
  'git'
  'ninja'
)
optdepends=(
  'qqc2-desktop-style: native desktop styling for Kirigami'
)
provides=('kiriflux')
conflicts=('kiriflux')
source=("$pkgname::git+https://codeberg.org/gnuslashdhruv/kiriflux.git")
sha256sums=('SKIP')

pkgver() {
  cd "$pkgname"
  if git describe --long --tags >/dev/null 2>&1; then
    git describe --long --tags --abbrev=7 | sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g'
  else
    printf "0.1.2.r%s.g%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short=7 HEAD)"
  fi
}

build() {
  cmake -B build -S "$pkgname" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_TESTING=OFF
  cmake --build build
}

package() {
  DESTDIR="$pkgdir" cmake --install build
}
