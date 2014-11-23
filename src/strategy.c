/**
  strategy.c

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

#include "shadowvpn.h"

#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#ifndef TARGET_WIN32
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <sodium.h>

#define ADDR_TIMEOUT 30

int strategy_choose_socket(vpn_ctx_t *ctx) {
  // rule: just pick a random socket
  if (ctx->nsock == 1) {
    return ctx->socks[0];
  }
  uint32_t r = randombytes_uniform(ctx->nsock);
  return ctx->socks[r];
}

static inline void save_addr(addr_info_t *addr_info, struct sockaddr *addrp,
                             socklen_t addrlen, time_t now) {
  memcpy(&addr_info->addr, addrp, addrlen);
  addr_info->addrlen = addrlen;
  addr_info->last_recv_time = now;
}

static inline void load_addr(addr_info_t *addr_info,
                             struct sockaddr_storage *addr,
                             socklen_t *addrlen) {
  memcpy(addr, &addr_info->addr, addr_info->addrlen);
  *addrlen = addr_info->addrlen;
}

int strategy_choose_remote_addr(vpn_ctx_t *ctx) {
  // rules:
  // 1. if there isn't any address received from within ADDR_TIMEOUT
  //    choose latest
  // 2. if there are some addresses received from within ADDR_TIMEOUT
  //    choose randomly from them
  //
  // how we do this efficiently
  // 1. scan once and find latest, total number of not timed out addresses
  // 2. if number <= 1, use latest
  // 3. if number > 1, generate random i in (0, number),
  //    scan again and pick (i)th address not timed out
  int i, total_not_timed_out, chosen;
  time_t now;
  addr_info_t *latest = NULL, *temp;

  if (ctx->nknown_addr == 0) {
    return 0;
  }

  time(&now);

  for (i = 0; i < ctx->nknown_addr; i++) {
    temp = &ctx->known_addrs[i];
    if (latest == NULL ||
        latest->last_recv_time < temp->last_recv_time) {
      latest = temp;
    }
    if (now - temp->last_recv_time > ADDR_TIMEOUT) {
      total_not_timed_out++;
    }
  }
  if (total_not_timed_out <= 1) {
    load_addr(latest, &ctx->remote_addr, &ctx->remote_addrlen);
  } else {
    chosen = randombytes_uniform(total_not_timed_out);
    total_not_timed_out = 0;
    for (i = 0; i < ctx->nknown_addr; i++) {
      temp = &ctx->known_addrs[i];
      if (now - temp->last_recv_time > ADDR_TIMEOUT) {
        if (total_not_timed_out == chosen) {
          load_addr(&ctx->known_addrs[i], &ctx->remote_addr,
                    &ctx->remote_addrlen);
          break;
        }
        total_not_timed_out++;
      }
    }
  }
  return 0;
}

void strategy_update_remote_addr_list(vpn_ctx_t *ctx) {
  int i;
  time_t now;
  struct sockaddr *remote_addrp = ctx->remote_addrp;

  time(&now);
  socklen_t remote_addrlen = ctx->remote_addrlen;

  // if already in list, update time and return
  for (i = 0; i < ctx->nknown_addr; i++) {
    if (remote_addrlen == ctx->known_addrs[i].addrlen) {
      if (0 == memcmp(remote_addrp, &ctx->known_addrs[i].addr,
                      remote_addrlen)) {
        ctx->known_addrs[i].last_recv_time = now;
        return;
      }
    }
  }
  // if address list is not full, just append remote addr
  if (ctx->nknown_addr < ctx->args->concurrency) {
    save_addr(&ctx->known_addrs[ctx->nknown_addr], remote_addrp,
              remote_addrlen, now);
    ctx->nknown_addr++;
    return;
  }
  // if full, replace the oldest
  addr_info_t *oldest_addr_info = NULL;
  for (i = 0; i < ctx->nknown_addr; i++) {
    if (oldest_addr_info == NULL ||
        oldest_addr_info->last_recv_time >
          ctx->known_addrs[i].last_recv_time) {
      oldest_addr_info = &ctx->known_addrs[i];
    }
  }
  if (oldest_addr_info) {
    save_addr(oldest_addr_info, remote_addrp, remote_addrlen, now);
  }
}

