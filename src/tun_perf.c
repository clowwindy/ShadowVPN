#include <sys/types.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/if_tun.h>

int main(int argc, char **argv) {
  struct ifreq ifr;
  int fd, err;
  char dev[] = "tun0";
  char buf[1500];

  if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
    return -1;
  }

  memset(&ifr, 0, sizeof(ifr));

  /* Flags: IFF_TUN   - TUN device (no Ethernet headers) 
   *        IFF_TAP   - TAP device  
   *
   *        IFF_NO_PI - Do not provide packet information  
   */ 
  ifr.ifr_flags = IFF_TUN; 
  if(*dev)
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

  if((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0){
    close(fd);
    return -1;
  }
  int i = 0;
  // send 1.5GB data
  for (i = 0; i < 1000 * 1000; i++) {
    int r = write(fd, buf, sizeof(buf));
  }
  return 0;
}
