#ifndef ARGS_H
#define ARGS_H

#include <stdint.h>

typedef enum {
  SHADOWVPN_MODE_SERVER, SHADOWVPN_MODE_CLIENT
} shadowvpn_mode;

typedef struct {
  shadowvpn_mode mode;
  const char *intf;
  const char *pif_file;
  const char *password;
  const char *server;
  uint16_t port;
  uint16_t mtu;
  const char *up_script;
  const char *down_script;

} shadowvpn_args_t;

int args_parse(shadowvpn_args_t *args, int argc, char **argv);

#endif
