bin_PROGRAMS = shadowvpn

SUBDIRS = ../libsodium

AM_CFLAGS = -I$(srcdir)/../libsodium/src/libsodium/include

shadowvpn_SOURCES = log.c \
										log.h \
										crypto_secretbox_salsa208poly1305.h \
										crypto_secretbox_salsa208poly1305.c \
										crypto.h \
										crypto.c \
										shell.h \
										shell.c \
										vpn.h \
										vpn.c \
										args.h \
										args.c \
										daemon.h \
										daemon.c \
										shadowvpn.h \
										main.c
shadowvpn_LDADD = ../libsodium/src/libsodium/libsodium.la

