#ifndef CRYPTO_H
#define CRYPTO_H

int shadowvpn_crypto_init();

int shadowvpn_set_password(const char *password,
                           unsigned long long password_len);

int shadowvpn_encrypt(unsigned char *c, unsigned char *m,
                      unsigned long long mlen);

int shadowvpn_decrypt(unsigned char *m, unsigned char *c,
                      unsigned long long clen);
/*
   MAC IV1 (IV2 NONCE CIPHER_TEXT)
*/
#define SHADOWVPN_KEY_LEN 32
#define SHADOWVPN_ZERO_BYTES 32
#define SHADOWVPN_OVERHEAD_LEN 24
#define SHADOWVPN_PACKET_OFFSET 8
#endif
