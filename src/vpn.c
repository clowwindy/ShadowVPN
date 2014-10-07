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
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include "shadowvpn.h"


int vpn_tun_alloc(const char *dev) {
  struct ifreq ifr;
  int fd, e;

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

  if((e = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0){
    err("ioctl");
    errf("can not setup tun device: %s", dev);
    close(fd);
    return -1;
  }
  // strcpy(dev, ifr.ifr_name);
  return fd;
}

int vpn_udp_alloc(int if_bind, const char *host, int port,
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

static int max(int a, int b) {
  return a > b ? a : b;
}

int vpn_ctx_init(vpn_ctx_t *ctx, shadowvpn_args_t *args) {
  bzero(ctx, sizeof(vpn_ctx_t));
  ctx->remote_addrp = (struct sockaddr *)&ctx->remote_addr;

  if (-1 == pipe(ctx->control_pipe)) {
    err("pipe");
    return -1;
  }
  if (-1 == (ctx->tun = vpn_tun_alloc(args->intf))) {
    errf("failed to create tun device");
    return -1;
  }
  if (-1 == (ctx->sock = vpn_udp_alloc(args->mode == SHADOWVPN_MODE_SERVER,
                                       args->server, args->port,
                                       ctx->remote_addrp,
                                       &ctx->remote_addrlen))) {
    errf("failed to create UDP socket");
    close(ctx->tun);
    return -1;
  }
  ctx->args = args;
  return 0;
}

int vpn_run(vpn_ctx_t *ctx) {
  fd_set readset;
  int max_fd;
  ssize_t r;
  if (ctx->running) {
    errf("can not start, already running");
    return -1;
  }

  ctx->running = 1;

  shell_up(ctx->args);

  ctx->tun_buf = malloc(ctx->args->mtu + SHADOWVPN_ZERO_BYTES);
  ctx->udp_buf = malloc(ctx->args->mtu + SHADOWVPN_ZERO_BYTES);
  bzero(ctx->tun_buf, SHADOWVPN_ZERO_BYTES);
  bzero(ctx->udp_buf, SHADOWVPN_ZERO_BYTES);

  logf("VPN started");

  while (ctx->running) {
    FD_ZERO(&readset);
    FD_SET(ctx->control_pipe[0], &readset);
    FD_SET(ctx->tun, &readset);
    FD_SET(ctx->sock, &readset);

    // we assume that pipe fd is always less than tun and sock fd which are
    // created later
    max_fd = max(ctx->tun, ctx->sock) + 1;

    if (-1 == select(max_fd, &readset, NULL, NULL, NULL)) {
      if (errno == EINTR)
        continue;
      err("select");
      break;
    }
    if (FD_ISSET(ctx->control_pipe[0], &readset)) {
      char pipe_buf;
      (void)read(ctx->control_pipe[0], &pipe_buf, 1);
      break;
    }
    if (FD_ISSET(ctx->tun, &readset)) {
      r = read(ctx->tun, ctx->tun_buf + SHADOWVPN_ZERO_BYTES, ctx->args->mtu); 
      if (r == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          // do nothing
        } else if (errno == EPERM || errno == EINTR) {
          // just log, do nothing
          err("read from tun");
        } else {
          err("read from tun");
          break;
        }
      }
      if (ctx->remote_addrlen) {
        crypto_encrypt(ctx->udp_buf, ctx->tun_buf, r);
        r = sendto(ctx->sock, ctx->udp_buf + SHADOWVPN_PACKET_OFFSET,
                   SHADOWVPN_OVERHEAD_LEN + r, 0,
                   ctx->remote_addrp, ctx->remote_addrlen);
        if (r == -1) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // do nothing
          } else if (errno == ENETUNREACH || errno == ENETDOWN ||
                     errno == EPERM || errno == EINTR || errno == EMSGSIZE) {
            // just log, do nothing
            err("sendto");
          } else {
            err("sendto");
            // TODO rebuild socket
            break;
          }
        }
      }
    }
    if (FD_ISSET(ctx->sock, &readset)) {
      // only change remote addr if decryption succeeds
      struct sockaddr_storage temp_remote_addr;
      socklen_t temp_remote_addrlen = sizeof(temp_remote_addr);
      r = recvfrom(ctx->sock, ctx->udp_buf + SHADOWVPN_PACKET_OFFSET,
                   SHADOWVPN_OVERHEAD_LEN + ctx->args->mtu, 0,
                   (struct sockaddr *)&temp_remote_addr,
                   &temp_remote_addrlen);
      if (r == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          // do nothing
        } else if (errno == ENETUNREACH || errno == ENETDOWN ||
                   errno == EPERM || errno == EINTR) {
          // just log, do nothing
          err("recvfrom");
        } else {
          err("recvfrom");
          // TODO rebuild socket
          break;
        }
      }
      if (r == 0)
        continue;

      if (-1 == crypto_decrypt(ctx->tun_buf, ctx->udp_buf,
                               r - SHADOWVPN_OVERHEAD_LEN)) {
        errf("dropping invalid packet, maybe wrong password");
      } else {
        if (ctx->args->mode == SHADOWVPN_MODE_SERVER) {
          // if we are running a server, update server address from recv_from
          memcpy(ctx->remote_addrp, &temp_remote_addr, temp_remote_addrlen);
          ctx->remote_addrlen = temp_remote_addrlen;
        }

        if (-1 == write(ctx->tun, ctx->tun_buf + SHADOWVPN_ZERO_BYTES,
              r - SHADOWVPN_OVERHEAD_LEN)) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // do nothing
          } else if (errno == EPERM || errno == EINTR || errno == EINVAL) {
            // just log, do nothing
            err("write to tun");
          } else {
            err("write to tun");
            break;
          }
        }
      }
    }
  }
  free(ctx->tun_buf);
  free(ctx->udp_buf);

  shell_down(ctx->args);

  close(ctx->tun);
  close(ctx->sock);

  ctx->running = 0;
  return -1;
}

int vpn_stop(vpn_ctx_t *ctx) {
  logf("shutting down by user");
  if (!ctx->running) {
    errf("can not stop, not running");
    return -1;
  }
  ctx->running = 0;
  char buf = 0;
  if (-1 == write(ctx->control_pipe[1], &buf, 1)) {
    err("write");
    return -1;
  }
  return 0;
}
