#include <sodium.h>
#include "crypto.h"

#define pos_mac(c) c
#define pos_iv1(c) (c + SHADOWVPN_MAC_LEN)
#define pos_iv2(c) (c + SHADOWVPN_MAC_LEN + SHADOWVPN_IV1_LEN)
#define pos_nonce(c) (c + SHADOWVPN_MAC_LEN + SHADOWVPN_IV1_LEN + \
                      SHADOWVPN_IV2_LEN)
#define pos_cipher_text(c) (c + SHADOWVPN_MAC_LEN + SHADOWVPN_IV1_LEN + \
                      SHADOWVPN_IV2_LEN + SHADOWVPN_NONCE_LEN)

// will not copy key any more
static unsigned char key[SHADOWVPN_KEY_LEN];

// a temp buf large enough to hold some temp data
static unsigned char temp[SHADOWVPN_KEY_LEN + crypto_generichash_KEYBYTES];

int shadowvpn_crypto_init() {
  if (-1 == sodium_init())
    return 1;
  randombytes_set_implementation(&randombytes_salsa20_implementation);
  randombytes_stir();
  return 0;
}

int shadowvpn_set_password(const char *password,
                         unsigned long long password_len) {
  return crypto_generichash(key, sizeof key, (unsigned char *)password,
                            password_len, NULL, 0);
}

int shadowvpn_encrypt(unsigned char *c, unsigned char *m,
                      unsigned long long mlen) {
  // generate IV1, IV2, nonce
  randombytes_buf(pos_mac(c), pos_cipher_text(c) - pos_mac(c));
  crypto_generichash_state state;
  // calculate crypt_key
  crypto_generichash_init(&state, NULL, 0, crypto_stream_salsa20_KEYBYTES);
  crypto_generichash_update(&state, key, SHADOWVPN_KEY_LEN);
  crypto_generichash_update(&state, pos_iv2(c), SHADOWVPN_IV2_LEN);
  crypto_generichash_final(&state, temp, crypto_stream_salsa20_KEYBYTES);
  // crypt_key in temp, nonce in pos_nonce(c), do encryption
  crypto_stream_salsa208_xor(pos_cipher_text(c), m, mlen, pos_nonce(c), temp);
  // calculate mac_key
  crypto_generichash_init(&state, NULL, 0, crypto_onetimeauth_KEYBYTES);
  crypto_generichash_update(&state, key, SHADOWVPN_KEY_LEN);
  crypto_generichash_update(&state, pos_iv1(c), SHADOWVPN_IV2_LEN);
  crypto_generichash_final(&state, temp, crypto_onetimeauth_KEYBYTES);
  // mac_key in temp, nonce in pos_nonce(c), do one time auth
  crypto_onetimeauth(pos_mac(c), pos_iv2(c),
                     pos_cipher_text(c) - pos_iv2(c) + mlen, temp);
  return 0;
}

