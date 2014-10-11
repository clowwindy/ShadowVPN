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

#include <time.h>
#include "shadowvpn.h"

int verbose_mode;

void log_timestamp(FILE *out) {
  time_t now;
  time(&now);
  char *time_str = ctime(&now);
  time_str[strlen(time_str) - 1] = '\0';
  fprintf(out, "%s ", time_str);
}

void perror_timestamp(const char *msg, const char *file, int line) {
  log_timestamp(stderr);
  fprintf(stderr, "%s:%d ", file, line);
#ifdef TARGET_WIN32
  LPVOID *err_str = NULL;
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, WSAGetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &err_str, 0, NULL);
  if (err_str != NULL) {
    fprintf(stderr, "%s: %s\n", msg, (char *)err_str);
    LocalFree(err_str);
  }
#else
  perror(msg);
#endif
}

