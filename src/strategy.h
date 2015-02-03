/**
  strategy.h

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

#ifndef STRATEGY_H
#define STRATEGY_H

#include "vpn.h"

// choose a socket by random
// return chosen socket
int strategy_choose_socket(vpn_ctx_t *ctx);

// choose a reasonable remote address based on magic
// update ctx->remote_addr and remote_addrlen
// return 0 on success
int strategy_choose_remote_addr(vpn_ctx_t *ctx);

// update remote address list from remote_addr and remote_addrlen
void strategy_update_remote_addr_list(vpn_ctx_t *ctx);

#endif
