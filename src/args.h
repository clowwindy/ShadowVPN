#ifndef ARGS_H
#define ARGS_H

#include <stdint.h>

typedef enum {
  SHADOWVPN_MODE_SERVER = 1,
  SHADOWVPN_MODE_CLIENT = 2
} shadowvpn_mode;

typedef enum {
  SHADOWVPN_CMD_NONE = 0,
  SHADOWVPN_CMD_START,
  SHADOWVPN_CMD_STOP,
  SHADOWVPN_CMD_RESTART
} shadowvpn_cmd;

typedef struct {
  shadowvpn_mode mode;
  shadowvpn_cmd cmd;
  const char *conf_file;
  const char *pid_file;
  const char *intf;
  const char *password;
  const char *server;
  uint16_t port;
  uint16_t mtu;
  const char *up_script;
  const char *down_script;
} shadowvpn_args_t;

int args_parse(shadowvpn_args_t *args, int argc, char **argv);

#endif
