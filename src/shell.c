/**
  shell.c

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

#include <stdlib.h>
#include <stdio.h>
#include "shadowvpn.h"
#include "shell.h"

static int shell_run(shadowvpn_args_t *args, int is_up);

int shell_up(shadowvpn_args_t *args) {
  return shell_run(args, 1);
}

int shell_down(shadowvpn_args_t *args) {
  return shell_run(args, 0);
}

static int shell_run(shadowvpn_args_t *args, int is_up) {
  const char *script;
  char *buf;
  int r;
  if (is_up) {
    script = args->up_script;
  } else {
    script = args->down_script;
  }
  if (script == NULL || script[0] == 0) {
    errf("warning: script not set");
    return 0;
  }
  buf = malloc(strlen(script) + 8);
#ifdef TARGET_WIN32
  sprintf(buf, "cmd /c %s", script);
#else
  sprintf(buf, "sh %s", script);
#endif
  logf("executing %s", script);
  if (0 != (r = system(buf))) {
    free(buf);
    errf("script %s returned non-zero return code: %d", script, r);
    return -1;
  }
  free(buf);
  return 0;
}


