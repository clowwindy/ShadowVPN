/**
  vpn.c

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

#include <sodium.h>
#include <string.h>
#include "crypto_secretbox_salsa208poly1305.h"

// will not copy key any more
static unsigned char key[32];

int crypto_init() {
  if (-1 == sodium_init())
    return 1;
  randombytes_set_implementation(&randombytes_salsa20_implementation);
  randombytes_stir();
  return 0;
}

int crypto_set_password(const char *password,
                        unsigned long long password_len) {
  return crypto_generichash(key, sizeof key, (unsigned char *)password,
                            password_len, NULL, 0);
}

int crypto_encrypt(unsigned char *c, unsigned char *m,
                   unsigned long long mlen) {
  unsigned char nonce[8];
  randombytes_buf(nonce, 8);
  int r = crypto_secretbox_salsa208poly1305(c, m, mlen, nonce, key);
  if (r != 0) return r;
  // copy nonce to the head
  memcpy(c + 8, nonce, 8);
  return 0;
}

int crypto_decrypt(unsigned char *m, unsigned char *c,
                   unsigned long long clen) {
  unsigned char nonce[8];
  memcpy(nonce, c + 8, 8);
  int r = crypto_secretbox_salsa208poly1305_open(m, c, clen, nonce, key);
  if (r != 0) return r;
  return 0;
}

