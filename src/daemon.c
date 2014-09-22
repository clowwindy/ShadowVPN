/**
  deamon.c

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

#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "shadowvpn.h"

#define PID_BUF_SIZE 32

static int write_pid_file(const char *filename, pid_t pid);

int daemon_start(const shadowvpn_args_t *args) {
  pid_t pid = fork();
  if (pid == -1) {
    err("fork");
    return -1;
  }
  if (pid > 0) {
    // let the child print message to the console first
    usleep(100);
    exit(0);
  } 

  pid = getpid();
  if (0 != write_pid_file(args->pid_file, pid))
    return -1;

  // print on console
  printf("started\n");

  // then rediret stdout & stderr
  fclose(stdin);
  FILE *fp;
  fp = freopen(args->log_file, "a", stdout);
  if (fp == NULL) {
    err("freopen");
    return -1;
  }
  fp = freopen(args->log_file, "a", stderr);
  if (fp == NULL) {
    err("freopen");
    return -1;
  }

  return 0;
}

static int write_pid_file(const char *filename, pid_t pid) {
  char buf[PID_BUF_SIZE];
  int fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    errf("can not open %s", filename);
    err("open");
    return -1;
  }
  int flags = fcntl(fd, F_GETFD);
  if (flags == -1) {
    err("fcntl");
    return -1;
  }

  flags |= FD_CLOEXEC;
  if (-1 == fcntl(fd, F_SETFD, flags))
    err("fcntl");

  struct flock fl;
  fl.l_type = F_WRLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = 0;
  fl.l_len = 0;
  if (-1 == fcntl(fd, F_SETLK, &fl)) {
    ssize_t n = read(fd, buf, PID_BUF_SIZE - 1);
    if (n > 0) {
      buf[n] = 0;
      errf("already started at pid %ld", atol(buf));
    } else {
      errf("already started");
    }
    close(fd);
    return -1;
  }
  if (-1 == ftruncate(fd, 0)) {
    err("ftruncate");
    return -1;
  }
  snprintf(buf, PID_BUF_SIZE, "%ld\n", (long)getpid());

  if (write(fd, buf, strlen(buf)) != strlen(buf)) {
    err("write");
    return -1;
  }
  return 0;
}

int daemon_stop(const shadowvpn_args_t *args) {
  char buf[PID_BUF_SIZE];
  FILE *fp = fopen(args->pid_file, "r");
  if (fp == NULL) {
    errf("not started");
    return -1;
  }
  char *line = fgets(buf, PID_BUF_SIZE, fp);
  fclose(fp);
  pid_t pid = (pid_t)atol(buf);
  if (pid > 0) {
    // make sure pid is not zero or negative
    if (0 != kill(pid, SIGTERM)){
      err("kill");
      return -1;
    }
    printf("stopped\n");
    if (0 != unlink(args->pid_file)) {
      err("unlink");
      return -1;
    }
  } else {
    errf("pid is not positive: %ld", (long)pid);
    return -1;
  }
  return 0;
}
