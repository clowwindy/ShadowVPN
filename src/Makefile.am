bin_PROGRAMS = shadowvpn

SUBDIRS = ../libsodium
AM_CPPFLAGS = -I$(srcdir)/../libsodium/src/libsodium/include
shadowvpn_SOURCES = log.c crypto_secretbox_salsa208poly1305.c crypto.c args.c main.c
shadowvpn_LDADD = ../libsodium/src/libsodium/libsodium.la
