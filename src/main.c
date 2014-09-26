/**
  main.c

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "shadowvpn.h"

static void sig_handler(int signo) {
  exit(0);
}

int main(int argc, char **argv) {
  shadowvpn_args_t args;
  // parse args
  if (0 != args_parse(&args, argc, argv)) {
    errf("error when parsing args");
    return EXIT_FAILURE;
  }
  if (args.cmd == SHADOWVPN_CMD_START) {
    if (0 != daemon_start(&args)) {
      errf("can not start daemon");
      return EXIT_FAILURE;
    }
  } else if (args.cmd == SHADOWVPN_CMD_STOP) {
    if (0 != daemon_stop(&args)) {
      errf("can not stop daemon");
      return EXIT_FAILURE;
    }
    // always exit if we are exec stop cmd
    return 0;
  } else if (args.cmd == SHADOWVPN_CMD_RESTART) {
    if (0 != daemon_stop(&args)) {
      errf("can not stop daemon");
      return EXIT_FAILURE;
    }
    if (0 != daemon_start(&args)) {
      errf("can not start daemon");
      return EXIT_FAILURE;
    }
  }

  if (0 != crypto_init()) {
    errf("shadowvpn_crypto_init");
    return EXIT_FAILURE;
  }

  if (0 !=crypto_set_password(args.password, strlen(args.password))) {
    errf("can not set password");
    return EXIT_FAILURE;
  }

  signal(SIGINT, sig_handler);

  return run_vpn(&args);;
}
