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

int strategy_choose_socket(vpn_ctx_t *ctx) {
  if (ctx->nsock == 1) {
    return ctx->socks[0];
  }
  uint32_t r = randombytes_uniform(ctx->nsock);
  return ctx->socks[r];
}

int strategy_choose_remote_addr(vpn_ctx_t *ctx) {
  // TODO implement this

  return 0;
}

static inline void copy_addr(addr_info_t *addr_info, struct sockaddr *addrp,
                             socklen_t addrlen, time_t now) {
  memcpy(&addr_info->addr, addrp, addrlen);
  addr_info->addrlen = addrlen;
  addr_info->last_recv_time = now;
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
    copy_addr(&ctx->known_addrs[ctx->nknown_addr], remote_addrp,
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
    copy_addr(oldest_addr_info, remote_addrp, remote_addrlen, now);
  }
}

