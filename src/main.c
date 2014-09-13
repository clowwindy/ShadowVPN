#include <stdio.h>
#include <string.h>
#include <sodium.h>
#include "crypto.h"

#define MTU 1500

int main(int arc, char **argv) {
  if (0 != shadowvpn_crypto_init()) {
    perror("shadowvpn_crypto_init");
    return 1;
  }

  char password[] = "secret";
  shadowvpn_set_password(password, strlen(password));
  unsigned char plain[SHADOWVPN_PACKET_OFFSET + MTU];
  unsigned char ciphertext[sizeof(plain)];

  strcpy((char *)plain + SHADOWVPN_ZERO_BYTES, "I am the plain text!");

  shadowvpn_encrypt(ciphertext, plain, sizeof(plain));
  // ciphertext[256] ^= 1;  // try to tamper with it
  if (0 != shadowvpn_decrypt(plain, ciphertext, sizeof(ciphertext))) {
    printf("invalid packet!\n");
    return 1;
  }
  // char hex[sizeof(ciphertext) * 2 + 1];
  // sodium_bin2hex(hex, sizeof(hex), ciphertext, sizeof(ciphertext));
  printf("%s\n", plain + SHADOWVPN_ZERO_BYTES);
  return 0;
}
