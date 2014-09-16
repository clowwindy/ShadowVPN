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
