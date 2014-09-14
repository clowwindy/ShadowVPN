#ifndef CRYPTO_H
#define CRYPTO_H

int crypto_init();

int crypto_set_password(const char *password, unsigned long long password_len);

int crypto_encrypt(unsigned char *c, unsigned char *m,
                   unsigned long long mlen);

int crypto_decrypt(unsigned char *m, unsigned char *c,
                   unsigned long long clen);

#define SHADOWVPN_KEY_LEN 32
#define SHADOWVPN_ZERO_BYTES 32
#define SHADOWVPN_OVERHEAD_LEN 24
#define SHADOWVPN_PACKET_OFFSET 8

#endif
