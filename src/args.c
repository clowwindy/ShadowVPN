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
"                        in foreground"
"  -c config_file        path to config file\n"
"  -v                    verbose logging\n"
"\n"
"Online help: <https://github.com/clowwindy/ShadowVPN>\n";

void print_help() __attribute__ ((noreturn));

void load_default_args(shadowvpn_args_t *args);

int process_key_value(shadowvpn_args_t *args, const char *key,
                      const char *value);

void print_help() {
  printf("%s", help_message);
  exit(1);
}

int parse_config_file(shadowvpn_args_t *args, const char *filename) {
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
    if (sp_pos) *sp_pos = 0;
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
  return 0;
}

int process_key_value(shadowvpn_args_t *args, const char *key,
                      const char *value) {
  if (strcmp("server", key) == 0) {
    args->server = strdup(value);
  } else if (strcmp("port", key) == 0) {
    args->port = atol(value);
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
    if (mtu > UINT16_MAX) {
      errf("MTU %ld is too large", mtu);
      return -1;
    }
    args->mtu = mtu;
  } else if (strcmp("intf", key) == 0) {
    args->intf = strdup(value);
  } else if (strcmp("pidfile", key) == 0) {
    args->pid_file = strdup(value);
  } else if (strcmp("up", key) == 0) {
    args->up_script = strdup(value);
  } else if (strcmp("down", key) == 0) {
    args->down_script = strdup(value);
  } else {
    errf("warning: unknown key in config file: %s", key);
  }
  return 0;
}

void load_default_args(shadowvpn_args_t *args) {
  args->pid_file = "/var/run/shadowvpn.pid";
  args->intf = "tun0";
  args->mtu = 1500;
}

int args_parse(shadowvpn_args_t *args, int argc, char **argv) {
  int ch;
  bzero(args, sizeof(shadowvpn_args_t));
  while ((ch = getopt(argc, argv, "hs:c:v")) != -1) {
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
