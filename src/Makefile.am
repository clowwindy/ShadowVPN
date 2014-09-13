bin_PROGRAMS = shadowvpn

SUBDIRS = ../libsodium
AM_CPPFLAGS = -I$(srcdir)/../libsodium/src/libsodium/include
shadowvpn_SOURCES = main.c
shadowvpn_LDADD = ../libsodium/src/libsodium/libsodium.la
