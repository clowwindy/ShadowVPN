#include <stdio.h>
#include <string.h>
#include <sodium.h>
#include "shadowvpn.h"

#define MTU 1500

static shadowvpn_args_t args;

int main(int argc, char **argv) {
  // parse args
  if (0 != args_parse(&args, argc, argv)) {
    perror("can not parse args");
    return EXIT_FAILURE;
  }
  if (0 != crypto_init()) {
    perror("shadowvpn_crypto_init");
    return EXIT_FAILURE;
  }

  char password[] = "secret";
  crypto_set_password(password, strlen(password));
  unsigned char plain[SHADOWVPN_PACKET_OFFSET + MTU];
  unsigned char ciphertext[sizeof(plain)];

  strcpy((char *)plain + SHADOWVPN_ZERO_BYTES, "I am the plain text!");

  crypto_encrypt(ciphertext, plain, sizeof(plain));
  // ciphertext[256] ^= 1;  // try to tamper with it
  if (0 != crypto_decrypt(plain, ciphertext, sizeof(ciphertext))) {
    printf("invalid packet!\n");
    return 1;
  }
  // char hex[sizeof(ciphertext) * 2 + 1];
  // sodium_bin2hex(hex, sizeof(hex), ciphertext, sizeof(ciphertext));
  printf("%s\n", plain + SHADOWVPN_ZERO_BYTES);
  return 0;
}
