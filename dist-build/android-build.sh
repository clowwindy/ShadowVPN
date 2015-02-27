#!/bin/sh
# this script is based on build script of
# libsodium <https://github.com/jedisct1/libsodium>

if [ -z "$ANDROID_NDK_HOME" ]; then
    echo "You should probably set ANDROID_NDK_HOME to the directory containing"
    echo "the Android NDK"
    exit
fi

if [ ! -f ./configure ]; then
	echo "Can't find ./configure. Wrong directory or haven't run autogen.sh?"
	exit 1
fi

if [ "x$TARGET_ARCH" = 'x' ] || [ "x$HOST_COMPILER" = 'x' ]; then
    echo "You shouldn't use android-build.sh directly, use android-[arch].sh instead"
    exit 1
fi

export MAKE_TOOLCHAIN="${ANDROID_NDK_HOME}/build/tools/make-standalone-toolchain.sh"

export PREFIX="$(pwd)/shadowvpn-android-${TARGET_ARCH}"
export TOOLCHAIN_DIR="$(pwd)/android-toolchain-${TARGET_ARCH}"
export PATH="${PATH}:${TOOLCHAIN_DIR}/bin"

# Clean up before build
rm -rf "${TOOLCHAIN_DIR}" "${PREFIX}"

$MAKE_TOOLCHAIN --platform="${NDK_PLATFORM:-android-14}" \
                --arch="$TARCH" \
                --install-dir="$TOOLCHAIN_DIR" && \
./configure --host="${HOST_COMPILER}" \
            --with-sysroot="${TOOLCHAIN_DIR}/sysroot" \
            --prefix="${PREFIX}" \
            --disable-soname-versions && \
make clean && \
make -j3 install && \
echo "shadowvpn has been installed into $PREFIX"
