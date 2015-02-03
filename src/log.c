/**
  vpn.c

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

