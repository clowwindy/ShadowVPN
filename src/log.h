/**
  log.h

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

#ifndef LOG_H
#define LOG_H

#include <time.h>
#include <string.h>
#include <stdio.h>
#include "config.h"

extern int verbose_mode;

#define __LOG(o, t, v, s...) do {                                   \
  time_t now;                                                       \
  time(&now);                                                       \
  char *time_str = ctime(&now);                                     \
  time_str[strlen(time_str) - 1] = '\0';                            \
  if (t == 0) {                                                     \
    if (stdout != o || verbose_mode) {                              \
      fprintf(o, "%s ", time_str);                                  \
      fprintf(o, s);                                                \
      fprintf(o, "\n");                                             \
      fflush(o);                                                    \
    }                                                               \
  } else if (t == 1) {                                              \
    fprintf(o, "%s %s:%d ", time_str, __FILE__, __LINE__);          \
    perror(v);                                                      \
  }                                                                 \
} while (0)

#define logf(s...) __LOG(stdout, 0, "_", s)
#define err(s) __LOG(stderr, 1, s, "_")
#define errf(s...) __LOG(stderr, 0, "_", s)

#ifdef DEBUG
#define debugf(s...) logf(s)
#else
#define debugf(s...)
#endif

#endif
