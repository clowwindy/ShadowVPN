bin_PROGRAMS = shadowvpn

lib_LTLIBRARIES = libshadowvpn.la

SUBDIRS = ../libsodium

AM_CFLAGS = -I$(srcdir)/../libsodium/src/libsodium/include

libshadowvpn_la_SOURCES = log.c \
	log.h \
	crypto_secretbox_salsa208poly1305.h \
	crypto_secretbox_salsa208poly1305.c \
	crypto.h \
	crypto.c \
	shell.h \
	shell.c \
	strategy.h \
	strategy.c \
	vpn.h \
	vpn.c \
	args.h \
	args.c \
	daemon.h \
	daemon.c \
	shadowvpn.h

if WIN32
libshadowvpn_la_SOURCES += win32.c win32.h
endif

libshadowvpn_la_LIBADD = ../libsodium/src/libsodium/libsodium.la

shadowvpn_SOURCES = main.c

shadowvpn_LDADD = libshadowvpn.la
