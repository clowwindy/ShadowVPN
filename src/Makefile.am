bin_PROGRAMS = shadowvpn crypto-perf

SUBDIRS = ../libsodium
AM_CPPFLAGS = -I$(srcdir)/../libsodium/src/libsodium/include

shadowvpn_SOURCES = log.c crypto_secretbox_salsa208poly1305.c crypto.c shell.c vpn.c args.c daemon.c main.c
shadowvpn_LDADD = ../libsodium/src/libsodium/libsodium.la

crypto_perf_SOURCES = crypto_perf.c crypto.c crypto_secretbox_salsa208poly1305.c
crypto_perf_LDADD = ../libsodium/src/libsodium/libsodium.la
