#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sodium.h>
#include <sys/select.h>
#include <sys/socket.h>
#include "shadowvpn.h"

#define MTU 1500

static shadowvpn_args_t args;

int main(int argc, char **argv) {
  // parse args
  if (0 != args_parse(&args, argc, argv)) {
    errf("error when parsing args");
    return EXIT_FAILURE;
  }
  if (args.cmd == SHADOWVPN_CMD_START) {
    if (0 != daemon_start(&args)) {
      errf("can not start daemon");
      return EXIT_FAILURE;
    }
  } else if (args.cmd == SHADOWVPN_CMD_STOP) {
    if (0 != daemon_stop(&args)) {
      errf("can not stop daemon");
      return EXIT_FAILURE;
    }
    // always exit if we are exec stop cmd
    return 0;
  } else if (args.cmd == SHADOWVPN_CMD_RESTART) {
    if (0 != daemon_stop(&args)) {
      errf("can not stop daemon");
      return EXIT_FAILURE;
    }
    if (0 != daemon_start(&args)) {
      errf("can not start daemon");
      return EXIT_FAILURE;
    }
  }

  if (0 != crypto_init()) {
    errf("shadowvpn_crypto_init");
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

  // temp code, just for testing daemon
  struct addrinfo hints;
  struct addrinfo *addr_ip;
  int local_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  int r;
  if (0 != (r = getaddrinfo("0.0.0.0", "1234", &hints, &addr_ip))) {
    return -1; 
  }
  if (0 != bind(local_sock, addr_ip->ai_addr, addr_ip->ai_addrlen)) {
    return -1; 
  }
  freeaddrinfo(addr_ip);
  fd_set readset, errorset;
  int max_fd = local_sock + 1;
  while (1) {
    FD_ZERO(&readset);
    FD_ZERO(&errorset);
    FD_SET(local_sock, &readset);
    FD_SET(local_sock, &errorset);
    struct timeval timeout = {
      .tv_sec = 0,
      .tv_usec = 0,
    };
    if (-1 == select(max_fd, &readset, NULL, &errorset, &timeout)) {
      err("select");
      return EXIT_FAILURE;
    }
  }


  return 0;
}
