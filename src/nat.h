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

#ifndef NAT_H
#define NAT_H

#ifdef TARGET_WIN32
#include "win32.h"
#else
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

#include "uthash.h"

/**
  This module maps any IP from the client net to the server net
  based on user_token.
*/

/* the structure to store known client addresses for the server */
typedef struct {
  struct sockaddr_storage addr;
  socklen_t addrlen;
  time_t last_recv_time;
} addr_info_t;

/* the structure to store known client addresses for the server */
typedef struct {
  int id;
  char user_token[SHADOWVPN_USERTOKEN_LEN];

  // source address of UDP
  addr_info_t source_addr;

  // input tun IP
  // TODO support IPv6 address on tun
  uint32_t input_tun_ip;

  // output tun IP
  uint32_t output_tun_ip;

  UT_hash_handle hh;
} client_info_t;

typedef struct {
  /* clients map
     key: user token */
  client_info_t *token_to_clients;

  /* clients array
     uses array because it's fast
     index: output IP - tun IP - 1 */
  client_info_t *ip_to_clients;
} nat_ctx_t;

/* init hash tables */
int nat_init(nat_ctx_t *ctx, shadowvpn_args_t *args);

/* UDP -> TUN NAT
   buf starts from payload
*/
int nat_fix_upstream(nat_ctx_t *ctx, unsigned char *buf, size_t buflen,
                     const struct sockaddr *addr, socklen_t addrlen);

/* TUN -> UDP NAT 
   buf starts from payload
*/
int nat_fix_downstream(nat_ctx_t *ctx, unsigned char *buf, size_t buflen,
                       struct sockaddr *addr, socklen_t *addrlen);

#endif
