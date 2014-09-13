#ifndef CRYPTO_SECRETBOX_SALSA208POLY1305_H
#define CRYPTO_SECRETBOX_SALSA208POLY1305_H

int crypto_secretbox_salsa208poly1305(
  unsigned char *c,
  const unsigned char *m,unsigned long long mlen,
  const unsigned char *n,
  const unsigned char *k
);

int crypto_secretbox_salsa208poly1305_open(
  unsigned char *m,
  const unsigned char *c,unsigned long long clen,
  const unsigned char *n,
  const unsigned char *k
);

#endif
