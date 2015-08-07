/**
  vpn.c

  Copyright (C) 2015 clowwindy

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

// TODO we want to put shadowvpn.h at the bottom of the imports
// but TARGET_* is defined in config.h
#include "shadowvpn.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#ifndef TARGET_WIN32
#include <sys/select.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#endif

#ifdef TARGET_DARWIN
#include <sys/kern_control.h>
#include <net/if_utun.h>
#include <sys/sys_domain.h>
#include <netinet/ip.h>
#include <sys/uio.h>
#endif

#ifdef TARGET_LINUX
#include <linux/if_tun.h>
#endif

#ifdef TARGET_FREEBSD
#include <net/if_tun.h>
#endif


/*
 * Darwin & OpenBSD use utun which is slightly
 * different from standard tun device. It adds
 * a uint32 to the beginning of the IP header
 * to designate the protocol.
 *
 * We use utun_read to strip off the header
 * and utun_write to put it back.
 */
#ifdef TARGET_DARWIN
#define tun_read(...) utun_read(__VA_ARGS__)
#define tun_write(...) utun_write(__VA_ARGS__)
#elif !defined(TARGET_WIN32)
#define tun_read(...) read(__VA_ARGS__)
#define tun_write(...) write(__VA_ARGS__)
#endif

#ifdef TARGET_WIN32

#undef errno
#undef EWOULDBLOCK
#undef EAGAIN
#undef EINTR
#undef ENETDOWN
#undef ENETUNREACH
#undef EMSGSIZE

#define errno WSAGetLastError()
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EAGAIN WSAEWOULDBLOCK
#define EINTR WSAEINTR
#define ENETUNREACH WSAENETUNREACH
#define ENETDOWN WSAENETDOWN
#define EMSGSIZE WSAEMSGSIZE
#define close(fd) closesocket(fd)

#endif

#ifdef TARGET_LINUX
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

  if ((e = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) {
    err("ioctl[TUNSETIFF]");
    errf("can not setup tun device: %s", dev);
    close(fd);
    return -1;
  }
  // strcpy(dev, ifr.ifr_name);
  return fd;
}
#endif

#ifdef TARGET_FREEBSD
int vpn_tun_alloc(const char *dev) {
  int fd;
  char devname[32]={0,};
  snprintf(devname, sizeof(devname), "/dev/%s", dev);
  if ((fd = open(devname, O_RDWR)) < 0) {
    err("open");
    errf("can not open %s", devname);
    return -1;
  }
  int i = IFF_POINTOPOINT | IFF_MULTICAST;
  if (ioctl(fd, TUNSIFMODE, &i) < 0) {
    err("ioctl[TUNSIFMODE]");
    errf("can not setup tun device: %s", dev);
    close(fd);
    return -1;
  }
  i = 0;
  if (ioctl(fd, TUNSIFHEAD, &i) < 0) {
    err("ioctl[TUNSIFHEAD]");
    errf("can not setup tun device: %s", dev);
    close(fd);
    return -1;
  }
  return fd;
}
#endif

#ifdef TARGET_DARWIN
static inline int utun_modified_len(int len) {
  if (len > 0)
    return (len > sizeof (u_int32_t)) ? len - sizeof (u_int32_t) : 0;
  else
    return len;
}

static int utun_write(int fd, void *buf, size_t len) {
  u_int32_t type;
  struct iovec iv[2];
  struct ip *iph;

  iph = (struct ip *) buf;

  if (iph->ip_v == 6)
    type = htonl(AF_INET6);
  else
    type = htonl(AF_INET);

  iv[0].iov_base = &type;
  iv[0].iov_len = sizeof(type);
  iv[1].iov_base = buf;
  iv[1].iov_len = len;

  return utun_modified_len(writev(fd, iv, 2));
}

static int utun_read(int fd, void *buf, size_t len) {
  u_int32_t type;
  struct iovec iv[2];

  iv[0].iov_base = &type;
  iv[0].iov_len = sizeof(type);
  iv[1].iov_base = buf;
  iv[1].iov_len = len;

  return utun_modified_len(readv(fd, iv, 2));
}

int vpn_tun_alloc(const char *dev) {
  struct ctl_info ctlInfo;
  struct sockaddr_ctl sc;
  int fd;
  int utunnum;

  if (dev == NULL) {
    errf("utun device name cannot be null");
    return -1;
  }
  if (sscanf(dev, "utun%d", &utunnum) != 1) {
    errf("invalid utun device name: %s", dev);
    return -1;
  }

  memset(&ctlInfo, 0, sizeof(ctlInfo));
  if (strlcpy(ctlInfo.ctl_name, UTUN_CONTROL_NAME, sizeof(ctlInfo.ctl_name)) >=
      sizeof(ctlInfo.ctl_name)) {
    errf("can not setup utun device: UTUN_CONTROL_NAME too long");
    return -1;
  }

  fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);

  if (fd == -1) {
    err("socket[SYSPROTO_CONTROL]");
    return -1;
  }

  if (ioctl(fd, CTLIOCGINFO, &ctlInfo) == -1) {
    close(fd);
    err("ioctl[CTLIOCGINFO]");
    return -1;
  }

  sc.sc_id = ctlInfo.ctl_id;
  sc.sc_len = sizeof(sc);
  sc.sc_family = AF_SYSTEM;
  sc.ss_sysaddr = AF_SYS_CONTROL;
  sc.sc_unit = utunnum + 1;

  if (connect(fd, (struct sockaddr *) &sc, sizeof(sc)) == -1) {
    close(fd);
    err("connect[AF_SYS_CONTROL]");
    return -1;
  }

  return fd;
}
#endif

#ifdef TARGET_WIN32
static int tun_write(int tun_fd, char *data, size_t len) {
  DWORD written;
  DWORD res;
  OVERLAPPED olpd;

  olpd.Offset = 0;
  olpd.OffsetHigh = 0;
  olpd.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  res = WriteFile(dev_handle, data, len, &written, &olpd);
  if (!res && GetLastError() == ERROR_IO_PENDING) {
    WaitForSingleObject(olpd.hEvent, INFINITE);
    res = GetOverlappedResult(dev_handle, &olpd, &written, FALSE);
    if (written != len) {
      return -1;
    }
  }
  return 0;
}

static int tun_read(int tun_fd, char *buf, size_t len) {
  return recv(tun_fd, buf, len, 0);
}
#endif

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
    freeaddrinfo(res);
    return -1;
  }
  memcpy(addr, res->ai_addr, res->ai_addrlen);
  *addrlen = res->ai_addrlen;

  if (-1 == (sock = socket(res->ai_family, SOCK_DGRAM, IPPROTO_UDP))) {
    err("socket");
    errf("can not create socket");
    freeaddrinfo(res);
    return -1;
  }

  if (if_bind) {
    if (0 != bind(sock, res->ai_addr, res->ai_addrlen)) {
      err("bind");
      errf("can not bind %s:%d", host, port);
      close(sock);
      freeaddrinfo(res);
      return -1;
    }
  }
  freeaddrinfo(res);

#ifndef TARGET_WIN32
  flags = fcntl(sock, F_GETFL, 0);
  if (flags != -1) {
    if (-1 != fcntl(sock, F_SETFL, flags | O_NONBLOCK))
      return sock;
  }
  err("fcntl");
#else
  u_long mode = 0;
  if (NO_ERROR == ioctlsocket(sock, FIONBIO, &mode))
    return disable_reset_report(sock);
  err("ioctlsocket");
#endif

  close(sock);
  return -1;
}

#ifndef TARGET_WIN32
static int max(int a, int b) {
  return a > b ? a : b;
}
#endif

int vpn_ctx_init(vpn_ctx_t *ctx, shadowvpn_args_t *args) {
  int i;
#ifdef TARGET_WIN32
  WORD wVersionRequested;
  WSADATA wsaData;
  int ret;

  wVersionRequested = MAKEWORD(1, 1);
  ret = WSAStartup(wVersionRequested, &wsaData);
  if (ret != 0) {
    errf("can not initialize winsock");
    return -1;
  }
  if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1) {
    WSACleanup();
    errf("can not find a usable version of winsock");
    return -1;
  }
#endif

  bzero(ctx, sizeof(vpn_ctx_t));
  ctx->remote_addrp = (struct sockaddr *)&ctx->remote_addr;

#ifndef TARGET_WIN32
  if (-1 == pipe(ctx->control_pipe)) {
    err("pipe");
    return -1;
  }
  if (-1 == (ctx->tun = vpn_tun_alloc(args->intf))) {
    errf("failed to create tun device");
    return -1;
  }
#else
  if (-1 == (ctx->control_fd = vpn_udp_alloc(1, TUN_DELEGATE_ADDR,
                                             args->tun_port + 1,
                                             &ctx->control_addr,
                                             &ctx->control_addrlen))) {
    err("failed to create control socket");
    return -1;
  }
  if (NULL == (ctx->cleanEvent = CreateEvent(NULL, TRUE, FALSE, NULL))) {
    err("CreateEvent");
    return -1;
  }
  if (-1 == (ctx->tun = tun_open(args->intf, args->tun_ip, args->tun_mask,
                                 args->tun_port))) {
    errf("failed to create tun device");
    return -1;
  }
#endif
  ctx->nsock = 1;
  ctx->socks = calloc(ctx->nsock, sizeof(int));
  for (i = 0; i < ctx->nsock; i++) {
    int *sock = ctx->socks + i;
    if (-1 == (*sock = vpn_udp_alloc(args->mode == SHADOWVPN_MODE_SERVER,
                                     args->server, args->port,
                                     ctx->remote_addrp,
                                     &ctx->remote_addrlen))) {
      errf("failed to create UDP socket");
      close(ctx->tun);
      return -1;
    }
  }
  ctx->args = args;
  return 0;
}

int vpn_run(vpn_ctx_t *ctx) {
  fd_set readset;
  int max_fd = 0, i;
  ssize_t r;
  size_t usertoken_len = 0;
  if (ctx->running) {
    errf("can not start, already running");
    return -1;
  }

  ctx->running = 1;

  shell_up(ctx->args);

  if (ctx->args->user_tokens_len) {
    usertoken_len = SHADOWVPN_USERTOKEN_LEN;
  }

  ctx->tun_buf = malloc(ctx->args->mtu + SHADOWVPN_ZERO_BYTES +
                        usertoken_len);
  ctx->udp_buf = malloc(ctx->args->mtu + SHADOWVPN_ZERO_BYTES +
                        usertoken_len);
  bzero(ctx->tun_buf, SHADOWVPN_ZERO_BYTES);
  bzero(ctx->udp_buf, SHADOWVPN_ZERO_BYTES);

  logf("VPN started");

  while (ctx->running) {
    FD_ZERO(&readset);
#ifndef TARGET_WIN32
    FD_SET(ctx->control_pipe[0], &readset);
#else
    FD_SET(ctx->control_fd, &readset);
#endif
    FD_SET(ctx->tun, &readset);

    max_fd = 0;
    for (i = 0; i < ctx->nsock; i++) {
      FD_SET(ctx->socks[i], &readset);
      max_fd = max(max_fd, ctx->socks[i]);
    }

    // we assume that pipe fd is always less than tun and sock fd which are
    // created later
    max_fd = max(ctx->tun, max_fd) + 1;

    if (-1 == select(max_fd, &readset, NULL, NULL, NULL)) {
      if (errno == EINTR)
        continue;
      err("select");
      break;
    }
#ifndef TARGET_WIN32
    if (FD_ISSET(ctx->control_pipe[0], &readset)) {
      char pipe_buf;
      (void)read(ctx->control_pipe[0], &pipe_buf, 1);
      break;
    }
#else
    if (FD_ISSET(ctx->control_fd, &readset)) {
      char buf;
      recv(ctx->control_fd, &buf, 1, 0);
      break;
    }
#endif
    if (FD_ISSET(ctx->tun, &readset)) {
      r = tun_read(ctx->tun,
                   ctx->tun_buf + SHADOWVPN_ZERO_BYTES + usertoken_len,
                   ctx->args->mtu);
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
      if (usertoken_len) {
        if (ctx->args->mode == SHADOWVPN_MODE_CLIENT) {
          memcpy(ctx->udp_buf + SHADOWVPN_ZERO_BYTES,
                 ctx->args->user_tokens[0], usertoken_len);
        } else {
          // TODO
        }
      }
      if (ctx->remote_addrlen) {
        crypto_encrypt(ctx->udp_buf, ctx->tun_buf, r);

        // TODO concurrency is currently removed
        int sock_to_send = ctx->socks[0];

        r = sendto(sock_to_send, ctx->udp_buf + SHADOWVPN_PACKET_OFFSET,
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
    for (i = 0; i < ctx->nsock; i++) {
      int sock = ctx->socks[i];
      if (FD_ISSET(sock, &readset)) {
        // only change remote addr if decryption succeeds
        struct sockaddr_storage temp_remote_addr;
        socklen_t temp_remote_addrlen = sizeof(temp_remote_addr);
        r = recvfrom(sock, ctx->udp_buf + SHADOWVPN_PACKET_OFFSET,
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
            // if we are running a server, update server address from
            // recv_from
            memcpy(ctx->remote_addrp, &temp_remote_addr, temp_remote_addrlen);
            ctx->remote_addrlen = temp_remote_addrlen;
          }

          if (-1 == tun_write(ctx->tun,
                              ctx->tun_buf + SHADOWVPN_ZERO_BYTES +
                              usertoken_len,
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
  }
  free(ctx->tun_buf);
  free(ctx->udp_buf);

  shell_down(ctx->args);

  close(ctx->tun);
  for (i = 0; i < ctx->nsock; i++) {
    close(ctx->socks[i]);
  }

  ctx->running = 0;

#ifdef TARGET_WIN32
  close(ctx->control_fd);
  WSACleanup();
  SetEvent(ctx->cleanEvent);
#endif

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
#ifndef TARGET_WIN32
  if (-1 == write(ctx->control_pipe[1], &buf, 1)) {
    err("write");
    return -1;
  }
#else
  int send_sock;
  struct sockaddr addr;
  socklen_t addrlen;
  if (-1 == (send_sock = vpn_udp_alloc(0, TUN_DELEGATE_ADDR, 0, &addr,
                                       &addrlen))) {
    errf("failed to init control socket");
    return -1;
  }
  if (-1 == sendto(send_sock, &buf, 1, 0, &ctx->control_addr,
                   ctx->control_addrlen)) {
    err("sendto");
    close(send_sock);
    return -1;
  }
  close(send_sock);
  WaitForSingleObject(ctx->cleanEvent, INFINITE);
  CloseHandle(ctx->cleanEvent);
#endif
  return 0;
}
