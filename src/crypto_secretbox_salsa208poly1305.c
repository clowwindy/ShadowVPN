/**
  crypto_secretbox_salsa208poly1305.c

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

int crypto_secretbox_salsa208poly1305(
  unsigned char *c,
  const unsigned char *m,unsigned long long mlen,
  const unsigned char *n,
  const unsigned char *k
)
{
  int i;
  if (mlen < 32) return -1;
  crypto_stream_salsa208_xor(c,m,mlen,n,k);
  crypto_onetimeauth_poly1305(c + 16,c + 32,mlen - 32,c);
  for (i = 0;i < 16;++i) c[i] = 0;
  return 0;
}

int crypto_secretbox_salsa208poly1305_open(
  unsigned char *m,
  const unsigned char *c,unsigned long long clen,
  const unsigned char *n,
  const unsigned char *k
)
{
  int i;
  unsigned char subkey[32];
  if (clen < 32) return -1;
  crypto_stream_salsa208(subkey,32,n,k);
  if (crypto_onetimeauth_poly1305_verify(c + 16,c + 32,clen - 32,subkey) != 0) return -1;
  crypto_stream_salsa208_xor(m,c,clen,n,k);
  for (i = 0;i < 32;++i) m[i] = 0;
  return 0;
}
