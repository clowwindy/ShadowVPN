/**
  args.c

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
#include <string.h>
#include <getopt.h>
#include "shadowvpn.h"

static const char *help_message =
"usage: shadowvpn -c config_file [-s start/stop/restart] [-v]\n"
"\n"
"  -h, --help            show this help message and exit\n"
"  -s start/stop/restart control shadowvpn process. if omitted, will run\n"
"                        in foreground\n"
"  -c config_file        path to config file\n"
"  -t                    TCP mode(fake tcp header)\n"
"  -q                    queue to bind (<65535)\n"
"  -v                    verbose logging\n"
"\n"
"Online help: <https://github.com/clowwindy/ShadowVPN>\n";

static void print_help() __attribute__ ((noreturn));

static void load_default_args(shadowvpn_args_t *args);

static int process_key_value(shadowvpn_args_t *args, const char *key,
                      const char *value);

static void print_help() {
  printf("%s", help_message);
  exit(1);
}

static int parse_config_file(shadowvpn_args_t *args, const char *filename) {
  char buf[512];
  char *line;
  FILE *fp;
  size_t len = sizeof(buf);
  int lineno = 0;

  fp = fopen(filename, "rb");
  if (fp == NULL) {
    err("fopen");
    errf("Can't open config file: %s", filename);
    return -1;
  }
  while ((line = fgets(buf, len, fp))) {
    char *sp_pos;
    lineno++;
    sp_pos = strchr(line, '\r');
    if (sp_pos) *sp_pos = '\n';
    sp_pos = strchr(line, '\n');
    if (sp_pos) {
      *sp_pos = 0;
    } else {
      errf("line %d too long in %s", lineno, filename);
      return -1;
    }
    if (*line == 0 || *line == '#')
      continue;
    sp_pos = strchr(line, '=');
    if (!sp_pos) {
      errf("%s:%d: \"=\" is not found in this line: %s", filename, lineno,
           line);
      return -1;
    }
    *sp_pos = 0;
    sp_pos++;
    // line points to key and sp_pos points to value
    if (0 != process_key_value(args, line, sp_pos))
      return 1;
  }
  // check if every required arg is set
  if (!args->mode) {
    errf("mode not set in config file");
    return -1;
  }
  if (!args->server) {
    errf("server not set in config file");
    return -1;
  }
  if (!args->port) {
    errf("port not set in config file");
    return -1;
  }
  if (!args->password) {
    errf("password not set in config file");
    return -1;
  }
#ifdef TARGET_WIN32
  if (!args->tun_ip) {
    errf("tunip not set in config file");
    return -1;
  }
#endif
  return 0;
}

static int process_key_value(shadowvpn_args_t *args, const char *key,
                      const char *value) {
  if (strcmp("password", key) != 0) {
    // set environment variables so that up/down script can
    // make use of these values
    if (-1 == setenv(key, value, 1)) {
      err("setenv");
      return -1;
    }
  }
  if (strcmp("server", key) == 0) {
    args->server = strdup(value);
  } else if (strcmp("port", key) == 0) {
    args->port = atol(value);
  } else if (strcmp("concurrency", key) == 0) {
    args->concurrency = atol(value);
    if (args->concurrency == 0) {
      errf("concurrency should >= 1");
      return -1;
    }
    if (args->concurrency > 100) {
      errf("concurrency should <= 100");
      return -1;
    }
  } else if (strcmp("password", key) == 0) {
    args->password = strdup(value);
  } else if (strcmp("mode", key) == 0) {
    if (strcmp("server", value) == 0) {
      args->mode = SHADOWVPN_MODE_SERVER;
    } else if (strcmp("client", value) == 0) {
      args->mode = SHADOWVPN_MODE_CLIENT;
    } else {
      errf("warning: unknown mode in config file: %s", value);
      return -1;
    }
  } else if (strcmp("mtu", key) == 0) {
    long mtu = atol(value);
    // RFC 791
    // in order to wrap packet of length 68, MTU should be > 68 + overhead
    if (mtu < 68 + SHADOWVPN_OVERHEAD_LEN) {
      errf("MTU %ld is too small", mtu);
      return -1;
    }
    if (mtu > MAX_MTU) {
      errf("MTU %ld is too large", mtu);
      return -1;
    }
    args->mtu = mtu;
  } else if (strcmp("intf", key) == 0) {
    args->intf = strdup(value);
  } else if (strcmp("pidfile", key) == 0) {
    args->pid_file = strdup(value);
  } else if (strcmp("logfile", key) == 0) {
    args->log_file = strdup(value);
  } else if (strcmp("up", key) == 0) {
    args->up_script = strdup(value);
  } else if (strcmp("down", key) == 0) {
    args->down_script = strdup(value);
  }
#ifdef TARGET_WIN32
  else if (strcmp("tunip", key) == 0) {
    args->tun_ip = strdup(value);
  } else if (strcmp("tunmask", key) == 0) {
    args->tun_mask = (int) atol(value);
  } else if (strcmp("tunport", key) == 0) {
    args->tun_port = (int) atol(value);
  }
#endif
  else {
    errf("warning: unknown key in config file: %s", key);
  }
  return 0;
}

static void load_default_args(shadowvpn_args_t *args) {
#ifdef TARGET_DARWIN
  args->intf = "utun0";
#else
  args->intf = "tun0";
#endif
  args->mtu = 1440;
  args->pid_file = "/var/run/shadowvpn.pid";
  args->log_file = "/var/log/shadowvpn.log";
  args->concurrency = 1;
#ifdef TARGET_WIN32
  args->tun_mask = 24;
  args->tun_port = TUN_DELEGATE_PORT;
#endif
}

int args_parse(shadowvpn_args_t *args, int argc, char **argv) {
  int ch;
  bzero(args, sizeof(shadowvpn_args_t));
  while ((ch = getopt(argc, argv, "hs:c:q:tv")) != -1) {
    switch (ch) {
      case 's':
        if (strcmp("start", optarg) == 0)
          args->cmd = SHADOWVPN_CMD_START;
        else if (strcmp("stop", optarg) == 0)
          args->cmd = SHADOWVPN_CMD_STOP;
        else if (strcmp("restart", optarg) == 0)
          args->cmd = SHADOWVPN_CMD_RESTART;
        else {
          errf("unknown command %s", optarg);
          print_help();
         }
        break;
      case 'c':
        args->conf_file = strdup(optarg);
        break;
      case 'q':
        if (atoi(optarg)>65535) {
          errf("queue num must be smaller than 65535");
          print_help();
        } else {
          args->queue_num = atoi(optarg);
          if (-1 == setenv("queue_num", optarg, 1))
            err("setenv");
        }
        break;
      case 't':
        args->tcp_mode = 1;
        if (-1 == setenv("tcp_mode", "1", 1))
          err("setenv");
        break;
      case 'v':
        verbose_mode = 1;
        break;
       default:
        print_help();
    }
  }
  if (!args->conf_file)
    print_help();
  load_default_args(args);
  return parse_config_file(args, args->conf_file);
}
