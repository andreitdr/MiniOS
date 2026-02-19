set -e

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
PREFIX="$ROOT_DIR/tools/local"
BUILD_DIR="$ROOT_DIR/tools/build"
SRC_DIR="$ROOT_DIR/tools/src"
TARGET=i686-elf

BINUTILS_VER=2.41
GCC_VER=13.2.0
NASM_VER=2.16.01
MTOOLS_VER=4.0.43
DOSFSTOOLS_VER=4.2

sudo apt update
sudo apt install -y \
	build-essential bison flex libgmp-dev libmpc-dev libmpfr-dev libisl-dev \
	texinfo wget xz-utils

mkdir -p "$PREFIX" "$BUILD_DIR" "$SRC_DIR"
export PATH="$PREFIX/bin:$PATH"

fetch() {
	local url="$1"
	local out="$2"

	if [ ! -f "$out" ]; then
		wget -O "$out" "$url"
	fi
}

extract() {
	local archive="$1"
	local dest="$2"

	if [ ! -d "$dest" ]; then
		mkdir -p "$dest"
		tar -xf "$archive" -C "$dest" --strip-components=1
	fi
}

# binutils
fetch "https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VER.tar.xz" "$SRC_DIR/binutils.tar.xz"
extract "$SRC_DIR/binutils.tar.xz" "$SRC_DIR/binutils"
mkdir -p "$BUILD_DIR/binutils"
cd "$BUILD_DIR/binutils"
"$SRC_DIR/binutils/configure" --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make -j"$(nproc)"
make install

# gcc (C only)
fetch "https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VER/gcc-$GCC_VER.tar.xz" "$SRC_DIR/gcc.tar.xz"
extract "$SRC_DIR/gcc.tar.xz" "$SRC_DIR/gcc"
mkdir -p "$BUILD_DIR/gcc"
cd "$BUILD_DIR/gcc"
"$SRC_DIR/gcc/configure" --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c --without-headers
make -j"$(nproc)" all-gcc
make -j"$(nproc)" all-target-libgcc
make install-gcc
make install-target-libgcc

# nasm
fetch "https://www.nasm.us/pub/nasm/releasebuilds/$NASM_VER/nasm-$NASM_VER.tar.xz" "$SRC_DIR/nasm.tar.xz"
extract "$SRC_DIR/nasm.tar.xz" "$SRC_DIR/nasm"
cd "$SRC_DIR/nasm"
./configure --prefix="$PREFIX"
make -j"$(nproc)"
make install

# mtools (mcopy)
fetch "https://ftp.gnu.org/gnu/mtools/mtools-$MTOOLS_VER.tar.gz" "$SRC_DIR/mtools.tar.gz"
extract "$SRC_DIR/mtools.tar.gz" "$SRC_DIR/mtools"
cd "$SRC_DIR/mtools"
./configure --prefix="$PREFIX"
make -j"$(nproc)"
make install

# dosfstools (mkfs.fat)
fetch "https://github.com/dosfstools/dosfstools/releases/download/v$DOSFSTOOLS_VER/dosfstools-$DOSFSTOOLS_VER.tar.gz" "$SRC_DIR/dosfstools.tar.gz"
extract "$SRC_DIR/dosfstools.tar.gz" "$SRC_DIR/dosfstools"
cd "$SRC_DIR/dosfstools"
./configure --prefix="$PREFIX" --sbindir="$PREFIX/sbin"
make -j"$(nproc)"
make install PREFIX="$PREFIX" SBINDIR="$PREFIX/sbin"

echo "Installed toolchain to $PREFIX"
echo "Add to PATH: export PATH=\"$PREFIX/bin:$PREFIX/sbin:\$PATH\""