#include <stdio.h>
#include <sodium.h>

int main(int arc, char **argv) {
  if (sodium_init() == -1) {
    return 1;
  }
  randombytes_set_implementation(&randombytes_salsa20_implementation);
  randombytes_stir();

  char plain[1500 - crypto_aead_chacha20poly1305_ABYTES];
  char ciphertext[1500];
  unsigned char nonce[crypto_aead_chacha20poly1305_NPUBBYTES];
  unsigned char key[crypto_aead_chacha20poly1305_KEYBYTES];
  printf("%ld\n", crypto_aead_chacha20poly1305_KEYBYTES);
  printf("%ld\n", crypto_aead_chacha20poly1305_NPUBBYTES);
  printf("%ld\n", crypto_aead_chacha20poly1305_ABYTES);
  randombytes_buf(key, sizeof key);
  long long clen;
  for (int i = 0; i < 1000 * 100; i++) {
    clen = 1500;
    //randombytes_buf(nonce, sizeof nonce);
    //crypto_hash_sha256(nonce, nonce, sizeof(nonce));
    //crypto_generichash(nonce, sizeof(nonce), plain, sizeof(plain), NULL, 0);
    // generate nonce using chacha20
    //crypto_stream_chacha20(nonce, sizeof(nonce), nonce, key);
    //crypto_secretbox_easy(ciphertext, plain, sizeof(plain), nonce, key);
    crypto_stream_chacha20_xor(ciphertext, plain, sizeof(plain), nonce, key);
    //crypto_onetimeauth(ciphertext, plain, sizeof(plain), key);
    //crypto_aead_chacha20poly1305_encrypt(ciphertext, &clen,
    //                                     plain, sizeof(plain), NULL, 0, NULL,
    //                                     nonce, key);
  }
  return 0;
}
