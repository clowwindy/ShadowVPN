/**
  vpn.c

  Copyright (c) 2014 clowwindy

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*/

#include <sys/types.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include "shadowvpn.h"

static int tun_alloc(const char *dev);

static int udp_alloc(int if_bind, const char *host, int port,
                     struct sockaddr *addr, socklen_t* addrlen);


static int tun_alloc(const char *dev) {
  struct ifreq ifr;
  int fd, err;

  if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
    err("open");
    errf("can not open /dev/net/tun");
    return -1;
  }

  memset(&ifr, 0, sizeof(ifr));

  /* Flags: IFF_TUN   - TUN device (no Ethernet headers) 
   *        IFF_TAP   - TAP device  
   *
   *        IFF_NO_PI - Do not provide packet information  
   */ 
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI; 
  if(*dev)
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

  if((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0){
    err("ioctl");
    errf("can not setup tun device");
    close(fd);
    return -1;
  }
  // strcpy(dev, ifr.ifr_name);
  return fd;
}

static int udp_alloc(int if_bind, const char *host, int port,
                     struct sockaddr *addr, socklen_t* addrlen) {
  struct addrinfo hints;
  struct addrinfo *res;
  int sock, r, flags;

  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;
  if (0 != (r = getaddrinfo(host, NULL, &hints, &res))) {
    errf("getaddrinfo: %s", gai_strerror(r));
    return -1; 
  }

  if (res->ai_family == AF_INET)
    ((struct sockaddr_in *)res->ai_addr)->sin_port = htons(port);
  else if (res->ai_family == AF_INET6)
    ((struct sockaddr_in6 *)res->ai_addr)->sin6_port = htons(port);
  else {
    errf("unknown ai_family %d", res->ai_family);
    return -1;
  }
  memcpy(addr, res->ai_addr, res->ai_addrlen); 
  *addrlen = res->ai_addrlen;

  if (-1 == (sock = socket(res->ai_family, SOCK_DGRAM, IPPROTO_UDP))) {
    err("socket");
    errf("can not create socket");
    return -1;
  }

  if (if_bind) {
    if (0 != bind(sock, res->ai_addr, res->ai_addrlen)) {
      err("bind");
      errf("can not bind %s:%d", host, port);
      return -1; 
    }
    freeaddrinfo(res);
  }
  flags = fcntl(sock, F_GETFL, 0);
  if (flags != -1) {
    if (-1 != fcntl(sock, F_SETFL, flags | O_NONBLOCK))
      return sock;
  }
  close(sock);
  err("fcntl");
  return -1;
}

int max(int a, int b) {
  return a > b ? a : b;
}

int run_vpn(shadowvpn_args_t *args) {
  fd_set readset;
  int tun, sock, max_fd;
  ssize_t r;
  unsigned char *tun_buf;
  unsigned char *udp_buf;
  struct sockaddr_storage remote_addr;
  struct sockaddr *remote_addrp = (struct sockaddr *)&remote_addr;
  socklen_t remote_addrlen;
  struct sockaddr *src_addrp = NULL;
  socklen_t *src_addrlen = NULL;

  if (args->mode == SHADOWVPN_MODE_SERVER) {
    // if we are running a server, update server address from recv_from
    src_addrp = remote_addrp;
    src_addrlen = &remote_addrlen;
  }

  if (-1 == (tun = tun_alloc(args->intf))) {
    errf("failed to create tun device");
    return -1;
  }
  if (-1 == (sock = udp_alloc(args->mode == SHADOWVPN_MODE_SERVER,
                              args->server, args->port,
                              remote_addrp, &remote_addrlen))) {
    errf("failed to create UDP socket");
    close(tun);
    return -1;
  }

  shell_up(args);

  tun_buf = malloc(args->mtu + SHADOWVPN_ZERO_BYTES);
  udp_buf = malloc(args->mtu + SHADOWVPN_ZERO_BYTES);
  memset(tun_buf, 0, SHADOWVPN_ZERO_BYTES);
  memset(udp_buf, 0, SHADOWVPN_ZERO_BYTES);

  for(;;) {
    FD_ZERO(&readset);
    FD_SET(tun, &readset);
    FD_SET(sock, &readset);
    max_fd = max(tun, sock) + 1;

    if (-1 == select(max_fd, &readset, NULL, NULL, NULL)) {
      if (errno == EINTR)
        continue;
      err("select");
      break;
    }
    if (FD_ISSET(tun, &readset)) {
      r = read(tun, tun_buf + SHADOWVPN_ZERO_BYTES, args->mtu); 
      if (r == -1) {
        // TODO
        err("read from tun");
        break;
      }
      if (remote_addrlen) {
        crypto_encrypt(udp_buf, tun_buf, r);
        r = sendto(sock, udp_buf + SHADOWVPN_PACKET_OFFSET,
                   SHADOWVPN_OVERHEAD_LEN + r, 0,
                   remote_addrp, remote_addrlen);
        if (r == -1) {
          err("sendto");
          // TODO rebuild socket
          break;
        }
      }
    }
    if (FD_ISSET(sock, &readset)) {
      r = recvfrom(sock, udp_buf + SHADOWVPN_PACKET_OFFSET,
                   SHADOWVPN_OVERHEAD_LEN + args->mtu, 0,
                   src_addrp, src_addrlen);
      if (r == -1) {
        err("recvfrom");
        // TODO rebuild socket
        break;
      }

      if (-1 == crypto_decrypt(tun_buf, udp_buf, r - SHADOWVPN_OVERHEAD_LEN)) {
        errf("invalid packet, drop");
      } else {
        if (-1 == write(tun, tun_buf + SHADOWVPN_ZERO_BYTES,
              r - SHADOWVPN_OVERHEAD_LEN)) {
          err("write to tun");
          break;
        }
      }
    }
  }
  free(tun_buf);
  free(udp_buf);

  shell_down(args);

  close(tun);
  close(sock);
  return -1;
}
