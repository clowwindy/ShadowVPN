/**
  crypto.h

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

#ifndef CRYPTO_H
#define CRYPTO_H

/* call once after start */
int crypto_init();

// TODO use a struct to hold context instead
/* call when password changed */
int crypto_set_password(const char *password,
                        unsigned long long password_len);

int crypto_encrypt(unsigned char *c, unsigned char *m,
                   unsigned long long mlen);

int crypto_decrypt(unsigned char *m, unsigned char *c,
                   unsigned long long clen);

#define SHADOWVPN_KEY_LEN 32
#define SHADOWVPN_ZERO_BYTES 32
#define SHADOWVPN_OVERHEAD_LEN 24
#define SHADOWVPN_PACKET_OFFSET 8

#endif
