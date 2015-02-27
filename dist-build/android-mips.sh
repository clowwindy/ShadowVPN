#!/bin/sh
export CFLAGS="-Os"
TARCH=mips TARGET_ARCH=mips HOST_COMPILER=mipsel-linux-android "$(dirname "$0")/android-build.sh"
