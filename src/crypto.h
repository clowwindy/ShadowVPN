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
#define SHADOWVPN_KEY_LEN crypto_stream_salsa20_KEYBYTES
#define SHADOWVPN_NONCE_LEN crypto_stream_salsa20_NONCEBYTES
#define SHADOWVPN_MAC_LEN crypto_onetimeauth_BYTES
#define SHADOWVPN_IV1_LEN 16
#define SHADOWVPN_IV2_LEN 8
#define SHADOWVPN_OVERHEAD_LEN (SHADOWVPN_NONCE_LEN + SHADOWVPN_MAC_LEN + \
                                SHADOWVPN_IV1_LEN + SHADOWVPN_IV2_LEN)

#endif
