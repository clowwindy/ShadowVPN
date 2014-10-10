#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([shadowvpn],[0.1.3],[clowwindy42@gmail.com])
AC_CONFIG_AUX_DIR([.])

AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11 dist-bzip2 tar-ustar foreign subdir-objects])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB
AC_PROG_INSTALL
LT_INIT

# Checks for header files.
AC_HEADER_RESOLV
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h stdlib.h string.h sys/socket.h unistd.h])

# from OpenVPN configure.ac
case "$host" in
    *-*-linux*)
        AC_DEFINE([TARGET_LINUX], [1], [Are we running on Linux?])
        AC_CHECK_HEADER([linux/if_tun.h],[],[AC_MSG_ERROR([linux/if_tun.h not found.])],[])
        ;;
    *-*-darwin*)
        AC_DEFINE([TARGET_DARWIN], [1], [Are we running on Mac OS X?])
        AC_CHECK_HEADER([net/if_utun.h],[],[AC_MSG_ERROR([Mac OS X net/if_utun.h not found.])],[])
        ;;
    *-*-freebsd*)
        AC_DEFINE([TARGET_FREEBSD], [1], [Are we running on FreeBSD?])
        AC_CHECK_HEADER([net/if_tun.h],[],[AC_MSG_ERROR([FreeBSD net/if_tun.h not found.])],[])
        ;;
    *-mingw*)
        AC_DEFINE([TARGET_WIN32], [1], [Are we running on Windows?])
        WIN32=yes
        LIBS="${LIBS} -lgdi32 -lws2_32"
        AC_CHECK_HEADERS([windows.h winsock2.h ws2tcpip.h],[],[AC_MSG_ERROR([Windows headers not found.])],[])
        ;;
    *)
        AC_DEFINE([TARGET_UNKNOWN], [1], [Unknown platform ?])
esac

AM_CONDITIONAL([WIN32], [test "${WIN32}" = "yes"])
AC_CHECK_HEADER([android/log.h],[CFLAGS="$CFLAGS -DHAVE_ANDROID_LOG"; LDFLAGS="$LDFLAGS -llog"],[],[])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T

# Checks for library functions.
# To fix rpl_malloc undefined error in mips cross-compile enviroment.
AC_CHECK_FUNCS([malloc realloc])
AC_CHECK_FUNCS([inet_ntoa memset select socket strchr strdup strrchr])

AC_ARG_ENABLE([debug],
    [  --enable-debug          build with additional debugging code],
    [CFLAGS="$CFLAGS -g -DDEBUG"])

AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")

# Add a option to force static link the target.
AC_ARG_ENABLE([static],
    [  --enable-static         build with static linking],
    [LDFLAGS="$LDFLAGS -static"])

AC_ARG_ENABLE([profile],
    [  --enable-profile        build with profile],
    [CFLAGS="$CFLAGS -pg"])

AC_ARG_ENABLE([analyze],
    [  --enable-analyze        build with analyze],
    [CC="clang --analyze"])

AM_CONDITIONAL(STATIC, test x"$static" = x"true")

AC_CONFIG_FILES([Makefile src/Makefile samples/Makefile])
AC_CONFIG_SUBDIRS([libsodium])
AC_OUTPUT
