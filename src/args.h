/**
  args.h

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

#ifndef ARGS_H
#define ARGS_H

#include <stdint.h>

#define MAX_MTU 9000

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
  const char *log_file;
  const char *intf;
  const char *password;
  const char *server;
  uint16_t port;
  uint16_t mtu;
  uint16_t concurrency;
  const char *up_script;
  const char *down_script;
#ifdef TARGET_WIN32
  const char *tun_ip;
  int tun_mask;
  int tun_port;
#endif
} shadowvpn_args_t;

int args_parse(shadowvpn_args_t *args, int argc, char **argv);

#endif
