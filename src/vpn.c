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

//for raw packet
#include <netinet/tcp.h>
#include <netinet/ip6.h>
#include <netinet/ip.h>
//bpf filter
#include <linux/if_ether.h>
#include <linux/filter.h>

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

int is_ipv6;

uint32_t xorshift32(uint32_t *a) {
  uint32_t state = *a;
  state ^= (state << 13);
  state ^= (state >> 17);
  state ^= (state <<  5);
  *a = state;
  return state;
}

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


int vpn_raw_alloc(int is_server, const char *host, int port,
                  struct sockaddr *addr, socklen_t* addrlen) {
  struct addrinfo hints;
  struct addrinfo *res;
  int sock, r;

  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  if (0 != (r = getaddrinfo(host, NULL, &hints, &res))) {
    errf("getaddrinfo: %s", gai_strerror(r));
    return -1;
  }

  if (res->ai_family == AF_INET)
    ((struct sockaddr_in *)res->ai_addr)->sin_port = htons(port);
  else if (res->ai_family == AF_INET6) {
    is_ipv6 = 1;
    ((struct sockaddr_in6 *)res->ai_addr)->sin6_port = htons(port);
  }
  else {
    errf("unknown ai_family %d", res->ai_family);
    freeaddrinfo(res);
    return -1;
  }

  memcpy(addr, res->ai_addr, res->ai_addrlen);
  *addrlen = res->ai_addrlen;

  if (-1 == (sock = socket(res->ai_family, SOCK_RAW, IPPROTO_TCP))) {
    err("socket");
    errf("can not create RAW socket");
    freeaddrinfo(res);
    return -1;
  }


  //bpf here
  int port_offset;
  if (is_server) {
    if (res->ai_family == AF_INET) //TODO: add ipv6 support
      port_offset = 22;
  } else {
    if (res->ai_family == AF_INET)
      port_offset = 20;
  }

   if (is_server) {
     struct sock_filter code[] = {
       { 0x28,  0,  0, port_offset },
       { 0x15,  0,  1, port },
       { 0x06,  0,  0, 0x0000ffff },
       { 0x06,  0,  0, 0000000000 },
     };
      struct sock_fprog bpf = {
      .len = ARRAY_SIZE(code),
      .filter = code,
      };
      if (-1 == setsockopt(sock, SOL_SOCKET, SO_ATTACH_FILTER, &bpf, sizeof(bpf))) {
        err("filter");
        errf("can not attach filter");
        freeaddrinfo(res);
        return -1;
      }
   } else {
     struct sock_filter code[] = {
      { 0x28,  0,  0, port_offset },
      { 0x15,  0,  3, port },
      { 0x20,  0,  0, 0x0000000c },
      { 0x15,  0,  1, ntohl(((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr) }, //TODO add ipv6 support
      { 0x06,  0,  0, 0x0000ffff },
      { 0x06,  0,  0, 0000000000 },
     };
     struct sock_fprog bpf = {
       .len = ARRAY_SIZE(code),
       .filter = code,
     };
     if (-1 == setsockopt(sock, SOL_SOCKET, SO_ATTACH_FILTER, &bpf, sizeof(bpf))) {
       err("filter");
       errf("can not attach filter");
       freeaddrinfo(res);
       return -1;
     }
   }


  freeaddrinfo(res);

  return set_nonblock(sock);
}


int vpn_udp_alloc(int if_bind, const char *host, int port,
                  struct sockaddr *addr, socklen_t* addrlen) {
  struct addrinfo hints;
  struct addrinfo *res;
  int sock, r;

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

  return set_nonblock(sock);
}

int set_nonblock(int sock) {
  int flags;
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
  ctx->sock = vpn_raw_alloc(args->mode == SHADOWVPN_MODE_SERVER, args->server, args->port,
                                (struct sockaddr *)&ctx->remote_addr,
                                &ctx->remote_addrlen);
  if (ctx->sock == -1) {
      errf("failed to create RAW socket");
      close(ctx->tun);
      return -1;
  }

  ctx->args = args;
  return 0;
}

int vpn_run(vpn_ctx_t *ctx) {
  if (ctx->running) {
    errf("can not start, already running");
    return -1;
  }
  ctx->running = 1;
  shell_up(ctx->args);
  logf("VPN started");

  fd_set readset;
  int max_fd = 0, isv6 = is_ipv6;
  ssize_t r;
  uint32_t rand = 314159265;
  uint16_t c_port, s_port = htons(ctx->args->port); //client port & server port

  int mtu = ctx->args->mtu;

  int tun = ctx->tun;
  int sock = ctx->sock;

 #ifndef TARGET_WIN32
  int control_pipe[2];
  control_pipe[0] = ctx->control_pipe[0];
  control_pipe[1] = ctx->control_pipe[1];
 #else
  int control_fd = ctx->control_fd;
 #endif

  struct sockaddr *remote_addrp = malloc(sizeof(struct sockaddr_storage));
  remote_addrp = (struct sockaddr *)&ctx->remote_addr;
  socklen_t remote_addrlen = ctx->remote_addrlen;

  unsigned char *tun_buf = malloc(mtu + SHADOWVPN_ZERO_BYTES);
  unsigned char *tcp_buf = malloc(4096);
  struct tcphdr *tcphdr = (struct tcphdr*)tcp_buf;

  bzero(tun_buf, SHADOWVPN_ZERO_BYTES);
  bzero(tcp_buf, SHADOWVPN_ZERO_BYTES);

  int is_server = (ctx->args->mode == SHADOWVPN_MODE_SERVER)? 1 : 0;

  while (ctx->running) {
    FD_ZERO(&readset);
#ifndef TARGET_WIN32
    FD_SET(ctx->control_pipe[0], &readset);
#else
    FD_SET(control_fd, &readset);
#endif
    FD_SET(tun, &readset);

    FD_SET(sock, &readset);
    max_fd = max(max_fd, sock);

    // we assume that pipe fd is always less than tun and sock fd which are
    // created later
    max_fd = max(tun, max_fd) + 1;

    if (-1 == select(max_fd, &readset, NULL, NULL, NULL)) {
      if (errno == EINTR)
        continue;
      err("select");
      break;
    }
#ifndef TARGET_WIN32
    if (FD_ISSET(control_pipe[0], &readset)) {
      char pipe_buf;
      (void)read(control_pipe[0], &pipe_buf, 1);
      break;
    }
#else
    if (FD_ISSET(control_fd, &readset)) {
      char buf;
      recv(control_fd, &buf, 1, 0);
      break;
    }
#endif
    if (FD_ISSET(tun, &readset)) {
      r = tun_read(tun, tun_buf + 32, mtu);
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
      if (remote_addrlen) {
        crypto_encrypt(tcp_buf + 12, tun_buf, r);
        if (is_server) {
          tcphdr->source = s_port;
          tcphdr->dest = c_port;
        } else {
          tcphdr->dest = s_port;
          tcphdr->source = (uint16_t) xorshift32(&rand);
        }
        tcphdr->seq = rand;
        tcphdr->syn = 1; //must be a syn packet
        tcphdr->doff = 5;
        r = sendto(sock, tcp_buf,
                   SHADOWVPN_OVERHEAD_LEN + r + 20, 0,
                   remote_addrp, remote_addrlen);
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
    if (FD_ISSET(sock, &readset)) {
      // only change remote addr if decryption succeeds
      int decrypt;
      r = recv(sock, tcp_buf, 4096, 0);
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

      if (!isv6) {
        decrypt = crypto_decrypt(tun_buf, tcp_buf + 32,
                              r - SHADOWVPN_OVERHEAD_LEN - 40);
        r = r - SHADOWVPN_OVERHEAD_LEN - 40;
      } else {
        decrypt = crypto_decrypt(tun_buf, tcp_buf + 52,
                              r - SHADOWVPN_OVERHEAD_LEN - 60);
        r = r - SHADOWVPN_OVERHEAD_LEN - 60;
      }

      if (-1 == decrypt) {
        errf("dropping invalid packet, maybe wrong password");
      } else {
        if (is_server) {
          // if we are running a server, update server address from
          if (!isv6) {
            struct sockaddr_in *temp = (struct sockaddr_in *)remote_addrp;
            //temp->sin_family = AF_INET; //enable this after enabling dual stack
            temp->sin_addr.s_addr = ((struct iphdr *)(tcp_buf))->saddr;
            c_port = ((struct tcphdr *)(tcp_buf + 20))->source;
            //remote_addrlen = sizeof(struct sockaddr_in);
          } else {
            struct sockaddr_in6 *temp = (struct sockaddr_in6 *)remote_addrp;
            //temp->sin6_family = AF_INET6;
            memcpy(temp->sin6_addr.s6_addr, ((struct ip6_hdr *)(tcp_buf))->ip6_src.s6_addr, 16);
            c_port = ((struct tcphdr *)(tcp_buf + 40))->source;
            //remote_addrlen = sizeof(struct sockaddr_in6);
          }
        }
        if (-1 == tun_write(tun, tun_buf + 32, r)) {
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
  free(tun_buf);
  free(tcp_buf);

  shell_down(ctx->args);

  close(tun);
  close(sock);

  ctx->running = 0;

#ifdef TARGET_WIN32
  close(control_fd);
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
