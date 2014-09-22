#include <stdlib.h>
#include "shadowvpn.h"

int main(int argc, char **argv) {
  int i;

  unsigned char *tun_buf = malloc(1532);
  unsigned char *udp_buf = malloc(1532);
  memset(tun_buf, 0, SHADOWVPN_ZERO_BYTES);
  memset(udp_buf, 0, SHADOWVPN_ZERO_BYTES);

  for(i = 0; i < 1000 * 100; i++) {
      crypto_decrypt(tun_buf, udp_buf, 1500);
  }
  return 0;
}
