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

//queue support
#include <linux/netfilter.h>    /* for NF_ACCEPT */
#include <libnetfilter_queue/libnetfilter_queue.h>
extern int tcp_mode;
extern uint16_t queue_num;

/* the structure to store known client addresses for the server */
typedef struct {
  struct sockaddr_storage addr;
  socklen_t addrlen;
  time_t last_recv_time;
} addr_info_t;

typedef struct {
  int running;
  int nsock;
  int *socks;
  int tun;
  /* select() in winsock doesn't support file handler */
  //queue support
  int q_sock;
  int rv;
  unsigned char *data;
#ifndef TARGET_WIN32
  int control_pipe[2];
#else
  int control_fd;
  struct sockaddr control_addr;
  socklen_t control_addrlen;
  HANDLE cleanEvent;
#endif
  unsigned char *tun_buf;
  unsigned char *udp_buf;

  /* known client addrs for the server */
  int nknown_addr;
  addr_info_t *known_addrs;

  /* the address we currently use */
  struct sockaddr_storage remote_addr;
  struct sockaddr *remote_addrp;
  socklen_t remote_addrlen;
  shadowvpn_args_t *args;
} vpn_ctx_t;

uint32_t xorshift32(uint32_t *a);
int vpn_raw_alloc(const char *host, int port,
                  struct sockaddr *addr, socklen_t* addrlen);
int queue_alloc(void *ctx);
int set_nonblock(int sock);
int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
        struct nfq_data *nfa, void *data);

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
