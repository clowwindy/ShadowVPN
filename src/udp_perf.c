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

int main(int argc, char **argv) {
  struct addrinfo hints;
  struct addrinfo *res;
  char buf[1500];
  int sock, r, flags;

  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_DGRAM;
  if (0 != (r = getaddrinfo("127.0.0.1", NULL, &hints, &res))) {
    return -1; 
  }

  if (res->ai_family == AF_INET)
    ((struct sockaddr_in *)res->ai_addr)->sin_port = 1234;
  else if (res->ai_family == AF_INET6)
    ((struct sockaddr_in6 *)res->ai_addr)->sin6_port = 1234;
  else {
    return -1;
  }

  if (-1 == (sock = socket(res->ai_family, SOCK_DGRAM, IPPROTO_UDP))) {
    return -1;
  }

  flags = fcntl(sock, F_GETFL, 0);
  if (flags != -1) {
    if (-1 == fcntl(sock, F_SETFL, flags | O_NONBLOCK))
      return -1;
  }
  int i = 0;
  // send 1.5GB data
  for (i = 0; i < 1000 * 1000; i++) {
        r = sendto(sock, buf, sizeof(buf), 0,
                   res->ai_addr, res->ai_addrlen);
        if (r == -1)
          perror("sendto");
  }
  return 0;
}
