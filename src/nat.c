/**
  nat.c

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

#include "shadowvpn.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include "portable_endian.h"

#ifndef TARGET_WIN32
#include <sys/select.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

int nat_init(nat_ctx_t *ctx, shadowvpn_args_t *args) {
  int i;
  bzero(ctx, sizeof(nat_ctx_t));
  for (i = 0; i < args->user_tokens_len; i++) {
    client_info_t *client = malloc(sizeof(client_info_t));
    bzero(client, sizeof(client_info_t));

    memcpy(client->user_token, args->user_tokens[i], SHADOWVPN_USERTOKEN_LEN);

    // assign IP based on tun IP and user tokens
    // for example:
    //     tun IP is 10.7.0.1
    //     client IPs will be 10.7.0.2, 10.7.0.3, 10.7.0.4, etc
    client->output_tun_ip = htonl(args->netip + i + 1);

    struct in_addr in;
    in.s_addr = client->output_tun_ip;
    logf("assigning %s to user %16llx",
         inet_ntoa(in),
         htobe64(*((uint64_t *)args->user_tokens[i])));

    // add to hash: ctx->token_to_clients[user_token] = client
    HASH_ADD(hh1, ctx->token_to_clients, user_token,
             SHADOWVPN_USERTOKEN_LEN, client);

    // add to hash: ctx->ip_to_clients[output_tun_ip] = client
    HASH_ADD(hh2, ctx->ip_to_clients, output_tun_ip, 4, client);
  }
  return 0;
}

/*
   RFC791
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |Version|  IHL  |Type of Service|          Total Length         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         Identification        |Flags|      Fragment Offset    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Time to Live |    Protocol   |         Header Checksum       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       Source Address                          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Destination Address                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Options                    |    Padding    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

typedef struct {
  uint8_t ver;
  uint8_t tos;
  uint16_t total_len;
  uint16_t id;
  uint16_t frag;
  uint8_t ttl;
  uint8_t proto;
  uint16_t checksum;
  uint32_t saddr;
  uint32_t daddr;
} ipv4_hdr_t;

typedef struct {
  uint16_t sport;
  uint16_t dport;
  uint32_t seq;
  uint32_t ack;
  uint32_t not_interested;
  uint16_t checksum;
  uint16_t upt;
} tcp_hdr_t;

typedef struct {
  uint16_t sport;
  uint16_t dport;
  uint16_t len;
  uint16_t checksum;
} udp_hdr_t;

// from OpenVPN
// acc is the changes (+ old - new)
// cksum is the checksum to adjust
#define ADJUST_CHECKSUM(acc, cksum) { \
  int _acc = acc; \
  _acc += (cksum); \
  if (_acc < 0) { \
    _acc = -_acc; \
    _acc = (_acc >> 16) + (_acc & 0xffff); \
    _acc += _acc >> 16; \
    (cksum) = (uint16_t) ~_acc; \
  } else { \
    _acc = (_acc >> 16) + (_acc & 0xffff); \
    _acc += _acc >> 16; \
    (cksum) = (uint16_t) _acc; \
  } \
}

int nat_fix_upstream(nat_ctx_t *ctx, unsigned char *buf, size_t buflen,
                     const struct sockaddr *addr, socklen_t addrlen) {
  uint8_t iphdr_len;
  if (buflen < SHADOWVPN_USERTOKEN_LEN + 20) {
    errf("nat: ip packet too short");
    return -1;
  }
  ipv4_hdr_t *iphdr = (ipv4_hdr_t *)(buf + SHADOWVPN_USERTOKEN_LEN);
  if ((iphdr->ver & 0xf0) != 0x40) {
    // check header, currently IPv4 only
    // bypass IPv6
    return 0;
  }
  iphdr_len = (iphdr->ver & 0x0f) * 4;

  // print_hex_memory(buf, SHADOWVPN_USERTOKEN_LEN);
  client_info_t *client = NULL;
  HASH_FIND(hh1, ctx->token_to_clients, buf, SHADOWVPN_USERTOKEN_LEN, client);
  if (client == NULL) {
    errf("nat: client not found for given user token");
    return -1;
  }
  // print_hex_memory(iphdr, buflen - SHADOWVPN_USERTOKEN_LEN);

  // save source address
  client->source_addr.addrlen =  addrlen;
  memcpy(&client->source_addr.addr, addr, addrlen);

  int32_t acc = 0;
  // save tun input ip to client
  client->input_tun_ip = iphdr->saddr;

  // overwrite IP
  iphdr->saddr = client->output_tun_ip;

  // add old, sub new
  acc = client->input_tun_ip - iphdr->saddr;
  ADJUST_CHECKSUM(acc, iphdr->checksum);

  void *ip_payload = buf + SHADOWVPN_USERTOKEN_LEN + iphdr_len;
  if (iphdr->proto == IPPROTO_TCP) {
    if (buflen < iphdr_len + 20) {
      errf("nat: tcp packet too short");
      return -1;
    }
    tcp_hdr_t *tcphdr = ip_payload;
    ADJUST_CHECKSUM(acc, tcphdr->checksum);
  } else if (iphdr->proto == IPPROTO_UDP) {
    if (buflen < iphdr_len + 8) {
      errf("nat: udp packet too short");
      return -1;
    }
    udp_hdr_t *udphdr = ip_payload;
    ADJUST_CHECKSUM(acc, udphdr->checksum);
  }
  return 0;
}

int nat_fix_downstream(nat_ctx_t *ctx, unsigned char *buf, size_t buflen,
                       struct sockaddr *addr, socklen_t *addrlen) {
  uint8_t iphdr_len;
  if (buflen < SHADOWVPN_USERTOKEN_LEN + 20) {
    errf("nat: ip packet too short");
    return -1;
  }
  ipv4_hdr_t *iphdr = (ipv4_hdr_t *)(buf + SHADOWVPN_USERTOKEN_LEN);
  if ((iphdr->ver & 0xf0) != 0x40) {
    // check header, currently IPv4 only
    // bypass IPv6
    return 0;
  }
  iphdr_len = (iphdr->ver & 0x0f) * 4;

  client_info_t *client = NULL;
  // print_hex_memory(iphdr, buflen - SHADOWVPN_USERTOKEN_LEN);

  HASH_FIND(hh2, ctx->ip_to_clients, &iphdr->daddr, 4, client);
  if (client == NULL) {
    errf("nat: client not found for given user ip");
    return -1;
  }

  // print_hex_memory(client->user_token, SHADOWVPN_USERTOKEN_LEN);

  // update dest address
  *addrlen = client->source_addr.addrlen;
  memcpy(addr, &client->source_addr.addr, *addrlen);
  
  // copy usertoken back
  memcpy(buf, client->user_token, SHADOWVPN_USERTOKEN_LEN);

  int32_t acc = 0;

  // add old, sub new
  acc = iphdr->daddr - client->input_tun_ip;

  // overwrite IP
  iphdr->daddr = client->input_tun_ip;

  ADJUST_CHECKSUM(acc, iphdr->checksum);

  void *ip_payload = buf + SHADOWVPN_USERTOKEN_LEN + iphdr_len;
  if (iphdr->proto == IPPROTO_TCP) {
    if (buflen < iphdr_len + 20) {
      errf("nat: tcp packet too short");
      return -1;
    }
    tcp_hdr_t *tcphdr = ip_payload;
    ADJUST_CHECKSUM(acc, tcphdr->checksum);
  } else if (iphdr->proto == IPPROTO_UDP) {
    if (buflen < iphdr_len + 8) {
      errf("nat: udp packet too short");
      return -1;
    }
    udp_hdr_t *udphdr = ip_payload;
    ADJUST_CHECKSUM(acc, udphdr->checksum);
  }
  return 0;
}


