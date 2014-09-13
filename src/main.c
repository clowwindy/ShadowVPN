#include <stdio.h>
#include <string.h>
#include <sodium.h>
#include "crypto.h"

int main(int arc, char **argv) {
  if (0 != shadowvpn_crypto_init()) {
    perror("shadowvpn_crypto_init");
    return 1;
  }

  char password[256];
  shadowvpn_set_password(password, strlen(password));
  unsigned char plain[1500];
  unsigned char ciphertext[sizeof(plain) + SHADOWVPN_OVERHEAD_LEN];

  shadowvpn_encrypt(ciphertext, plain, sizeof(plain));

  // char hex[sizeof(ciphertext) * 2 + 1];
  // sodium_bin2hex(hex, sizeof(hex), ciphertext, sizeof(ciphertext));
  // printf("%s\n", hex);
  return 0;
}
