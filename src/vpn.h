/**
  vpn.h

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

#ifndef VPN_H
#define VPN_H

#include <time.h>

#ifdef TARGET_WIN32
#include "win32.h"
#else
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif
#include "args.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))
#define __must_be_array(a) BUILD_BUG_ON_ZERO(__same_type((a), &(a)[0]))
#define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))
#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))

typedef struct {
  int running;
  int sock;
  int tun;
  /* select() in winsock doesn't support file handler */
#ifndef TARGET_WIN32
  int control_pipe[2];
#else
  int control_fd;
  struct sockaddr control_addr;
  socklen_t control_addrlen;
  HANDLE cleanEvent;
#endif
  /* the address we currently use */
  struct sockaddr_storage remote_addr;
  socklen_t remote_addrlen;
  shadowvpn_args_t *args;
} vpn_ctx_t;

uint32_t xorshift32(uint32_t *a);
int vpn_raw_alloc(int is_server, const char *host, int port,
                  struct sockaddr *addr, socklen_t* addrlen);

int set_nonblock(int sock);

/* return -1 on error. no need to destroy any resource */
int vpn_ctx_init(vpn_ctx_t *ctx, shadowvpn_args_t *args);

/* return -1 on error. no need to destroy any resource */
int vpn_run(vpn_ctx_t *ctx);

/* return -1 on error. no need to destroy any resource */
int vpn_stop(vpn_ctx_t *ctx);

/* these low level functions are exposed for Android jni */
#ifndef TARGET_WIN32
int vpn_tun_alloc(const char *dev);
#endif
int vpn_udp_alloc(int if_bind, const char *host, int port,
                  struct sockaddr *addr, socklen_t* addrlen);

#endif